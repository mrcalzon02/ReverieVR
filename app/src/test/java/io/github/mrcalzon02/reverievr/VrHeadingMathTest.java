package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class VrHeadingMathTest {
    private static final float EPSILON = 0.0001f;

    @Test
    public void shakeCalibrationPointsControllerAtHeadsetYaw() {
        float controllerYaw = (float) Math.toRadians(75);
        float headsetYaw = (float) Math.toRadians(-20);
        float calibration = VrHeadingMath.controllerCalibrationForHeadset(
            controllerYaw, headsetYaw
        );
        assertEquals(headsetYaw,
            VrHeadingMath.wrapAngle(controllerYaw - calibration), EPSILON);
        assertTrue(Float.isNaN(
            VrHeadingMath.controllerCalibrationForHeadset(
                Float.NaN, headsetYaw)));
    }

    @Test
    public void shakeCalibrationWrapsAcrossRearHeading() {
        float controllerYaw = (float) Math.toRadians(179);
        float headsetYaw = (float) Math.toRadians(-179);
        float calibration = VrHeadingMath.controllerCalibrationForHeadset(
            controllerYaw, headsetYaw
        );
        assertEquals((float) Math.toRadians(-2), calibration, EPSILON);
    }

    @Test
    public void forwardMapsToZeroYaw() {
        assertEquals(
            0.0f,
            VrHeadingMath.yawFromForward(
                0.0f,
                -1.0f
            ),
            EPSILON
        );
    }

    @Test
    public void rightwardForwardMapsToNegativeQuarterTurn() {
        assertEquals(
            (float) (-Math.PI * 0.5),
            VrHeadingMath.yawFromForward(
                1.0f,
                0.0f
            ),
            EPSILON
        );
    }

    @Test
    public void leftwardForwardMapsToPositiveQuarterTurn() {
        assertEquals(
            (float) (Math.PI * 0.5),
            VrHeadingMath.yawFromForward(
                -1.0f,
                0.0f
            ),
            EPSILON
        );
    }

    @Test
    public void backwardMapsToHalfTurn() {
        float yaw =
            VrHeadingMath.yawFromForward(
                0.0f,
                1.0f
            );

        assertTrue(
            Math.abs(
                Math.abs(yaw)
                    - Math.PI
            ) < EPSILON
        );
    }

    @Test
    public void nearVerticalForwardHasNoStableHeading() {
        assertTrue(
            Float.isNaN(
                VrHeadingMath.yawFromForward(
                    0.001f,
                    0.001f
                )
            )
        );
    }

    @Test
    public void relativeYawWrapsAcrossPiBoundary() {
        float relative =
            VrHeadingMath.relativeYaw(
                (float) Math.toRadians(-179.0),
                (float) Math.toRadians(179.0)
            );

        assertEquals(
            (float) Math.toRadians(2.0),
            relative,
            0.001f
        );
    }
}
