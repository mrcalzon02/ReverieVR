package io.github.mrcalzon02.reverievr;

/**
 * Head-gaze recovery must never flash on just because a connected controller
 * misses some BLE pose notifications. Input pose freshness is independently
 * enforced at the renderer's 750 ms live-pose gate: this class never makes an
 * old pose usable for clicks or movement.
 */
final class GazeFallbackPolicy {
    static final long CONTROLLER_LOSS_GRACE_NANOS = 4_000_000_000L;
    // A READY controller may stall its pose stream without losing its BLE link.
    // Reserve gaze for a sustained outage, not a few lost notifications.
    static final long CONNECTED_POSE_STALL_NANOS = 30_000_000_000L;

    private long lastUsableControllerAtNanos = -1L;
    private long linkedSinceNanos = -1L;
    private long lastFrameNanos = -1L;

    boolean shouldUseGaze(VrPointerMode preference,
                          boolean usableController, boolean controllerLinked,
                          long frameNanos) {
        // A renderer restart or monotonic clock anomaly starts a new grace
        // window; negative durations cannot authorize a fallback.
        if (lastFrameNanos >= 0L && frameNanos < lastFrameNanos) {
            if (lastUsableControllerAtNanos >= 0L)
                lastUsableControllerAtNanos = frameNanos;
            linkedSinceNanos = controllerLinked ? frameNanos : -1L;
        }
        lastFrameNanos = frameNanos;
        if (controllerLinked) {
            if (linkedSinceNanos < 0L) linkedSinceNanos = frameNanos;
        } else {
            linkedSinceNanos = -1L;
        }
        if (usableController) lastUsableControllerAtNanos = frameNanos;

        // Explicit gaze selection always wins, even with an active controller.
        if (preference == VrPointerMode.GAZE) return true;
        if (usableController) return false;

        // No detected controller: headset-only startup remains immediate.
        // A READY transport without any live pose gets a bounded acquisition
        // window; otherwise a late first packet could briefly flash the ray.
        long anchor = lastUsableControllerAtNanos;
        if (controllerLinked && linkedSinceNanos > anchor)
            anchor = linkedSinceNanos;
        if (anchor < 0L) return true;
        if (frameNanos < anchor) return false;
        return frameNanos - anchor >= (controllerLinked
            ? CONNECTED_POSE_STALL_NANOS : CONTROLLER_LOSS_GRACE_NANOS);
    }
}
