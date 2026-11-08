package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

/** Exercises the production binding engine and bus, including queued taps. */
public final class HostedInputFocusTest {
    private static InputBindingEngine nativeEngine(VirtualInputBus bus) {
        return new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(BuiltInBindingProfiles.ID_NATIVE_CLASSICS));
    }

    @Test public void menuEntryClearsHeldControlsAndQueuedTaps() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = nativeEngine(bus);
        engine.submitDigital(BindingInput.SELECT, true);
        engine.submitDigital(BindingInput.SELECT, false);
        engine.submitDigital(BindingInput.VOLUME_UP, true);
        engine.submitAxis(BindingInput.TOUCHPAD_X, 0.8f);
        engine.setGuestInputSuppressed(true);
        VirtualInputSnapshot frame = new VirtualInputSnapshot();
        bus.capture(frame);
        assertFalse(frame.joystickButtons[0]);
        assertFalse(frame.joystickButtons[1]);
        assertEquals(0f, frame.joystickAxisX, 0f);
    }

    @Test public void heldClickCannotLeakThroughMenuClose() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = nativeEngine(bus);
        engine.setGuestInputSuppressed(true);
        engine.submitDigital(BindingInput.SELECT, true);
        engine.setGuestInputSuppressed(false);
        engine.submitDigital(BindingInput.SELECT, true);
        assertFalse(bus.isJoystickButtonDown(0));
        engine.submitDigital(BindingInput.SELECT, false);
        engine.submitDigital(BindingInput.SELECT, true);
        assertTrue(bus.isJoystickButtonDown(0));
    }

    @Test public void releaseDuringMenuRearmsNextPress() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = nativeEngine(bus);
        engine.submitDigital(BindingInput.SELECT, true);
        engine.setGuestInputSuppressed(true);
        engine.submitDigital(BindingInput.SELECT, false);
        engine.setGuestInputSuppressed(false);
        engine.submitDigital(BindingInput.SELECT, true);
        assertTrue(bus.isJoystickButtonDown(0));
    }

    @Test public void heldAxisMustReturnToItsConfiguredDeadzone() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = nativeEngine(bus);
        engine.submitAxis(BindingInput.TOUCHPAD_X, 0.8f);
        assertTrue(bus.getJoystickAxisX() > 0f);
        engine.setGuestInputSuppressed(true);
        engine.setGuestInputSuppressed(false);
        engine.submitAxis(BindingInput.TOUCHPAD_X, 0.8f);
        assertEquals(0f, bus.getJoystickAxisX(), 0f);
        engine.submitAxis(BindingInput.TOUCHPAD_X, 0f);
        engine.submitAxis(BindingInput.TOUCHPAD_X, -0.8f);
        assertTrue(bus.getJoystickAxisX() < 0f);
    }

    @Test public void menuMotionNeverAccumulatesForLaterReplay() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(BuiltInBindingProfiles.ID_DOS_CURSOR));
        engine.submitRelative(BindingInput.HEAD_YAW_DELTA, 0.2f);
        engine.setGuestInputSuppressed(true);
        engine.submitRelative(BindingInput.HEAD_YAW_DELTA, 0.4f);
        engine.setGuestInputSuppressed(false);
        assertEquals(0f, bus.consumeMouseRelativeX(), 0f);
        engine.submitRelative(BindingInput.HEAD_YAW_DELTA, 0.01f);
        assertTrue(bus.consumeMouseRelativeX() > 0f);
    }

    @Test public void oneHeldSourceDoesNotBlockAnIndependentControl() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = nativeEngine(bus);
        engine.submitDigital(BindingInput.SELECT, true);
        engine.setGuestInputSuppressed(true);
        engine.setGuestInputSuppressed(false);
        engine.submitDigital(BindingInput.VOLUME_UP, true);
        assertFalse(bus.isJoystickButtonDown(0));
        assertTrue(bus.isJoystickButtonDown(1));
    }

    @Test public void nonFiniteAxisCannotPoisonTheInputBus() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = nativeEngine(bus);
        engine.submitAxis(BindingInput.TOUCHPAD_X, Float.NaN);
        assertEquals(0f, bus.getJoystickAxisX(), 0f);
    }
}
