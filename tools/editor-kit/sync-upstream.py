#!/usr/bin/env python3
"""Review/refresh *source-only* Beckenburg editor modules used by ReverieVR.

Default is offline integrity check. --check-upstream performs read-only
comparison. --apply explicitly updates only pristine vendored Java files; it
never touches ReverieVR game-specific editors or auto-runs during a release.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parents[2]
LOCK = ROOT / "tools/editor-kit/upstream-lock.json"
API = "https://api.github.com/repos/mrcalzon02/The-Beckenburg-Project"
RAW = "https://raw.githubusercontent.com/mrcalzon02/The-Beckenburg-Project"

def blob_hash(data):
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()

def read_json(url):
    req = Request(url, headers={"Accept":"application/vnd.github+json",
                                "User-Agent":"ReverieVR-editor-sync"})
    with urlopen(req, timeout=25) as response:
        return json.load(response)

def read_bytes(url):
    with urlopen(Request(url,headers={"User-Agent":"ReverieVR-editor-sync"}),
                 timeout=25) as response:
        return response.read()

def is_allowed(path, includes):
    return path.endswith(".java") and any(path.startswith(x) for x in includes) \
        and ".." not in Path(path).parts and not path.startswith("/")

def verify_local(lock):
    issues = []
    for entry in lock["files"]:
        source = entry["path"]
        if not is_allowed(source,lock["includes"]):
            issues.append("Unsafe tracked path: " + source)
            continue
        dest = ROOT / lock["prefix"] / source
        if not dest.is_file():
            issues.append("Missing: " + str(dest.relative_to(ROOT)))
        elif blob_hash(dest.read_bytes()) != entry["blob"]:
            issues.append("Modified locally: " + str(dest.relative_to(ROOT)))
    return issues

def atomic_write(path, data):
    path.parent.mkdir(parents=True,exist_ok=True)
    descriptor, temp = tempfile.mkstemp(prefix=".upstream-",dir=str(path.parent))
    try:
        with os.fdopen(descriptor,"wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temp,path)
    finally:
        if os.path.exists(temp):
            os.unlink(temp)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--check-upstream",action="store_true")
    group.add_argument("--apply",action="store_true")
    args = parser.parse_args()
    lock = json.loads(LOCK.read_text("utf-8"))
    if lock["schema"] != 1 or lock["repository"] != "mrcalzon02/The-Beckenburg-Project":
        parser.error("Untrusted upstream lock")
    problems = verify_local(lock)
    if problems:
        for issue in problems: print("ERROR:",issue)
        parser.error("Local vendor diverged; protect local changes before refreshing")
    print("PASS: locally pinned editor sources:",len(lock["files"]),
          "at",lock["revision"])
    if not args.check_upstream and not args.apply: return
    branch = read_json(API + "/git/ref/heads/main")["object"]["sha"]
    tree = read_json(API + "/git/trees/" + branch + "?recursive=1")
    if tree.get("truncated"): parser.error("Remote tree truncated; refuse partial sync")
    remote = {
        item["path"]: item["sha"] for item in tree["tree"]
        if item["type"] == "blob" and is_allowed(item["path"],lock["includes"])
    }
    previous = {item["path"]:item["blob"] for item in lock["files"]}
    removed = sorted(set(previous)-set(remote))
    changed = sorted(path for path,sha in remote.items()
                     if previous.get(path) != sha)
    print("Upstream:",branch,"changed/new:",len(changed),"removed:",len(removed))
    for path in changed: print("  UPDATE",path)
    for path in removed: print("  REMOVED",path)
    if not args.apply: return
    if removed: parser.error("Upstream deletions require explicit compatibility review")
    if not changed: print("Already current"); return
    # Download and hash-verify EVERYTHING before any local mutation.
    updates = {}
    for path in changed:
        dest = ROOT / lock["prefix"] / path
        if dest.exists() and path not in previous:
            parser.error("Untracked destination conflicts with new upstream file: "+path)
        data = read_bytes(RAW + "/" + branch + "/" + path)
        if blob_hash(data) != remote[path]:
            parser.error("Download checksum differs from source git blob: "+path)
        updates[path] = data
    for path,data in updates.items():
        atomic_write(ROOT / lock["prefix"] / path,data)
    lock["files"]=[{"path":p,"blob":remote[p]} for p in sorted(remote)]
    lock["revision"]=branch
    atomic_write(LOCK,(json.dumps(lock,indent=2)+"\n").encode("utf-8"))
    print("UPDATED",len(changed),"vendored Java sources to",branch)
    print("Next: run tools/desktop/test.sh and tools/sitka/test-editor.sh;")
    print("inspect changes before committing. Runtime/game adapters were NOT overwritten.")

if __name__ == "__main__":
    main()
