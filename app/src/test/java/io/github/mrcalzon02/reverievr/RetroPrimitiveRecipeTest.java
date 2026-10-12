package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

public final class RetroPrimitiveRecipeTest {
    @Test public void snapIsRepeatableAndFinite() {
        assertEquals(1.125f, RetroPrimitiveRecipe.snapPosition(1.123f), 0f);
        assertEquals(-1.125f, RetroPrimitiveRecipe.snapPosition(-1.123f), 0f);
        assertEquals(0f, RetroPrimitiveRecipe.snapPosition(0f), 0f);
        assertThrows(IllegalArgumentException.class,
            () -> RetroPrimitiveRecipe.snapPosition(Float.NaN));
    }

    @Test public void rgb555IsStableAndBounded() {
        float[] color = RetroPrimitiveRecipe.color555(.5f, -.1f, 1.3f);
        assertEquals(16f / 31f, color[0], 0.0001f);
        assertEquals(0f, color[1], 0f);
        assertEquals(1f, color[2], 0f);
        for (float value : color)
            assertEquals(Math.round(value * 31f), value * 31f, .0001f);
        assertArrayEquals(color,
            RetroPrimitiveRecipe.color555(.5f, -.1f, 1.3f), 0f);
    }

    @Test public void sameSeedSamePropsAndDifferentSeedVaries() {
        for (int index = 0; index < 40; index++)
            assertEquals(RetroPrimitiveRecipe.offset(4172, index),
                RetroPrimitiveRecipe.offset(4172, index), 0f);
        assertNotEquals(RetroPrimitiveRecipe.offset(4172, 9),
            RetroPrimitiveRecipe.offset(9931, 9), .000001f);
    }
}
