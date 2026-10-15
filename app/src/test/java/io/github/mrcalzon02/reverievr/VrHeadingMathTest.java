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
    public void deriveHeadingFromRawStereoViewForBothQuarterTurns() {
        float[] m = new float[16];
        m[0] = m[5] = m[10] = m[15] = 1f;
        assertEquals(0f, VrHeadingMath.yawFromHeadView(m), EPSILON);
        m[0] = 0f; m[2] = 1f; m[8] = -1f; m[10] = 0f;
        assertEquals((float)(Math.PI / 2f),
            VrHeadingMath.yawFromHeadView(m), EPSILON);
        m[2] = -1f; m[8] = 1f;
        assertEquals((float)(-Math.PI / 2f),
            VrHeadingMath.yawFromHeadView(m), EPSILON);
        assertTrue(Float.isNaN(VrHeadingMath.yawFromHeadView(new float[3])));
    }

    /** Matrix matches the 3D camera world-to-eye rotation, not Euler. */
    private static float[] cameraView(float yawRight, float pitchUp) {
        float cy=(float)Math.cos(yawRight), sy=(float)Math.sin(yawRight);
        float cp=(float)Math.cos(pitchUp), sp=(float)Math.sin(pitchUp);
        return new float[] {
            cy, -sp*sy, -cp*sy, 0f,
            0f, cp, -sp, 0f,
            sy, sp*cy, cp*cy, 0f,
            0f, 0f, 0f, 1f
        };
    }

    @Test
    public void guestAxesFollow3dCameraIndependentlyForAllPitchDirections() {
        for (int pitch : new int[] {-90,-89,-60,-30,0,30,60,89,90}) {
            for (int yaw : new int[] {-90,-45,0,45,90}) {
                float[] v=cameraView((float)Math.toRadians(yaw),
                    (float)Math.toRadians(pitch));
                assertEquals("yaw at pitch " + pitch, (float)Math.toRadians(yaw),
                    VrHeadingMath.guestYawRightFromHeadView(v), 0.0002f);
                assertEquals("pitch at yaw " + yaw, (float)Math.toRadians(pitch),
                    VrHeadingMath.guestPitchUpFromHeadView(v), 0.0002f);
            }
        }
    }

    @Test
    public void guestViewRejectsBadMatricesInsteadOfSpikingMouse() {
        assertTrue(Float.isNaN(VrHeadingMath.guestYawRightFromHeadView(null)));
        assertTrue(Float.isNaN(VrHeadingMath.guestPitchUpFromHeadView(new float[2])));
        float[] invalid=cameraView(0f,0f);
        invalid[8]=Float.POSITIVE_INFINITY;
        assertTrue(Float.isNaN(VrHeadingMath.guestYawRightFromHeadView(invalid)));
        invalid=cameraView(0f,0f);
        invalid[6]=Float.NaN;
        assertTrue(Float.isNaN(VrHeadingMath.guestPitchUpFromHeadView(invalid)));
    }

    @Test
    public void startupHeadingCancelsViewYawAndKeepsShellPanelsCentered() {
        float right = (float) (Math.PI * 0.5);
        assertEquals(90f, VrHeadingMath.viewCorrectionDegrees(right), EPSILON);
        assertEquals(-90f, VrHeadingMath.viewCorrectionDegrees(-right), EPSILON);
        assertEquals(0f, VrHeadingMath.shellPanelRotationDegrees(false, right), EPSILON);
        assertEquals(90f, VrHeadingMath.shellPanelRotationDegrees(true, right), EPSILON);
    }

    @Test
    public void pitchRemainsPitchAfterRecenterAtBothQuarterTurns() {
        // Cardboard world-to-eye = pitch * inverse(world heading).
        // Recenter MUST apply its yaw on the right/world side. Otherwise
        // looking up/down couples into roll when initially facing sideways.
        for (float headingSign : new float[] {-1f, 1f}) {
            float[] recenterYaw = {
                0f, 0f, -headingSign, 0f,
                0f, 1f, 0f, 0f,
                headingSign, 0f, 0f, 0f,
                0f, 0f, 0f, 1f
            };
            for (float pitchDegrees : new float[] {-60f, -30f, 0f, 30f, 60f}) {
                float pitchRadians = (float) Math.toRadians(pitchDegrees);
                float cosine = (float) Math.cos(pitchRadians);
                float sine = (float) Math.sin(pitchRadians);
                float[] rawHeadView = {
                    0f, -sine * headingSign, cosine * headingSign, 0f,
                    0f, cosine, sine, 0f,
                    -headingSign, 0f, 0f, 0f,
                    0f, 0f, 0f, 1f
                };
                float[] corrected = new float[16];
                VrHeadingMath.applyWorldYawToHeadView(
                    corrected, rawHeadView, recenterYaw);
                // An unrolled view keeps world up on camera X=0.
                assertEquals(0f, corrected[4], EPSILON);
                assertEquals(1f, corrected[0], EPSILON);
                assertEquals(0f, corrected[8], EPSILON);
                assertEquals(cosine, corrected[5], EPSILON);
                assertEquals(sine, corrected[6], EPSILON);
                assertEquals(-sine, corrected[9], EPSILON);
                assertEquals(cosine, corrected[10], EPSILON);
            }
        }
    }

    @Test
    public void noRecenterPreservesActualHeadRollAndTranslation() {
        float c = (float) Math.cos(Math.PI / 6.0);
        float s = (float) Math.sin(Math.PI / 6.0);
        float[] rolled = {
            c, s, 0f, 0f,
            -s, c, 0f, 0f,
            0f, 0f, 1f, 0f,
            2f, -3f, 4f, 1f
        };
        float[] identityYaw = {
            1f, 0f, 0f, 0f,
            0f, 1f, 0f, 0f,
            0f, 0f, 1f, 0f,
            0f, 0f, 0f, 1f
        };
        float[] corrected = new float[16];
        VrHeadingMath.applyWorldYawToHeadView(
            corrected, rolled, identityYaw);
        for (int index = 0; index < 16; index++) {
            assertEquals(rolled[index], corrected[index], EPSILON);
        }
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
