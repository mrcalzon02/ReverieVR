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

    @Test public void lookingUpRevealsTheSameBarsLower() {
        assertEquals(0f, PowerHudArcGeometry.revealY(false), 0f);
        assertEquals(52f, PowerHudArcGeometry.revealY(true), 0f);
    }
}
