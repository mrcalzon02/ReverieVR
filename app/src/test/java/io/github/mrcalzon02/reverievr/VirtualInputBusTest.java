package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class VirtualInputBusTest {
    @Test
    public void mouseWheelAccumulatesUntilConsumed() {
        VirtualInputBus bus = new VirtualInputBus();

        bus.applyMouseWheel(1.0f);
        bus.applyMouseWheel(-0.25f);

        assertEquals(
            0.75f,
            bus.consumeMouseWheel(),
            0.0001f
        );
        assertEquals(
            0.0f,
            bus.consumeMouseWheel(),
            0.0001f
        );
    }

    @Test
    public void mouseButtonsReleaseWithReleaseAll() {
        VirtualInputBus bus = new VirtualInputBus();

        bus.applyDigital(
            VirtualOutput.mouseButton(
                VirtualOutput.MOUSE_LEFT
            ),
            true
        );
        assertTrue(
            bus.isMouseButtonDown(
                VirtualOutput.MOUSE_LEFT
            )
        );

        bus.releaseAll();
        assertFalse(
            bus.isMouseButtonDown(
                VirtualOutput.MOUSE_LEFT
            )
        );
    }

    @Test
    public void absoluteMousePositionIsClamped() {
        VirtualInputBus bus = new VirtualInputBus();

        bus.applyAnalog(
            VirtualOutput.mouseAbsoluteX(),
            2.0f
        );
        bus.applyAnalog(
            VirtualOutput.mouseAbsoluteY(),
            -2.0f
        );

        assertEquals(
            1.0f,
            bus.getMouseAbsoluteX(),
            0.0001f
        );
        assertEquals(
            0.0f,
            bus.getMouseAbsoluteY(),
            0.0001f
        );
    }
}
