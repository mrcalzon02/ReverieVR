package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

public final class DosScreenPlacementTest {
    @Test public void defaultMatchesExistingWindowedDosGeometry() {
        assertEquals(1f, DosScreenPlacement.DEFAULT.scale, 0.00001f);
        assertEquals(3.2f, DosScreenPlacement.DEFAULT.distance, 0.00001f);
        assertEquals(0f, DosScreenPlacement.DEFAULT.height, 0.00001f);
    }

    @Test public void allSixControlsAreIndependentAndResetIsExact() {
        DosScreenPlacement value = DosScreenPlacement.DEFAULT;
        value = value.adjust(1);
        assertTrue(value.scale > 1f);
        assertEquals(3.2f, value.distance, 0.0001f);
        value = value.adjust(2);
        assertTrue(value.distance < 3.2f);
        value = value.adjust(4);
        assertTrue(value.height > 0f);
        assertTrue(value.adjust(6).sameAs(DosScreenPlacement.DEFAULT));
        assertEquals(value, value.adjust(99));
    }

    @Test public void invalidPersistedValuesFailToSafeDefaults() {
        DosScreenPlacement value = new DosScreenPlacement(
            Float.NaN, Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY);
        assertTrue(value.sameAs(DosScreenPlacement.DEFAULT));
        DosScreenPlacement extreme = new DosScreenPlacement(100f, -100f, 100f);
        assertEquals(DosScreenPlacement.MAX_SCALE, extreme.scale, 0.0001f);
        assertEquals(DosScreenPlacement.MIN_DISTANCE, extreme.distance, 0.0001f);
        assertEquals(DosScreenPlacement.MAX_HEIGHT, extreme.height, 0.0001f);
    }

    @Test public void repeatedTouchesCannotEscapeBounds() {
        DosScreenPlacement p = DosScreenPlacement.DEFAULT;
        for (int i=0;i<100;i++)p=p.adjust(1).adjust(2).adjust(4);
        assertEquals(DosScreenPlacement.MAX_SCALE,p.scale,0.0001f);
        assertEquals(DosScreenPlacement.MIN_DISTANCE,p.distance,0.0001f);
        assertEquals(DosScreenPlacement.MAX_HEIGHT,p.height,0.0001f);
        for (int i=0;i<100;i++)p=p.adjust(0).adjust(3).adjust(5);
        assertEquals(DosScreenPlacement.MIN_SCALE,p.scale,0.0001f);
        assertEquals(DosScreenPlacement.MAX_DISTANCE,p.distance,0.0001f);
        assertEquals(DosScreenPlacement.MIN_HEIGHT,p.height,0.0001f);
    }
}
