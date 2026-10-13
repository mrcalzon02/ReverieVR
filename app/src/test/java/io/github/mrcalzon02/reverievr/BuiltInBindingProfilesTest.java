package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;

import org.junit.Test;

public final class BuiltInBindingProfilesTest {
    @Test
    public void nativeStandardUsesControllerButtons() {
        BindingProfile profile =
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_NATIVE_STANDARD
            );

        assertNotNull(profile);
        assertEquals(
            BuiltInBindingProfiles.ID_NATIVE_STANDARD,
            profile.id
        );
        assertEquals(15, profile.bindings.size());

        assertDigitalButton(
            profile.bindings.get(0),
            BindingInput.SELECT,
            0
        );
        assertDigitalButton(
            profile.bindings.get(1),
            BindingInput.VOLUME_UP,
            1
        );
        assertDigitalButton(
            profile.bindings.get(2),
            BindingInput.VOLUME_DOWN,
            2
        );
    }

    @Test
    public void nativeStandardKeepsPhysicalHidButtonsSeparateFromShell() {
        BindingProfile profile = BuiltInBindingProfiles.byId(
            BuiltInBindingProfiles.ID_NATIVE_STANDARD);
        assertDigitalButton(profile.bindings.get(3),
            BindingInput.GAMEPAD_BUTTON_B, 3);
        assertDigitalButton(profile.bindings.get(4),
            BindingInput.GAMEPAD_BUTTON_X, 4);
        assertDigitalButton(profile.bindings.get(5),
            BindingInput.GAMEPAD_BUTTON_Y, 5);
        assertDigitalButton(profile.bindings.get(12),
            BindingInput.GAMEPAD_BUTTON_SELECT, 12);
        assertEquals(BindingInput.GAMEPAD_X, profile.bindings.get(13).input);
        assertEquals(VirtualOutputKind.JOYSTICK_AXIS_X,
            profile.bindings.get(13).output.kind);
        assertEquals(BindingInput.GAMEPAD_Y, profile.bindings.get(14).input);
        assertEquals(VirtualOutputKind.JOYSTICK_AXIS_Y,
            profile.bindings.get(14).output.kind);
        for (InputBinding binding : profile.bindings) {
            assertFalse(binding.input == BindingInput.GAMEPAD_BUTTON_MODE);
        }
    }

    @Test
    public void nativeStandardDoesNotLeakIntoDosProfileList() {
        assertFalse(
            BuiltInBindingProfiles.isDosProfileId(
                BuiltInBindingProfiles.ID_NATIVE_STANDARD
            )
        );
    }

    @Test
    public void nativeClassicsTurnDaydreamPadIntoPlayableAxes() {
        assertEquals(
            BuiltInBindingProfiles.ID_NATIVE_CLASSICS,
            BuiltInBindingProfiles.nativeProfileForModule("classic-chess")
        );
        assertEquals(
            BuiltInBindingProfiles.ID_NATIVE_CLASSICS,
            BuiltInBindingProfiles.nativeProfileForModule("classic-pong")
        );
        assertEquals(
            BuiltInBindingProfiles.ID_NATIVE_STANDARD,
            BuiltInBindingProfiles.nativeProfileForModule("breakwater-battery")
        );
        assertFalse(BuiltInBindingProfiles.isDosProfileId(
            BuiltInBindingProfiles.ID_NATIVE_CLASSICS));

        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = new InputBindingEngine(
            bus, BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_NATIVE_CLASSICS));
        engine.submitAxis(BindingInput.TOUCHPAD_X, 0.9f);
        engine.submitAxis(BindingInput.TOUCHPAD_Y, -0.8f);
        assertEquals(0.9f, bus.getJoystickAxisX(), 0.0001f);
        assertEquals(-0.8f, bus.getJoystickAxisY(), 0.0001f);
        engine.submitDigital(BindingInput.SELECT, true);
        org.junit.Assert.assertTrue(bus.isJoystickButtonDown(0));
        engine.submitDigital(BindingInput.SELECT, false);
        engine.submitAxis(BindingInput.TOUCHPAD_X, 0.0f);
        engine.submitAxis(BindingInput.TOUCHPAD_Y, 0.0f);
        assertEquals(0.0f, bus.getJoystickAxisX(), 0.0001f);
        assertEquals(0.0f, bus.getJoystickAxisY(), 0.0001f);
        assertFalse(bus.isJoystickButtonDown(0));
    }

    private static void assertDigitalButton(
        InputBinding binding,
        BindingInput input,
        int button
    ) {
        assertEquals(input, binding.input);
        assertEquals(
            VirtualOutputKind.JOYSTICK_BUTTON,
            binding.output.kind
        );
        assertEquals(button, binding.output.code);
    }

    @Test
    public void dosProfilesNeverOfferEmptyBindings() {
        assertFalse(BuiltInBindingProfiles.isDosProfileId(
            BuiltInBindingProfiles.ID_NONE));
        org.junit.Assert.assertTrue(BuiltInBindingProfiles.isDosProfileId(
            BuiltInBindingProfiles.ID_DOS_STANDARD));
        for (BindingProfile profile : BuiltInBindingProfiles.dosProfiles())
            org.junit.Assert.assertTrue(profile.id, !profile.bindings.isEmpty());
    }

    @Test
    public void standardDosTrackpadProducesArrowsAndMouseClick() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(BuiltInBindingProfiles.ID_DOS_STANDARD));
        engine.submitAxis(BindingInput.TOUCHPAD_Y, -0.8f);
        org.junit.Assert.assertTrue(bus.isKeyDown(VirtualKey.UP));
        org.junit.Assert.assertTrue(bus.isJoystickButtonDown(4));
        engine.submitDigital(BindingInput.SELECT, true);
        org.junit.Assert.assertTrue(bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));
        engine.submitDigital(BindingInput.SELECT, false);
        engine.submitAxis(BindingInput.TOUCHPAD_Y, 0f);
        assertFalse(bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));
        assertFalse(bus.isKeyDown(VirtualKey.UP));
        assertFalse(bus.isJoystickButtonDown(4));
    }

    @Test
    public void independentDosAimProfilesRetainTouchpadDirections() {
        String[] ids = {
            BuiltInBindingProfiles.ID_DOS_HEAD_MOUSE,
            BuiltInBindingProfiles.ID_DOS_CONTROLLER_MOUSE
        };
        for (String id : ids) {
            VirtualInputBus bus = new VirtualInputBus();
            InputBindingEngine engine = new InputBindingEngine(
                bus, BuiltInBindingProfiles.byId(id));
            engine.submitAxis(BindingInput.TOUCHPAD_X, 0.8f);
            engine.submitAxis(BindingInput.TOUCHPAD_Y, -0.8f);
            org.junit.Assert.assertTrue(id, bus.isKeyDown(VirtualKey.RIGHT));
            org.junit.Assert.assertTrue(id, bus.isKeyDown(VirtualKey.UP));
            org.junit.Assert.assertTrue(id, bus.isJoystickButtonDown(7));
            org.junit.Assert.assertTrue(id, bus.isJoystickButtonDown(4));
            engine.submitAxis(BindingInput.TOUCHPAD_X, 0f);
            engine.submitAxis(BindingInput.TOUCHPAD_Y, 0f);
            assertFalse(id, bus.isKeyDown(VirtualKey.RIGHT));
            assertFalse(id, bus.isKeyDown(VirtualKey.UP));
            assertFalse(id, bus.isJoystickButtonDown(7));
            assertFalse(id, bus.isJoystickButtonDown(4));
            engine.submitDigital(BindingInput.SELECT, true);
            org.junit.Assert.assertTrue(id,
                bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));
        }
    }

    @Test
    public void touchpadMouseDoesNotEmitArrowsWhenSwiping() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_DOS_TOUCHPAD_MOUSE));
        engine.submitRelative(BindingInput.TOUCHPAD_REL_X, 18f);
        engine.submitAxis(BindingInput.TOUCHPAD_X, 0.8f);
        assertEquals(12.6f, bus.consumeMouseRelativeX(), 0.0001f);
        assertFalse(bus.isKeyDown(VirtualKey.RIGHT));
        assertFalse(bus.isJoystickButtonDown(7));
    }

    @Test
    public void dosHeadsetMouseCorrectsBothGuestPointerAxesOnly() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_DOS_HEAD_MOUSE));
        engine.submitRelative(BindingInput.HEAD_YAW_DELTA, .01f);
        engine.submitRelative(BindingInput.HEAD_PITCH_DELTA, .01f);
        assertEquals(-6.5f, bus.consumeMouseRelativeX(), .001f);
        assertEquals(6.5f, bus.consumeMouseRelativeY(), .001f);
        // The separate controller-aim profile must retain its own direction.
        VirtualInputBus controller = new VirtualInputBus();
        InputBindingEngine controllerEngine = new InputBindingEngine(controller,
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_DOS_CONTROLLER_MOUSE));
        controllerEngine.submitRelative(BindingInput.CONTROLLER_YAW_DELTA, .01f);
        assertEquals(6.5f, controller.consumeMouseRelativeX(), .001f);
    }

    @Test
    public void dosRockerProvidesKeyboardMenuRecovery() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(BuiltInBindingProfiles.ID_DOS_STANDARD));
        engine.submitDigital(BindingInput.VOLUME_UP, true);
        engine.submitDigital(BindingInput.VOLUME_DOWN, true);
        org.junit.Assert.assertTrue(bus.isKeyDown(VirtualKey.ENTER));
        org.junit.Assert.assertTrue(bus.isKeyDown(VirtualKey.ESCAPE));
    }

}
