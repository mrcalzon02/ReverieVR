package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class PointerVisualFeedbackTest {
    @Test public void pressOverridesHoverAndIdle() {
        assertEquals(PointerVisualFeedback.IDLE,
            PointerVisualFeedback.resolve(false, false));
        assertEquals(PointerVisualFeedback.HOVER,
            PointerVisualFeedback.resolve(true, false));
        assertEquals(PointerVisualFeedback.PRESSED,
            PointerVisualFeedback.resolve(false, true));
        assertEquals(PointerVisualFeedback.PRESSED,
            PointerVisualFeedback.resolve(true, true));
    }

    @Test public void markerGrowsWithInteractionStrength() {
        float idle = PointerVisualFeedback.markerSize(PointerVisualFeedback.IDLE);
        float hover = PointerVisualFeedback.markerSize(PointerVisualFeedback.HOVER);
        float pressed = PointerVisualFeedback.markerSize(PointerVisualFeedback.PRESSED);
        assertTrue(idle < hover);
        assertTrue(hover < pressed);
        assertEquals(idle, PointerVisualFeedback.markerSize(99), 0.00001f);
    }

    @Test public void colorsAreFiniteBoundedAndDistinct() {
        float[] idle = new float[4];
        float[] hover = new float[4];
        float[] pressed = new float[4];
        PointerVisualFeedback.writeColor(PointerVisualFeedback.IDLE, idle);
        PointerVisualFeedback.writeColor(PointerVisualFeedback.HOVER, hover);
        PointerVisualFeedback.writeColor(PointerVisualFeedback.PRESSED, pressed);
        for (float[] color : new float[][] {idle, hover, pressed}) {
            for (float value : color) {
                assertTrue(Float.isFinite(value));
                assertTrue(value >= 0.0f && value <= 1.0f);
            }
        }
        assertTrue(idle[0] != hover[0] || idle[1] != hover[1] || idle[2] != hover[2]);
        assertTrue(hover[0] != pressed[0] || hover[1] != pressed[1] || hover[2] != pressed[2]);
    }
}
