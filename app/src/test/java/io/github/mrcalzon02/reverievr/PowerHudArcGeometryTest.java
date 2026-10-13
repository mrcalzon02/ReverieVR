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

    @Test public void shrinkBySeventyPercentAndCenterLabels() {
        assertEquals(0.30f, PowerHudArcGeometry.OUTPUT_SCALE, 0.0001f);
        assertEquals(399.7f, PowerHudArcGeometry.scaledNearTopRight(280f, 451f),
            0.0001f);
        assertTrue(PowerHudArcGeometry.labelCenterX(PowerHudArcGeometry.OUTER) > 330f);
        assertTrue(PowerHudArcGeometry.labelTopBaselineY(PowerHudArcGeometry.OUTER) < 110f);
    }

    @Test public void powerHudMovesDownByTenPercentOfEachEyeWithoutResizing() {
        // A full eye covers 2 units of normalized clip-space height.
        assertEquals(0.10f, PowerHudArcGeometry.VIEWPORT_DOWN_NDC / 2f, 0.0001f);
        float originalTop = 1f - 2f * 42f / 512f;
        float originalBottom = PowerHudArcGeometry.scaledNearTopRight(
            1f - 2f * 284f / 512f, originalTop);
        float loweredTop = PowerHudArcGeometry.lowerInViewport(originalTop);
        float loweredBottom = PowerHudArcGeometry.lowerInViewport(originalBottom);
        assertEquals(originalTop - 0.20f, loweredTop, 0.0001f);
        assertEquals(originalBottom - 0.20f, loweredBottom, 0.0001f);
        assertEquals(originalTop - originalBottom,
            loweredTop - loweredBottom, 0.0001f);
        assertEquals(0.30f, PowerHudArcGeometry.OUTPUT_SCALE, 0.0001f);
    }

    @Test public void lookingUpRevealsTheSameBarsLower() {
        assertEquals(0f, PowerHudArcGeometry.revealY(false), 0f);
        assertEquals(52f, PowerHudArcGeometry.revealY(true), 0f);
    }
}
