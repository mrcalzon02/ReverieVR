package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class PhonePanelControlGeometryTest {
    @Test public void bothRailsHaveIndependentTargets() {
        assertEquals(10, PhonePanelControlGeometry.COUNT);
        for (int i = 0; i < PhonePanelControlGeometry.COUNT; i++) {
            float center = PhonePanelControlGeometry.top(i)
                - PhonePanelControlGeometry.KEY_HEIGHT / 2f;
            float x = (PhonePanelControlGeometry.left(i)
                + PhonePanelControlGeometry.right(i)) / 2f;
            assertEquals(i, PhonePanelControlGeometry.indexAtLocal(x, center));
        }
    }

    @Test public void originalCloseRemainsOnRightRail() {
        assertEquals(5, PhonePanelControlGeometry.CLOSE_INDEX);
        float center = PhonePanelControlGeometry.top(5)
            - PhonePanelControlGeometry.KEY_HEIGHT / 2f;
        assertEquals(5, PhonePanelControlGeometry.indexAtLocal(1.0f, center));
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(0.6f, center));
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(1.2f, center));
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(1.0f, -0.2f));
    }

    @Test public void newRailCannotTargetPhoneGlassOrOtherControls() {
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(0f, 0.6f));
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(-1.3f, 0.6f));
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(-0.5f, 0.6f));
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(
            Float.NaN, 0.2f));
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(
            -0.92f, Float.POSITIVE_INFINITY));
    }

    @Test public void adjustmentsAreBoundedAndResettable() {
        float scale = 1f;
        float offset = 0f;
        for (int i = 0; i < 20; i++) {
            scale = PhonePanelControlGeometry.adjustedScale(scale, 1);
            offset = PhonePanelControlGeometry.adjustedDistance(offset, 1);
        }
        assertEquals(1.15f, scale, .0001f);
        assertEquals(.45f, offset, .0001f);
        for (int i = 0; i < 20; i++) {
            scale = PhonePanelControlGeometry.adjustedScale(scale, -1);
            offset = PhonePanelControlGeometry.adjustedDistance(offset, -1);
        }
        assertEquals(.75f, scale, .0001f);
        assertEquals(-.30f, offset, .0001f);
        assertEquals(1.1f, PhonePanelControlGeometry.adjustedScale(1f, 1), .0001f);
        assertEquals(-.15f, PhonePanelControlGeometry.adjustedDistance(0f, -1), .0001f);
    }

    @Test public void nonfiniteAdjustmentStateUsesSafeDefaults() {
        assertEquals(1.1f, PhonePanelControlGeometry.adjustedScale(Float.NaN, 1), .0001f);
        assertEquals(-.15f, PhonePanelControlGeometry.adjustedDistance(Float.POSITIVE_INFINITY, -1), .0001f);
    }

    @Test(expected=IllegalArgumentException.class)
    public void invalidControlHasNoGeometry() {
        PhonePanelControlGeometry.top(PhonePanelControlGeometry.COUNT);
    }
}
