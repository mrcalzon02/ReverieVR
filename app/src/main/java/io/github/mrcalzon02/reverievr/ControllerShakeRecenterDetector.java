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
    // Initialized only from calm ~1g samples; an instantaneous jerk on
    // startup must still be recognized by the fallback magnitude path.
    private boolean gravityReferenceValid;
    private float gravityX;
    private float gravityY;
    private float gravityZ;

    boolean sample(float accelerationX, float accelerationY,
                   float accelerationZ, long timestampNanos) {
        return sample(accelerationX, accelerationY, accelerationZ,
            timestampNanos, true);
    }

    boolean sample(
        float accelerationX,
        float accelerationY,
        float accelerationZ,
        long timestampNanos,
        boolean accelerationInG
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

        // Daydream reports g; phone-controller emulation reports m/s^2.
        // A shake above 4g must not suddenly switch units and disappear.
        float scale = accelerationInG ? STANDARD_GRAVITY : 1.0f;
        float ax = accelerationX * scale;
        float ay = accelerationY * scale;
        float az = accelerationZ * scale;
        float metersPerSecondSquared = magnitude * scale;

        if (!gravityReferenceValid
            && metersPerSecondSquared >= STANDARD_GRAVITY * 0.5f
            && metersPerSecondSquared <= STANDARD_GRAVITY * 1.5f) {
            gravityX = ax;
            gravityY = ay;
            gravityZ = az;
            gravityReferenceValid = true;
        }

        float impulseX = gravityReferenceValid ? ax - gravityX : ax;
        float impulseY = gravityReferenceValid ? ay - gravityY : ay;
        float impulseZ = gravityReferenceValid ? az - gravityZ : az;
        float deviation = gravityReferenceValid
            ? (float) Math.sqrt(impulseX * impulseX
                + impulseY * impulseY + impulseZ * impulseZ)
            : Math.max(0.0f, metersPerSecondSquared - STANDARD_GRAVITY);

        if (deviation < IMPULSE_DEVIATION) {
            // Track changing rest orientation slowly, never learn a shake.
            if (gravityReferenceValid && deviation < IMPULSE_DEVIATION * 0.45f) {
                gravityX += (ax - gravityX) * 0.045f;
                gravityY += (ay - gravityY) * 0.045f;
                gravityZ += (az - gravityZ) * 0.045f;
            }
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
                impulseX, impulseY, impulseZ, timestampNanos
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
        float dot = firstImpulseX * impulseX
            + firstImpulseY * impulseY
            + firstImpulseZ * impulseZ;
        float firstMagnitude = (float) Math.sqrt(
            firstImpulseX * firstImpulseX
                + firstImpulseY * firstImpulseY
                + firstImpulseZ * firstImpulseZ
        );
        float impulseMagnitude = (float) Math.sqrt(
            impulseX * impulseX + impulseY * impulseY
                + impulseZ * impulseZ);
        if (dot > -0.25f * firstMagnitude * impulseMagnitude) {
            recordFirstImpulse(
                impulseX, impulseY, impulseZ, timestampNanos
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
        gravityReferenceValid = false;
        gravityX = gravityY = gravityZ = 0.0f;
    }
}
