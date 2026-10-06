package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class BoundedInertialTranslationTest {
    @Test
    public void resetClearsPosition() {
        BoundedInertialTranslation translation =
            new BoundedInertialTranslation();

        translation.update(
            2.0f,
            1.0f,
            -1.5f,
            0.05f
        );
        translation.reset();

        assertEquals(
            0.0f,
            translation.x(),
            0.000001f
        );
        assertEquals(
            0.0f,
            translation.y(),
            0.000001f
        );
        assertEquals(
            0.0f,
            translation.z(),
            0.000001f
        );
    }

    @Test
    public void accelerationProducesBoundedMotion() {
        BoundedInertialTranslation translation =
            new BoundedInertialTranslation();

        for (int index = 0;
             index < 240;
             index++) {
            translation.update(
                25.0f,
                -25.0f,
                25.0f,
                0.05f
            );
        }

        assertTrue(
            Math.abs(translation.x())
                <= 0.160001f
        );
        assertTrue(
            Math.abs(translation.y())
                <= 0.130001f
        );
        assertTrue(
            Math.abs(translation.z())
                <= 0.140001f
        );
    }

    @Test
    public void zeroInputSettlesBackTowardNeutral() {
        BoundedInertialTranslation translation =
            new BoundedInertialTranslation();

        for (int index = 0;
             index < 20;
             index++) {
            translation.update(
                3.0f,
                0.0f,
                0.0f,
                0.03f
            );
        }

        float displaced =
            Math.abs(
                translation.x()
            );
        assertTrue(displaced > 0.001f);

        for (int index = 0;
             index < 240;
             index++) {
            translation.update(
                0.0f,
                0.0f,
                0.0f,
                0.03f
            );
        }

        assertTrue(
            Math.abs(translation.x())
                < displaced
        );
        assertTrue(
            Math.abs(translation.x())
                < 0.01f
        );
    }

    @Test
    public void accelerationDeadzoneRejectsTinyNoise() {
        BoundedInertialTranslation translation =
            new BoundedInertialTranslation();

        for (int index = 0;
             index < 120;
             index++) {
            translation.update(
                0.02f,
                -0.03f,
                0.01f,
                0.016f
            );
        }

        assertEquals(
            0.0f,
            translation.x(),
            0.000001f
        );
        assertEquals(
            0.0f,
            translation.y(),
            0.000001f
        );
        assertEquals(
            0.0f,
            translation.z(),
            0.000001f
        );
    }
}
