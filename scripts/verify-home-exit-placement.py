#!/usr/bin/env python3
"""Static Home menu regression: preserve on-screen location and action parity.

Run: python3 scripts/verify-home-exit-placement.py

This checks source wiring and does not substitute for Android APK compilation
or a headset/controller/gaze acceptance run.
"""
from pathlib import Path
import re

source = (Path(__file__).resolve().parents[1]
          / "app/src/main/java/io/github/mrcalzon02/reverievr/VrShellRenderer.java"
          ).read_text(encoding="utf-8")


def section(first, last):
    start = source.index(first)
    return source[start:source.index(last, start)]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


rects = section("private static final int[][] HOME_BUTTONS",
                "private static final int[][] PHONE_PANEL_BUTTONS")
rects = rects.split("new int[][] {", 1)[1].split("};", 1)[0]
coords = [tuple(map(int, row)) for row in re.findall(
    r"\{(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\}", rects
)]
require(len(coords) == 27, "Home button count/index layout changed")
require(coords[5][0] < 230 and coords[26][0] >= 780,
        "Music must remain left, Exit to Phone must remain right")
require(coords[26][1] >= coords[12][3],
        "Exit to Phone overlaps the right-hand options")

draw = section("private void drawHome(", "private void drawHomePanelBackground(")
labels = section("String[] leftLabels =", "paint.setFakeBoldText(true);")
require('"Setup / Comfort",\n                "Music Library"' in labels,
        "Left-hand Music Library label missing")
require('new String[] {"Exit to Phone"}, 26, 1' in labels,
        "Right-hand Exit to Phone button missing")
require('new String[] {"Classic Games"}, 24, 1' in labels
        and 'new String[] {"Emulator Modules"}, 25, 1' in labels,
        "Existing left launch entries changed")
require('"QUICK OPTIONS"' in draw
        and '"Check Updates"' in draw
        and '"Android Bluetooth"' in draw,
        "Existing right quick options changed")

activation = section("private void activateHoveredButton()",
                     "private void handleOrientationMenuSelection(")
home = activation.split("if (mode == MODE_HOME) {", 1)[1]
selection = home.split("switch (hoveredButton) {", 1)[1].split(
    "} else if (mode == MODE_MUSIC_LIBRARY)", 1)[0]
music_case = selection.split("case 5:", 1)[1].split("case 26:", 1)[0]
exit_case = selection.split("case 26:", 1)[1].split("case 25:", 1)[0]
require("host.getNativeMusicTrackNames()" in music_case
        and "mode = MODE_MUSIC_LIBRARY;" in music_case,
        "Left Music Library is not an executable launcher")
require("host.onExitToPhoneRequested();" in exit_case
        and "return;" in exit_case,
        "Right Exit to Phone does not invoke the existing Stage A handoff")
require("host.onExitToPhoneRequested();" not in music_case,
        "Music Library incorrectly exits to phone")

help_text = section("private String homeHelpText(",
                    "private boolean hasResumeTarget()")
require("licensed native music library" in help_text.split(
    "case 5:", 1)[1].split("case 6:", 1)[0],
    "Music Library help text does not match the left button")
require("return to the phone interface" in help_text.split(
    "case 26:", 1)[1].split("default:", 1)[0],
    "Exit to Phone help text does not match the right button")
require('return phone' not in music_case.lower(),
        "Left launcher retains an incorrect phone-exit action")
print("PASS: Home Exit to Phone is right-side, Music Library left-side, both wired")
