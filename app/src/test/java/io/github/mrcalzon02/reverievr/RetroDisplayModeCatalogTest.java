package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class RetroDisplayModeCatalogTest {
    @Test
    public void mode13hPreservesFourByThreeDisplayIntent() {
        RetroDisplayMode mode =
            RetroDisplayModeCatalog.resolve(320, 200);

        assertEquals(1.6f, mode.sourceAspect(), 0.0001f);
        assertEquals(
            4.0f / 3.0f,
            mode.displayAspect(),
            0.0001f
        );
        assertEquals(
            5.0f / 6.0f,
            mode.pixelWidthToHeightRatio(),
            0.0001f
        );
        assertFalse(mode.squarePixels);
    }

    @Test
    public void squarePixelModesKeepNativeAspect() {
        RetroDisplayMode mode =
            RetroDisplayModeCatalog.resolve(320, 240);

        assertEquals(
            mode.sourceAspect(),
            mode.displayAspect(),
            0.0001f
        );
        assertEquals(
            1.0f,
            mode.pixelWidthToHeightRatio(),
            0.0001f
        );
        assertTrue(mode.squarePixels);
    }

    @Test
    public void unknownModeFallsBackToNativeAspect() {
        RetroDisplayMode mode =
            RetroDisplayModeCatalog.resolve(854, 480);

        assertEquals(
            854.0f / 480.0f,
            mode.displayAspect(),
            0.0001f
        );
        assertTrue(mode.squarePixels);
    }

    @Test
    public void integerScaleDoesNotInventFractionalPixelSteps() {
        assertEquals(
            3,
            RetroDisplayModeCatalog.largestIntegerScale(
                320,
                200,
                1024,
                768
            )
        );
        assertEquals(
            0,
            RetroDisplayModeCatalog.largestIntegerScale(
                640,
                480,
                320,
                240
            )
        );
    }
}
