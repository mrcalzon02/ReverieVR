#!/usr/bin/env python3
"""Static regression guard for the Stage B native-launch/frame handoff.

Run from any working directory:
    python3 scripts/verify-native-launch-handoff.py

The first eye pass can occur in the frame that selects a module, *before*
onNewFrame has had a chance to initialize its GL surface. That pending state
must never be mistaken for a guest render failure.
"""
from pathlib import Path
import re
import sys

source = (Path(__file__).resolve().parents[1]
          / "app/src/main/java/io/github/mrcalzon02/reverievr/VrShellRenderer.java"
         ).read_text(encoding="utf-8")


def require(ok: bool, why: str) -> None:
    if not ok:
        raise AssertionError(why)


new_frame = source.index("private void onNewFrameInternal(")
native_draw_method = source.index("private void onDrawEyeInternal(")
require(new_frame < native_draw_method,
        "Native GL setup must be driven by the frame lifecycle")
frame_code = source[new_frame:native_draw_method]
require("nativeModuleRuntime.onSurfaceCreated()" in frame_code,
        "Native GL surface initialization is missing from onNewFrame")
require("noteNativeModuleFailure(" in frame_code
        and "host.onNativeModuleStopRequested()" in frame_code,
        "True GL initialization failures must remain fatal and logged")

# In either regular play or with the universal Quick Menu open, an
# already-stopped guest must still release its hosted input/music lease.
# Without this callback the shell returns to Home with music disabled.
inactive_guest_exit = re.compile(
    r"if \(nativeModuleRuntime == null\s*"
    r"\|\| !nativeModuleRuntime\.isRunning\(\)\) \{\s*"
    r"nativeSurfaceReady = false;\s*"
    r"(?:closeOrientationMenu\(\);\s*)?"
    r"(?:\/\/[^\n]*\n\s*)*"
    r"host\.onNativeModuleStopRequested\(\);\s*"
    r"mode = MODE_HOME;"
)
require(len(inactive_guest_exit.findall(frame_code)) == 2,
        "Both inactive native guest paths must restore Home music/profile")

native_draw = source.index("if (mode == MODE_NATIVE) {", native_draw_method)
first_native_eye = source.index("System.arraycopy(", native_draw)
render_eye = source.index("nativeModuleRuntime.renderEye(", native_draw)
failure = source.index("noteNativeModuleFailure(", render_eye)
pending = source.index("if (!nativeSurfaceReady) {", native_draw)
require(native_draw < pending < first_native_eye < render_eye < failure,
        "Pending GL surface must be handled before the first render attempt")
pending_branch = source[pending:first_native_eye]
require("drawPointerOverlay(eye, eyeCorrection);" in pending_branch
        and "return;" in pending_branch,
        "Pending native surface must defer the eye pass, not fail/stop guest")
require("noteNativeModuleFailure(" not in pending_branch
        and "onNativeModuleStopRequested()" not in pending_branch,
        "Pending surface branch must not report a false guest failure")
render_branch = source[render_eye:failure + 400]
require("if (!rendered)" in render_branch
        and "host.onNativeModuleStopRequested()" in render_branch,
        "A genuine render failure must continue to stop the guest")

print("PASS native-launch handoff: pending first eye defers; fatal failures stop; both inactive exits restore Home audio")
