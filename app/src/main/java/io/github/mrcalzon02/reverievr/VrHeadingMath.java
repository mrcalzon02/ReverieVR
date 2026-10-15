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

    /**
     * Guest-control axes taken from the SAME world-to-eye head matrix as
     * the working 3D camera. Cardboard Euler yaw/pitch are deliberately not
     * used: Euler decomposition near vertical gaze can transfer motion
     * between axes. The camera right basis retains its horizontal heading
     * even when forward points straight up or down.
     *
     * Positive yaw means looking RIGHT; positive pitch means looking UP.
     * The DOS mouse then uses +X for right and -Y for up.
     * These helpers never alter the actual headset view/projection.
     */
    static float guestYawRightFromHeadView(float[] view) {
        if (view == null || view.length < 16) return Float.NaN;
        final float rightX = view[0];
        final float rightZ = view[8];
        final float horizontalLength = rightX*rightX + rightZ*rightZ;
        if (!Float.isFinite(horizontalLength) || horizontalLength < 0.0004f)
            return Float.NaN;
        return (float) Math.atan2(rightZ, rightX);
    }

    static float guestPitchUpFromHeadView(float[] view) {
        if (view == null || view.length < 16 || !Float.isFinite(view[6]))
            return Float.NaN;
        return (float) Math.asin(Math.max(-1f, Math.min(1f, -view[6])));
    }

    /** Positive heading cancels negative headset yaw in world-to-eye view. */
    /**
     * Apply recenter yaw on the world side of a Cardboard world-to-eye view.
     * Multiplying yaw * headView instead rotates around the head's local Y:
     * at nonzero headings, ordinary up/down pitch then becomes camera roll.
     * This computes headView * yaw without allocations or touching IPD.
     */
    static void applyWorldYawToHeadView(
        float[] output, float[] headView, float[] yawMatrix
    ) {
        float cosine = yawMatrix[0];
        float sine = yawMatrix[8];
        for (int row = 0; row < 4; row++) {
            float x = headView[row];
            float z = headView[8 + row];
            output[row] = x * cosine - z * sine;
            output[4 + row] = headView[4 + row];
            output[8 + row] = x * sine + z * cosine;
            output[12 + row] = headView[12 + row];
        }
    }

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
