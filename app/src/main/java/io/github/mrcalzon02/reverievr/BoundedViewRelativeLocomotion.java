package io.github.mrcalzon02.reverievr;

/**
 * Bounded horizontal headset-relative analog locomotion. No vertical
 * travel, artificial rotation, inertia or physical room-scale tracking.
 */
final class BoundedViewRelativeLocomotion {
    private static final float DEADZONE = 0.28f;
    private static final float SPEED_METERS_PER_SECOND = 0.70f;
    private float x;
    private float z;

    void reset() { x = 0.0f; z = 0.0f; }

    void update(float touchX, float touchY,
                float viewForwardX, float viewForwardZ,
                float deltaSeconds, float limitX, float limitZ) {
        if (!Float.isFinite(touchX) || !Float.isFinite(touchY)
            || !Float.isFinite(viewForwardX)
            || !Float.isFinite(viewForwardZ)
            || !Float.isFinite(deltaSeconds)
            || !Float.isFinite(limitX) || !Float.isFinite(limitZ)
            || deltaSeconds <= 0.0f || limitX <= 0.0f || limitZ <= 0.0f) {
            return;
        }
        float headingLength =
            (float) Math.hypot(viewForwardX, viewForwardZ);
        if (headingLength < 0.01f) {
            return;
        }
        float padX = clamp(touchX, -1.0f, 1.0f);
        float padY = clamp(touchY, -1.0f, 1.0f);
        float magnitude = (float) Math.hypot(padX, padY);
        if (magnitude <= DEADZONE) return;
        float strength = clamp(
            (magnitude - DEADZONE) / (1.0f - DEADZONE), 0.0f, 1.0f
        );
        float strafe = padX / magnitude;
        float forward = -padY / magnitude;
        float headingX = viewForwardX / headingLength;
        float headingZ = viewForwardZ / headingLength;
        float distance = SPEED_METERS_PER_SECOND
            * Math.min(deltaSeconds, 0.05f) * strength;
        x = clamp(
            x + distance * (-headingZ * strafe + headingX * forward),
            -limitX, limitX
        );
        z = clamp(
            z + distance * (headingX * strafe + headingZ * forward),
            -limitZ, limitZ
        );
    }

    float x() { return x; }
    float z() { return z; }

    private static float clamp(float value, float low, float high) {
        return Math.max(low, Math.min(high, value));
    }
}
