package io.github.mrcalzon02.reverievr;

final class ControllerShakeRecenterDetector {
    private static final float STANDARD_GRAVITY = 9.80665f;
    private static final float IMPULSE_DEVIATION = 6.25f;
    private static final long MIN_IMPULSE_GAP_NANOS = 40000000L;
    private static final long MAX_IMPULSE_GAP_NANOS = 500000000L;
    private static final long COOLDOWN_NANOS = 1200000000L;

    private long firstImpulseAtNanos;
    private long lastTriggerAtNanos;

    boolean sample(
        float accelerationX,
        float accelerationY,
        float accelerationZ,
        long timestampNanos
    ) {
        if (!Float.isFinite(accelerationX)
            || !Float.isFinite(accelerationY)
            || !Float.isFinite(accelerationZ)
            || timestampNanos <= 0L) {
            return false;
        }

        float magnitude =
            (float) Math.sqrt(
                accelerationX * accelerationX
                    + accelerationY * accelerationY
                    + accelerationZ * accelerationZ
            );
        if (!Float.isFinite(magnitude)) {
            return false;
        }

        float metersPerSecondSquared =
            magnitude < 4.0f
                ? magnitude * STANDARD_GRAVITY
                : magnitude;
        float deviation =
            Math.abs(
                metersPerSecondSquared
                    - STANDARD_GRAVITY
            );

        if (deviation < IMPULSE_DEVIATION) {
            return false;
        }

        if (lastTriggerAtNanos > 0L
            && timestampNanos - lastTriggerAtNanos
                < COOLDOWN_NANOS) {
            return false;
        }

        if (firstImpulseAtNanos <= 0L
            || timestampNanos - firstImpulseAtNanos
                > MAX_IMPULSE_GAP_NANOS) {
            firstImpulseAtNanos = timestampNanos;
            return false;
        }

        long gap =
            timestampNanos
                - firstImpulseAtNanos;
        if (gap < MIN_IMPULSE_GAP_NANOS) {
            return false;
        }

        firstImpulseAtNanos = 0L;
        lastTriggerAtNanos = timestampNanos;
        return true;
    }

    void reset() {
        firstImpulseAtNanos = 0L;
        lastTriggerAtNanos = 0L;
    }
}
