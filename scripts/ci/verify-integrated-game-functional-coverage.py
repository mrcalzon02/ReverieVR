#!/usr/bin/env python3
"""Ensure no registered ReverieVR native game is silently omitted by CI.

Each game receives the installed Android 10 15-game JNI/GLES2 boot/capture
gate (DownloadedNativeGameAssetsInstrumentedTest). Host functional suites
exercise engine groups; Test Chamber has Android runtime/companion tests.
No screenshots, pixel matches or cosmetic geometry tests block this gate.
"""
import json
from pathlib import Path

root = Path(__file__).resolve().parents[2]
registered = json.loads((root / "modules/game-identity-matrix.json").read_text())["games"]
ids = [item["id"] for item in registered]
assert len(ids) == len(set(ids)), "Duplicate native module game id"
groups = {
    "Breakwater campaign, combat and input": {"breakwater-battery"},
    "Red Ledger trade, faction and simulation": {"between-deliveries-red-ledger"},
    "Gumball Armada gameplay and route controls": {"gumball-armada"},
    "Omega Front campaign and tactics": {"omega-front"},
    "Lantern Desk reconnaissance mission cycle": {"ministry-lantern-desk"},
    "Iron Sight patrol and debrief cycle": {"iron-sight-forward-detachment"},
    "Classic eight rules, AI, controls and Pong dynamics": {
        "classic-chess", "classic-go", "classic-checkers", "classic-pong",
        "classic-snake", "classic-reversi", "classic-breakout",
        "classic-chutes-ladders",
    },
    "Test Chamber installed companion and real GLES2 frame": {
        "procedural-test-chamber",
    },
}
covered = [item for games in groups.values() for item in games]
assert len(covered) == len(set(covered)), "A game is assigned to multiple suites"
missing = set(ids) - set(covered)
unknown = set(covered) - set(ids)
if missing or unknown:
    raise SystemExit(f"CI coverage mismatch: missing={sorted(missing)} unknown={sorted(unknown)}")
assert len(ids) == 15, "Native game registry changed; update verification contract"
for group, games in groups.items():
    print(f"FUNCTIONAL COVERAGE ({len(games)}): {group} => {', '.join(sorted(games))}")
print(f"PASS: all {len(ids)} actual registered native games have an assigned functional/Android runtime gate")
