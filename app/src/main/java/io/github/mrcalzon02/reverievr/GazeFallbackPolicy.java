package io.github.mrcalzon02.reverievr;

/**
 * Prevent short controller tracking/connection gaps from activating the
 * headset's emergency gaze pointer. This is a visual/input-source handoff
 * policy, never permission to use stale controller poses for input.
 */
final class GazeFallbackPolicy {
    static final long CONTROLLER_LOSS_GRACE_NANOS = 4_000_000_000L;

    private long lastUsableControllerAtNanos = -1L;

    boolean shouldUseGaze(VrPointerMode preference,
                          boolean usableController, long frameNanos) {
        if (usableController) {
            lastUsableControllerAtNanos = frameNanos;
        }
        if (preference == VrPointerMode.GAZE) {
            return true; // Explicit user choice bypasses automatic handoff.
        }
        if (usableController) {
            return false;
        }
        // On a genuinely controller-free startup, emergency gaze works now.
        if (lastUsableControllerAtNanos < 0L) {
            return true;
        }
        if (frameNanos < lastUsableControllerAtNanos) {
            // Treat a non-monotonic frame timestamp as a new grace window.
            lastUsableControllerAtNanos = frameNanos;
            return false;
        }
        return frameNanos - lastUsableControllerAtNanos
            >= CONTROLLER_LOSS_GRACE_NANOS;
    }
}
