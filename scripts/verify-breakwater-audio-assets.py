#!/usr/bin/env python3
"""Verify the actual Breakwater bank, mapping, provenance and optional APK bytes.

No decoder, numpy, network or Android installation is needed. This reads the
quantized WAV samples independently of the preparation tool's reported metrics.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import zipfile

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "app/src/main/assets/native/breakwater/audio"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def rms_active(values: tuple[int, ...]) -> float:
    blocks = []
    for at in range(0, len(values), 441):
        block = values[at:at + 441]
        blocks.append((sum(v * v for v in block), len(block)))
    best = max(math.sqrt(energy / count) for energy, count in blocks)
    threshold = best * 10 ** (-25 / 20)
    active = [(e, n) for e, n in blocks if math.sqrt(e / n) >= threshold]
    return math.sqrt(sum(e for e, _ in active) / sum(n for _, n in active)) / 32768


def verify(apk: Path | None = None) -> dict:
    manifest = json.loads((ROOT / "art/breakwater/audio_manifest.json").read_text())
    script = ROOT / "tools/breakwater/audio/prepare_breakwater_audio.py"
    catalog = ROOT / "tools/breakwater/audio/source_catalog.json"
    require(sha(script.read_bytes()) == manifest["preparation_script_sha256"],
            "Preparation script differs from the manifest")
    require(sha(catalog.read_bytes()) == manifest["input_catalog_sha256"],
            "Source catalog differs from the manifest")
    registry = json.loads((ROOT / "docs/licensing/ASSET_REGISTRY.json").read_text())["apk_assets"]
    rows = manifest["outputs"]
    files = {row["file"] for row in rows}
    require(len(files) == len(rows), "Duplicate runtime filename")
    require(files == {p.name for p in ASSETS.iterdir() if p.is_file()},
            "Unlisted or missing runtime asset")
    policy = (ROOT / "app/src/main/java/io/github/mrcalzon02/reverievr/BreakwaterAudioPolicy.java").read_text()
    mapped = {m + ".wav" for m in re.findall(
        r'(?:effect|control|loop)\((?:\d+|[A-Z_]+),\s*"([a-z_]+)"', policy)}
    require(mapped == files, "Playback mapping and admitted WAV files differ")
    source_keys = {s["key"] for s in manifest["sources"]}
    result = {"files": len(files), "bytes": 0, "loops": 0,
              "largest_decoded_bytes": 0, "clipped_samples": 0,
              "maximum_rms_error_db": 0.0, "apk": str(apk) if apk else None}
    packaged = zipfile.ZipFile(apk) if apk else None
    try:
        for row in rows:
            name = row["file"]
            data = (ASSETS / name).read_bytes()
            require(len(data) == row["bytes"] and sha(data) == row["sha256"], name + ": hash/size mismatch")
            require(data[:4] == b"RIFF" and data[8:16] == b"WAVEfmt " and
                    data[36:40] == b"data", name + ": not canonical PCM WAV")
            require(struct.unpack_from("<I", data, 4)[0] == len(data) - 8 and
                    struct.unpack_from("<I", data, 16)[0] == 16,
                    name + ": malformed WAV chunks")
            require(struct.unpack_from("<HHIIHH", data, 20) == (1, 1, 22050, 44100, 2, 16),
                    name + ": wrong audio format")
            frames = row["frames"]
            require(0 < frames * 2 < 1000000 and len(data) == 44 + frames * 2 and
                    struct.unpack_from("<I", data, 40)[0] == frames * 2,
                    name + ": truncated/oversized decoded sample")
            values = struct.unpack_from("<" + str(frames) + "h", data, 44)
            peak = max(abs(v) for v in values) / 32768
            group = manifest["normalization_groups"][row["group"]]
            require(0 < peak <= 10 ** (group["peak_dbfs"] / 20), name + ": peak ceiling exceeded")
            clipped = sum(abs(v) >= 32767 for v in values)
            require(clipped == 0, name + ": clipped samples")
            rms_db = 20 * math.log10(rms_active(values))
            error = abs(rms_db - group["active_rms_dbfs"])
            require(error < .15, name + ": active RMS outside category tolerance")
            require(set(row["source_keys"]) <= source_keys and bool(row["licenses"]),
                    name + ": source/rights record missing")
            apk_name = "assets/native/breakwater/audio/" + name
            require(registry[apk_name]["sha256"] == row["sha256"], name + ": registry hash mismatch")
            if packaged:
                require(packaged.read(apk_name) == data, name + ": APK bytes differ from source")
                # Audio uses AssetManager.openFd(), which requires an uncompressed entry.
                require(packaged.getinfo(apk_name).compress_type == zipfile.ZIP_STORED,
                        name + ": APK compresses a direct-file-descriptor sound")
            if row["loop"]:
                require(frames == 132300, name + ": environmental loop must be six seconds")
                steps = sorted(abs(a - b) for a, b in zip(values, values[1:]))
                require(abs(values[0] - values[-1]) <= steps[int(.99 * (len(steps) - 1))],
                        name + ": loop seam exceeds normal adjacent-sample range")
                result["loops"] += 1
            else:
                require(values[0] == 0 and values[-1] == 0, name + ": one-shot edges are not faded")
            result["bytes"] += len(data)
            result["largest_decoded_bytes"] = max(result["largest_decoded_bytes"], frames * 2)
            result["maximum_rms_error_db"] = max(result["maximum_rms_error_db"], error)
        require(result["bytes"] == manifest["total_wav_bytes"], "Bank byte total differs")
        result["maximum_rms_error_db"] = round(result["maximum_rms_error_db"], 6)
        return result
    finally:
        if packaged:
            packaged.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apk", type=Path)
    args = parser.parse_args()
    print("BREAKWATER AUDIO ASSETS: PASS " + json.dumps(verify(args.apk), sort_keys=True))
