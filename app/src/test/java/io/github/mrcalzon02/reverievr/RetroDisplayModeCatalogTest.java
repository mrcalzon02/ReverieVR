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
    public void fullscreenDosPreservesVgaFourByThreeInsideEyeInset() {
        float aspect = RetroDisplayModeCatalog.resolve(320, 200)
            .displayAspect();
        // Daydream optical-safe fit: 70% of a 1480x1440 eye.
        int insetWidth = Math.round(
            1480 * RetroDisplayMode.FULLSCREEN_OPTICAL_SCALE);
        int insetHeight = Math.round(
            1440 * RetroDisplayMode.FULLSCREEN_OPTICAL_SCALE);
        assertEquals(1036, insetWidth);
        assertEquals(1008, insetHeight);
        int contentWidth = RetroDisplayMode.fittedWidth(
            insetWidth, insetHeight, aspect);
        int contentHeight = RetroDisplayMode.fittedHeight(
            insetWidth, insetHeight, aspect);
        assertEquals(1036, contentWidth);
        assertEquals(777, contentHeight);
        // Verify generous margins to see every DOS HUD and edge element.
        assertTrue((1480 - contentWidth) / 2 >= 220);
        assertTrue((1440 - contentHeight) / 2 >= 330);
        assertEquals(aspect, contentWidth / (float) contentHeight, 0.002f);
        // The guest image never exceeds the inset or physical eye.
        assertTrue(contentWidth <= insetWidth);
        assertTrue(contentHeight <= insetHeight);
        // A wider eye is height-limited instead; no image is cropped.
        assertEquals(1024, RetroDisplayMode.fittedWidth(1184, 768, aspect));
        assertEquals(768, RetroDisplayMode.fittedHeight(1184, 768, aspect));
    }

    @Test
    public void opticalSafeFitKeepsAllCornersForLandscapeAndPortraitEyes() {
        float[] aspects = {4f / 3f, 5f / 4f, 3f / 2f, 16f / 9f};
        int[][] eyes = {{1480, 1440}, {1024, 768}, {768, 1024}};
        for (int[] eye : eyes) {
            int insetWidth = Math.round(
                eye[0] * RetroDisplayMode.FULLSCREEN_OPTICAL_SCALE);
            int insetHeight = Math.round(
                eye[1] * RetroDisplayMode.FULLSCREEN_OPTICAL_SCALE);
            for (float aspect : aspects) {
                int width = RetroDisplayMode.fittedWidth(
                    insetWidth, insetHeight, aspect);
                int height = RetroDisplayMode.fittedHeight(
                    insetWidth, insetHeight, aspect);
                int x = (eye[0] - width) / 2;
                int y = (eye[1] - height) / 2;
                assertTrue(width > 0 && height > 0);
                assertTrue(x > 0 && y > 0);
                assertTrue(x + width <= eye[0]);
                assertTrue(y + height <= eye[1]);
                assertEquals(aspect, width / (float) height, 0.005f);
            }
        }
    }

    @Test
    public void fullscreenFitAlsoKeepsNonFourByThreeGuestModes() {
        float aspect = RetroDisplayModeCatalog.resolve(1280, 1024)
            .displayAspect();
        assertEquals(1000, RetroDisplayMode.fittedWidth(1000, 800, aspect));
        assertEquals(800, RetroDisplayMode.fittedHeight(1000, 800, aspect));
        float wide = RetroDisplayModeCatalog.resolve(240, 160)
            .displayAspect();
        assertEquals(1000, RetroDisplayMode.fittedWidth(1000, 800, wide));
        assertEquals(667, RetroDisplayMode.fittedHeight(1000, 800, wide));
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
