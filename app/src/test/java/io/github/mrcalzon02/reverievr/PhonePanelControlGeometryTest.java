package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class PhonePanelControlGeometryTest {
    @Test public void allSixPhysicalKeysHaveIndependentHitTargets() {
        assertEquals(6, PhonePanelControlGeometry.COUNT);
        for (int i = 0; i < PhonePanelControlGeometry.COUNT; i++) {
            float center = PhonePanelControlGeometry.top(i)
                - PhonePanelControlGeometry.KEY_HEIGHT / 2f;
            assertEquals(i, PhonePanelControlGeometry.indexAtLocal(0.92f, center));
        }
    }

    @Test public void closeIsTheBottomRightControl() {
        assertEquals(5, PhonePanelControlGeometry.CLOSE_INDEX);
        float closeCenter = PhonePanelControlGeometry.top(5)
            - PhonePanelControlGeometry.KEY_HEIGHT / 2f;
        assertTrue(closeCenter > -0.72f);
        assertEquals(5, PhonePanelControlGeometry.indexAtLocal(1.0f, closeCenter));
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(0.6f, closeCenter));
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(1.2f, closeCenter));
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(1.0f, -0.2f));
    }

    @Test public void outsideAndInvalidIntersectionsNeverActivate() {
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(
            Float.NaN, 0.2f));
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(
            0.92f, Float.POSITIVE_INFINITY));
        assertEquals(-1, PhonePanelControlGeometry.indexAtLocal(
            Float.NEGATIVE_INFINITY, 0.2f));
    }

    @Test(expected=IllegalArgumentException.class)
    public void invalidKeyIndexDoesNotHaveGeometry() {
        PhonePanelControlGeometry.top(6);
    }
}
