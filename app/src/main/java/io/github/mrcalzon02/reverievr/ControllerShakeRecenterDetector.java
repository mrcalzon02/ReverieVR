package io.github.mrcalzon02.reverievr;

final class ControllerShakeRecenterDetector {
    private static final float STANDARD_GRAVITY = 9.80665f;
    private static final float IMPULSE_DEVIATION = 9.5f;
    private static final long MIN_IMPULSE_GAP_NANOS = 65000000L;
    private static final long MAX_IMPULSE_GAP_NANOS = 350000000L;
    private static final long COOLDOWN_NANOS = 3000000000L;

    private long firstImpulseAtNanos;
    private long lastTriggerAtNanos;
    private float firstImpulseX;
    private float firstImpulseY;
    private float firstImpulseZ;

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
            recordFirstImpulse(
                accelerationX, accelerationY, accelerationZ, timestampNanos
            );
            return false;
        }

        long gap =
            timestampNanos
                - firstImpulseAtNanos;
        if (gap < MIN_IMPULSE_GAP_NANOS) {
            return false;
        }

        // Two unrelated impacts in the same direction must not recenter
        // the controller. A deliberate shake has an opposing impulse.
        float dot = firstImpulseX * accelerationX
            + firstImpulseY * accelerationY
            + firstImpulseZ * accelerationZ;
        float firstMagnitude = (float) Math.sqrt(
            firstImpulseX * firstImpulseX
                + firstImpulseY * firstImpulseY
                + firstImpulseZ * firstImpulseZ
        );
        if (dot > -0.25f * firstMagnitude * magnitude) {
            recordFirstImpulse(
                accelerationX, accelerationY, accelerationZ, timestampNanos
            );
            return false;
        }

        firstImpulseAtNanos = 0L;
        lastTriggerAtNanos = timestampNanos;
        return true;
    }

    private void recordFirstImpulse(
        float x, float y, float z, long atNanos
    ) {
        firstImpulseAtNanos = atNanos;
        firstImpulseX = x;
        firstImpulseY = y;
        firstImpulseZ = z;
    }

    void cancelPendingImpulse() {
        firstImpulseAtNanos = 0L;
        firstImpulseX = 0.0f;
        firstImpulseY = 0.0f;
        firstImpulseZ = 0.0f;
    }

    void reset() {
        cancelPendingImpulse();
        lastTriggerAtNanos = 0L;
    }
}
