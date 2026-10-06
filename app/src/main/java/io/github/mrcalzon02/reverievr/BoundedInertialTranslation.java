package io.github.mrcalzon02.reverievr;

final class BoundedInertialTranslation {
    private static final float DEADZONE_METERS_PER_SECOND_SQUARED = 0.08f;
    private static final float INPUT_GAIN = 1.25f;
    private static final float DAMPING_PER_SECOND = 4.25f;
    private static final float SPRING_PER_SECOND_SQUARED = 6.5f;
    private static final float MAX_X_METERS = 0.16f;
    private static final float MAX_Y_METERS = 0.13f;
    private static final float MAX_Z_METERS = 0.14f;

    private float x;
    private float y;
    private float z;
    private float velocityX;
    private float velocityY;
    private float velocityZ;

    void reset() {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
        velocityX = 0.0f;
        velocityY = 0.0f;
        velocityZ = 0.0f;
    }

    void update(
        float accelerationX,
        float accelerationY,
        float accelerationZ,
        float deltaSeconds
    ) {
        if (!Float.isFinite(accelerationX)
            || !Float.isFinite(accelerationY)
            || !Float.isFinite(accelerationZ)
            || !Float.isFinite(deltaSeconds)
            || deltaSeconds <= 0.0f) {
            return;
        }

        float dt =
            clamp(
                deltaSeconds,
                0.0f,
                0.05f
            );

        float ax =
            deadzone(accelerationX)
                * INPUT_GAIN;
        float ay =
            deadzone(accelerationY)
                * INPUT_GAIN;
        float az =
            deadzone(accelerationZ)
                * INPUT_GAIN;

        velocityX +=
            (
                ax
                    - x
                        * SPRING_PER_SECOND_SQUARED
            ) * dt;
        velocityY +=
            (
                ay
                    - y
                        * SPRING_PER_SECOND_SQUARED
            ) * dt;
        velocityZ +=
            (
                az
                    - z
                        * SPRING_PER_SECOND_SQUARED
            ) * dt;

        float damping =
            Math.max(
                0.0f,
                1.0f
                    - DAMPING_PER_SECOND
                        * dt
            );
        velocityX *= damping;
        velocityY *= damping;
        velocityZ *= damping;

        x += velocityX * dt;
        y += velocityY * dt;
        z += velocityZ * dt;

        float clampedX =
            clamp(
                x,
                -MAX_X_METERS,
                MAX_X_METERS
            );
        float clampedY =
            clamp(
                y,
                -MAX_Y_METERS,
                MAX_Y_METERS
            );
        float clampedZ =
            clamp(
                z,
                -MAX_Z_METERS,
                MAX_Z_METERS
            );

        if (clampedX != x) {
            velocityX = 0.0f;
        }
        if (clampedY != y) {
            velocityY = 0.0f;
        }
        if (clampedZ != z) {
            velocityZ = 0.0f;
        }

        x = clampedX;
        y = clampedY;
        z = clampedZ;
    }

    float x() {
        return x;
    }

    float y() {
        return y;
    }

    float z() {
        return z;
    }

    private static float deadzone(
        float value
    ) {
        float magnitude =
            Math.abs(value);
        if (magnitude
            <= DEADZONE_METERS_PER_SECOND_SQUARED) {
            return 0.0f;
        }

        return Math.copySign(
            magnitude
                - DEADZONE_METERS_PER_SECOND_SQUARED,
            value
        );
    }

    private static float clamp(
        float value,
        float low,
        float high
    ) {
        return Math.max(
            low,
            Math.min(
                high,
                value
            )
        );
    }
}
