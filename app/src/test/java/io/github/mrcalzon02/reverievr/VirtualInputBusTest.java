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
    public void tapCompletedBetweenFramesIsDeliveredExactlyOnce() {
        VirtualInputBus bus = new VirtualInputBus();
        VirtualInputSnapshot frame = new VirtualInputSnapshot();
        bus.applyDigital(
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT), true);
        bus.applyDigital(
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT), false);
        bus.applyDigital(VirtualOutput.key(VirtualKey.ENTER), true);
        bus.applyDigital(VirtualOutput.key(VirtualKey.ENTER), false);
        bus.applyDigital(VirtualOutput.joystickButton(0), true);
        bus.applyDigital(VirtualOutput.joystickButton(0), false);

        bus.capture(frame);
        assertTrue(frame.mouseButtons[VirtualOutput.MOUSE_LEFT]);
        assertTrue(frame.keys[VirtualKey.ENTER.ordinal()]);
        assertTrue(frame.joystickButtons[0]);
        bus.capture(frame);
        assertFalse(frame.mouseButtons[VirtualOutput.MOUSE_LEFT]);
        assertFalse(frame.keys[VirtualKey.ENTER.ordinal()]);
        assertFalse(frame.joystickButtons[0]);
    }

    @Test
    public void heldMouseButtonReleasesOnNextFrameWithoutExtraPress() {
        VirtualInputBus bus = new VirtualInputBus();
        VirtualInputSnapshot frame = new VirtualInputSnapshot();
        bus.applyDigital(
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT), true);
        bus.capture(frame);
        assertTrue(frame.mouseButtons[VirtualOutput.MOUSE_LEFT]);
        bus.capture(frame);
        assertTrue(frame.mouseButtons[VirtualOutput.MOUSE_LEFT]);
        bus.applyDigital(
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_LEFT), false);
        bus.capture(frame);
        assertFalse(frame.mouseButtons[VirtualOutput.MOUSE_LEFT]);
    }

    @Test
    public void shellRecoveryDiscardsUnobservedGuestPress() {
        VirtualInputBus bus = new VirtualInputBus();
        VirtualInputSnapshot frame = new VirtualInputSnapshot();
        bus.applyDigital(VirtualOutput.key(VirtualKey.ESCAPE), true);
        bus.applyDigital(VirtualOutput.key(VirtualKey.ESCAPE), false);
        bus.applyDigital(
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_RIGHT), true);
        bus.applyDigital(
            VirtualOutput.mouseButton(VirtualOutput.MOUSE_RIGHT), false);
        bus.releaseAll();
        bus.capture(frame);
        assertFalse(frame.keys[VirtualKey.ESCAPE.ordinal()]);
        assertFalse(frame.mouseButtons[VirtualOutput.MOUSE_RIGHT]);
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
