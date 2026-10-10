package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

public class PointerRayValidationTest {
    @Test public void acceptsValidButNonUnitDirection() {
        assertTrue(PointerRayValidation.valid(0f, 1f, 0f, 0f, 0f, -2f, 2f));
    }

    @Test public void rejectsZeroAndCorruptDirection() {
        assertFalse(PointerRayValidation.valid(0, 0, 0, 0, 0, 0, 2));
        assertFalse(PointerRayValidation.valid(0, 0, 0, Float.NaN, 0, -1, 2));
        assertFalse(PointerRayValidation.valid(0, 0, 0, Float.POSITIVE_INFINITY, 0, -1, 2));
        assertFalse(PointerRayValidation.valid(0, 0, 0, Float.MAX_VALUE, 0, -1, 2));
    }

    @Test public void rejectsCorruptOriginAndDistance() {
        assertFalse(PointerRayValidation.valid(Float.NaN, 0, 0, 0, 0, -1, 2));
        assertFalse(PointerRayValidation.valid(0, 0, 0, 0, 0, -1, Float.NaN));
        assertFalse(PointerRayValidation.valid(0, 0, 0, 0, 0, -1, Float.NEGATIVE_INFINITY));
    }

    @Test public void boundsInteractionRayDistance() {
        assertEquals(0.25f, PointerRayValidation.clampDistance(-12f), 0f);
        assertEquals(12f, PointerRayValidation.clampDistance(120f), 0f);
        assertEquals(3f, PointerRayValidation.clampDistance(3f), 0f);
    }
}
