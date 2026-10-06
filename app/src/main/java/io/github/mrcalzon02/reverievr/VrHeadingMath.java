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
