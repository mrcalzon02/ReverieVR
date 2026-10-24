package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class PowerHudArcGeometryTest {
    @Test public void arcIsOneEighthOfTheEyeCircumference() {
        assertEquals(45f, PowerHudArcGeometry.SWEEP, 0.001f);
        assertTrue(PowerHudArcGeometry.START < 0f);
        assertTrue(PowerHudArcGeometry.START + PowerHudArcGeometry.SWEEP < 0f);
    }

    @Test public void arcsStayInsideTheNormalLensSightline() {
        assertTrue(PowerHudArcGeometry.OUTER
            + PowerHudArcGeometry.TRACK_WIDTH / 2f
            < PowerHudArcGeometry.CENTER * 0.75f);
        assertTrue(PowerHudArcGeometry.INNER
            + PowerHudArcGeometry.TRACK_WIDTH
            < PowerHudArcGeometry.OUTER);
    }

    @Test public void controllerUnknownDoesNotFakeBatteryCharge() {
        assertEquals(0f, PowerHudArcGeometry.progress(-1), 0f);
        assertEquals(0f, PowerHudArcGeometry.progress(101), 0f);
        assertEquals(0f, PowerHudArcGeometry.progress(0), 0f);
        assertEquals(22.5f, PowerHudArcGeometry.progress(50), 0.001f);
        assertEquals(45f, PowerHudArcGeometry.progress(100), 0.001f);
    }

    @Test public void labelsSitLeftOfArcStartAndRemainInHudTextureCrop() {
        assertEquals(0.30f, PowerHudArcGeometry.OUTPUT_SCALE, 0.0001f);
        assertEquals(399.7f, PowerHudArcGeometry.scaledNearTopRight(280f, 451f),
            0.0001f);
        // The widest "P 100%" label is centered, never clipped by the quad.
        for (float radius : new float[] {
            PowerHudArcGeometry.OUTER, PowerHudArcGeometry.INNER
        }) {
            float center = PowerHudArcGeometry.labelCenterX(radius);
            assertTrue(center - 40f > PowerHudArcGeometry.CROP_LEFT);
            assertTrue(center + 40f < PowerHudArcGeometry.arcStartX(radius));
            assertEquals(PowerHudArcGeometry.arcStartY(radius) + 6f,
                PowerHudArcGeometry.labelTopBaselineY(radius), 0.0001f);
        }
        // Independent percentage rows must not overlap vertically.
        assertTrue(PowerHudArcGeometry.labelTopBaselineY(PowerHudArcGeometry.INNER)
            - PowerHudArcGeometry.labelTopBaselineY(PowerHudArcGeometry.OUTER)
            > 18f);
    }

    @Test public void powerHudMovesFifteenPercentUpAndLeftWithoutResizing() {
        // A full eye covers 2 units of normalized clip-space height.
        assertEquals(0.10f, PowerHudArcGeometry.VIEWPORT_DOWN_NDC / 2f, 0.0001f);
        assertEquals(0.05f, PowerHudArcGeometry.EXTRA_DOWN_NDC / 2f, 0.0001f);
        assertEquals(0.15f, PowerHudArcGeometry.EXTRA_INWARD_NDC / 2f, 0.0001f);
        float originalTop = 1f - 2f * 42f / 512f;
        float originalBottom = PowerHudArcGeometry.scaledNearTopRight(
            1f - 2f * 284f / 512f, originalTop);
        float loweredTop = PowerHudArcGeometry.lowerInViewport(originalTop);
        float loweredBottom = PowerHudArcGeometry.lowerInViewport(originalBottom);
        assertEquals(originalTop - 0.30f, loweredTop, 0.0001f);
        assertEquals(originalBottom - 0.30f, loweredBottom, 0.0001f);
        assertEquals(0.52f,
            PowerHudArcGeometry.inwardInViewport(0.82f), 0.0001f);
        assertEquals(originalTop - originalBottom,
            loweredTop - loweredBottom, 0.0001f);
        assertEquals(0.30f, PowerHudArcGeometry.OUTPUT_SCALE, 0.0001f);
    }

    @Test public void lookingUpRevealsTheSameBarsLower() {
        assertEquals(0f, PowerHudArcGeometry.revealY(false), 0f);
        assertEquals(52f, PowerHudArcGeometry.revealY(true), 0f);
    }
}
