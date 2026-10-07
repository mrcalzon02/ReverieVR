package io.github.mrcalzon02.reverievr;

/**
 * Torso-side proxy for a 3DoF controller, not absolute hand tracking.
 * Position follows sustained headset yaw, never headset pitch/roll.
 */
final class ControllerBodyAnchor {
    private static final float TURN_DEADBAND_RADIANS =
        (float) Math.toRadians(10.0);
    private static final float FOLLOW_PER_SECOND = 8.0f;

    private boolean initialized;
    private float bodyYaw;

    void reset() {
        initialized = false;
        bodyYaw = 0.0f;
    }

    void update(float headYaw, float deltaSeconds) {
        if (!Float.isFinite(headYaw)) {
            return;
        }
        if (!initialized) {
            bodyYaw = VrHeadingMath.wrapAngle(headYaw);
            initialized = true;
            return;
        }
        if (!Float.isFinite(deltaSeconds)
            || deltaSeconds <= 0.0f) {
            return;
        }
        float delta = VrHeadingMath.wrapAngle(headYaw - bodyYaw);
        float beyondDeadband =
            Math.abs(delta) - TURN_DEADBAND_RADIANS;
        if (beyondDeadband <= 0.0f) {
            return;
        }
        float dt = Math.min(deltaSeconds, 0.05f);
        float follow =
            1.0f - (float) Math.exp(-FOLLOW_PER_SECOND * dt);
        float step =
            Math.min(beyondDeadband, Math.abs(delta) * follow);
        bodyYaw = VrHeadingMath.wrapAngle(
            bodyYaw + Math.copySign(step, delta)
        );
    }

    void place(
        float headX,
        float headY,
        float headZ,
        float offsetX,
        float offsetY,
        float offsetZ,
        float[] outWorld
    ) {
        if (outWorld == null || outWorld.length < 4) {
            return;
        }
        float sine = (float) Math.sin(bodyYaw);
        float cosine = (float) Math.cos(bodyYaw);
        float worldX =
            headX + cosine * offsetX + sine * offsetZ;
        float worldY = headY + offsetY;
        float worldZ =
            headZ - sine * offsetX + cosine * offsetZ;
        outWorld[0] = worldX;
        outWorld[1] = worldY;
        outWorld[2] = worldZ;
        outWorld[3] = 1.0f;
    }

    float bodyYaw() {
        return bodyYaw;
    }
}
