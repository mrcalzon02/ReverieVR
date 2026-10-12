package io.github.mrcalzon02.reverievr;

/**
 * Separate the safety-critical live tracking gate from cosmetic continuity.
 *
 * A short BLE notification interruption or long GPU frame must not make the
 * headset controller blink in/out. An old quaternion is never returned as
 * fresh input: the last handset mesh may remain still while its interaction
 * ray is disabled. Genuine disconnects hide immediately.
 */
final class ControllerPoseVisibility {
    static final long LIVE_POSE_NANOS = 750_000_000L;
    static final long MODEL_HOLD_NANOS = 2_250_000_000L;

    private ControllerPoseVisibility() {}

    static boolean live(boolean connected, boolean poseValid,
                        long poseAtNanos, long nowNanos) {
        if (!connected || !poseValid || poseAtNanos <= 0L) return false;
        long age = nowNanos - poseAtNanos;
        return age >= 0L && age <= LIVE_POSE_NANOS;
    }

    static boolean holdModel(boolean connected, boolean poseValid,
                             long poseAtNanos, long nowNanos) {
        if (!connected || !poseValid || poseAtNanos <= 0L) return false;
        long age = nowNanos - poseAtNanos;
        return age > LIVE_POSE_NANOS && age <= MODEL_HOLD_NANOS;
    }
}
