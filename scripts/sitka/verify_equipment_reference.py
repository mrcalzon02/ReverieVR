#!/usr/bin/env python3
"""Pure source-only verification of the approved Sitka concept planning index."""
from __future__ import annotations
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
path = ROOT / "docs/project/sitka/equipment/asset_catalog.json"
catalog = json.loads(path.read_text(encoding="utf-8"))
groups = catalog["groups"]
expected = {
    "rod": 13, "reel": 11, "line": 11, "lure": 9,
    "beach_kit": 9, "skiff_hull": 6, "helm_outboard": 9,
    "catch_storage": 9, "fishfinder_nav": 8,
}
assert {g["id"]: len(g["tiers"]) for g in groups} == expected
assert sum(expected.values()) == 85
assert sum(expected[k]-1 for k in list(expected)[:5]) == 48
identifiers = []
filenames = set()
for group in groups:
    ref = group["source_image"]
    assert ref["filename"] not in filenames
    filenames.add(ref["filename"])
    assert ref["filename"].endswith(".png")
    assert ref["width"] == 1448 and ref["height"] == 1086
    assert ref["bytes"] > 2000000
    assert re.fullmatch(r"[0-9a-f]{64}", ref["sha256"])
    assert ref["google_drive_id"] in ref["google_drive_url"]
    assert group["model_component_brief"]
    for index,tier in enumerate(group["tiers"]):
        assert tier["asset_id"] == f"sitka.{group['id']}.{index:02d}"
        assert tier["sheet_tier"] == index + 1
        assert tier["gameplay_level_if_linear"] == index
        assert tier["display_name"] and tier["physical_model_cue"]
        assert tier["mesh_status"] == "not_yet_modeled"
        assert tier["icon_status"] == "not_yet_rendered"
        identifiers.append(tier["asset_id"])
assert len(identifiers) == len(set(identifiers))
cpp = (ROOT/"app/src/main/jni/native/sitka_fishing.h").read_text()
match = re.search(r"kEquipmentMax\s*=\s*\{\{([^}]+)\}\}", cpp)
assert match, "Cannot locate authoritative equipment progression"
actual = [int(x) for x in re.findall(r"(\d+)u", match.group(1))]
assert actual == [expected[k]-1 for k in ("rod","reel","line","lure","beach_kit")], (
    "Gameplay levels diverged from approved equipment reference",
    actual,
)
print("Sitka equipment visual reference: nine sheets, 85 stable IDs, "
      "48 live purchase levels aligned, PASS")
