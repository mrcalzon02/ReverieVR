package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import java.nio.FloatBuffer;
import org.junit.Test;

public class PointerRibbonGeometryTest {
    @Test public void writesEighteenFiniteVerticesWithoutAllocating() {
        FloatBuffer buffer = FloatBuffer.allocate(54);
        PointerRibbonGeometry.fill(buffer, 0f, 0f, -0.5f, 1f, 2f, -2f, 0.024f);
        assertEquals(54, buffer.position());
        for (int i = 0; i < 54; i++) assertTrue(Float.isFinite(buffer.get(i)));
    }

    @Test public void straightAheadRayRetainsNonZeroBeamWidth() {
        FloatBuffer buffer = FloatBuffer.allocate(54);
        PointerRibbonGeometry.fill(buffer, 0f, 0f, 0f, 0f, 0f, -3f, 0.016f);
        assertTrue(buffer.get(0) > buffer.get(3));
        assertEquals(-3f, buffer.get(8), 0.00001f);
    }

    @Test public void rejectsUndersizedScratchBuffer() {
        try {
            PointerRibbonGeometry.fill(FloatBuffer.allocate(53), 0, 0, 0, 0, 0, -1, 0.016f);
            fail("Expected capacity rejection");
        } catch (IllegalArgumentException expected) {
            assertTrue(expected.getMessage().contains("54"));
        }
    }
}
