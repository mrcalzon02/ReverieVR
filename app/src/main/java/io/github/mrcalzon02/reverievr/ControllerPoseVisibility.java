package io.github.mrcalzon02.reverievr;

/**
 * Separate the safety-critical live tracking gate from cosmetic continuity.
 *
 * BLE pose delivery must not control whether a paired, connected physical
 * controller exists visually. A stale quaternion never becomes fresh input:
 * the 3D handset freezes at its last known pose while its interaction ray is
 * disabled. Only a confirmed disconnect, source switch or explicit reset
 * removes the model.
 */
final class ControllerPoseVisibility {
    static final long LIVE_POSE_NANOS = 750_000_000L;

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
        return age > LIVE_POSE_NANOS;
    }
}
