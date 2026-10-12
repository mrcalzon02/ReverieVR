package io.github.mrcalzon02.reverievr;

final class VrHeadingMath {
    private static final float MIN_HORIZONTAL_LENGTH_SQUARED =
        0.0004f;

    private VrHeadingMath() {
    }

    static float yawFromForward(
        float forwardX,
        float forwardZ
    ) {
        float horizontalLengthSquared =
            forwardX * forwardX
                + forwardZ * forwardZ;
        if (!Float.isFinite(
                horizontalLengthSquared
            )
            || horizontalLengthSquared
                < MIN_HORIZONTAL_LENGTH_SQUARED) {
            return Float.NaN;
        }

        return wrapAngle(
            (float) Math.atan2(
                -forwardX,
                -forwardZ
            )
        );
    }

    /** Yaw directly from the raw world-to-eye basis used by Cardboard. */
    static float yawFromHeadView(float[] rawView) {
        if (rawView == null || rawView.length < 16) return Float.NaN;
        return yawFromForward(-rawView[2], -rawView[10]);
    }

    /** Positive heading cancels negative headset yaw in world-to-eye view. */
    static float viewCorrectionDegrees(float heading) {
        return (float) Math.toDegrees(heading);
    }

    /** Panels are already in shell space; only quick menu follows relative head yaw. */
    static float shellPanelRotationDegrees(boolean quickMenu, float relativeYaw) {
        return quickMenu ? (float) Math.toDegrees(relativeYaw) : 0f;
    }

    /** The yaw correction to rotate a raw controller aim onto headset forward. */
    static float controllerCalibrationForHeadset(float controllerYaw, float headsetYaw) {
        if (!Float.isFinite(controllerYaw) || !Float.isFinite(headsetYaw)) {
            return Float.NaN;
        }
        return wrapAngle(controllerYaw - headsetYaw);
    }

    static float relativeYaw(
        float absoluteYaw,
        float shellYaw
    ) {
        if (!Float.isFinite(absoluteYaw)
            || !Float.isFinite(shellYaw)) {
            return Float.NaN;
        }

        return wrapAngle(
            absoluteYaw - shellYaw
        );
    }

    static float angularDistance(
        float a,
        float b
    ) {
        if (!Float.isFinite(a)
            || !Float.isFinite(b)) {
            return Float.POSITIVE_INFINITY;
        }
        return Math.abs(
            wrapAngle(a - b)
        );
    }

    static float wrapAngle(
        float angle
    ) {
        float wrapped = angle;
        while (wrapped > Math.PI) {
            wrapped -=
                (float) (
                    Math.PI * 2.0
                );
        }
        while (wrapped < -Math.PI) {
            wrapped +=
                (float) (
                    Math.PI * 2.0
                );
        }
        return wrapped;
    }
}
