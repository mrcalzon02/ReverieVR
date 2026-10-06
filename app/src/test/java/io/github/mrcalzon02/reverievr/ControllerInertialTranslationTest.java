package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class ControllerInertialTranslationTest {
    @Test
    public void forwardAndBackwardMotionChangesDepth() {
        ControllerInertialTranslation translation =
            new ControllerInertialTranslation();

        for (int index = 0; index < 12; index++) {
            translation.update(
                0.0f,
                0.0f,
                -3.5f,
                0.02f
            );
        }

        float pushedAway = translation.z();
        assertTrue(pushedAway < -0.005f);

        for (int index = 0; index < 24; index++) {
            translation.update(
                0.0f,
                0.0f,
                4.5f,
                0.02f
            );
        }

        assertTrue(translation.z() > pushedAway);
    }

    @Test
    public void movementIsBoundedToControllerEnvelope() {
        ControllerInertialTranslation translation =
            new ControllerInertialTranslation();

        for (int index = 0; index < 300; index++) {
            translation.update(
                30.0f,
                -30.0f,
                30.0f,
                0.05f
            );
        }

        assertTrue(Math.abs(translation.x()) <= 0.450001f);
        assertTrue(Math.abs(translation.y()) <= 0.350001f);
        assertTrue(Math.abs(translation.z()) <= 0.650001f);
    }

    @Test
    public void tinyAccelerationNoiseDoesNotMoveHandset() {
        ControllerInertialTranslation translation =
            new ControllerInertialTranslation();

        for (int index = 0; index < 120; index++) {
            translation.update(
                0.04f,
                -0.05f,
                0.03f,
                0.016f
            );
        }

        assertEquals(0.0f, translation.x(), 0.000001f);
        assertEquals(0.0f, translation.y(), 0.000001f);
        assertEquals(0.0f, translation.z(), 0.000001f);
    }

    @Test
    public void resetReturnsHandsetToReferencePosition() {
        ControllerInertialTranslation translation =
            new ControllerInertialTranslation();

        translation.update(3.0f, 2.0f, -4.0f, 0.05f);
        translation.reset();

        assertEquals(0.0f, translation.x(), 0.000001f);
        assertEquals(0.0f, translation.y(), 0.000001f);
        assertEquals(0.0f, translation.z(), 0.000001f);
    }
}
