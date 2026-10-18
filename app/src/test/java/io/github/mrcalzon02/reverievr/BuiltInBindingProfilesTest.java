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
        assertEquals(18, profile.bindings.size());

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
        assertEquals(BindingInput.DAYDREAM_TAP, profile.bindings.get(13).input);
        assertEquals(13, profile.bindings.get(13).output.code);
        assertEquals(BindingInput.DAYDREAM_DOUBLE_TAP, profile.bindings.get(14).input);
        assertEquals(14, profile.bindings.get(14).output.code);
        assertEquals(BindingInput.DAYDREAM_PRESS_HOLD, profile.bindings.get(15).input);
        assertEquals(15, profile.bindings.get(15).output.code);
        assertEquals(BindingInput.GAMEPAD_X, profile.bindings.get(16).input);
        assertEquals(VirtualOutputKind.JOYSTICK_AXIS_X,
            profile.bindings.get(16).output.kind);
        assertEquals(BindingInput.GAMEPAD_Y, profile.bindings.get(17).input);
        assertEquals(VirtualOutputKind.JOYSTICK_AXIS_Y,
            profile.bindings.get(17).output.kind);
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
            BuiltInBindingProfiles.ID_NATIVE_CLASSICS,
            BuiltInBindingProfiles.nativeProfileForModule("omega-front")
        );
        assertEquals(
            BuiltInBindingProfiles.ID_NATIVE_BREAKWATER,
            BuiltInBindingProfiles.nativeProfileForModule("breakwater-battery")
        );
        VirtualInputBus breakwaterBus=new VirtualInputBus();
        InputBindingEngine breakwaterEngine=new InputBindingEngine(
            breakwaterBus,BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_NATIVE_BREAKWATER));
        breakwaterEngine.submitAxis(BindingInput.TOUCHPAD_X,.95f);
        assertEquals(.95f,breakwaterBus.getJoystickAxisX(),.0001f);
        breakwaterEngine.submitAxis(BindingInput.TOUCHPAD_X,0.f);
        assertEquals(0.f,breakwaterBus.getJoystickAxisX(),.0001f);
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
    public void dosMenuDirectionsAndHeadLookWorkWithoutHoldingSelect() {
        for (String id : new String[] {
                BuiltInBindingProfiles.ID_DOS_STANDARD,
                BuiltInBindingProfiles.ID_DOS_HEAD_MOUSE
        }) {
            VirtualInputBus bus = new VirtualInputBus();
            InputBindingEngine engine = new InputBindingEngine(bus,
                BuiltInBindingProfiles.byId(id));

            engine.submitAxis(BindingInput.TOUCHPAD_Y, -0.8f);
            engine.submitRelative(BindingInput.HEAD_PITCH_DELTA, 0.01f);
            engine.submitRelative(BindingInput.HEAD_YAW_DELTA, 0.01f);
            assertFalse(id, bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));
            org.junit.Assert.assertTrue(id, bus.isKeyDown(VirtualKey.UP));
            assertEquals(id, -6.5f, bus.consumeMouseRelativeY(), 0.001f);
            assertEquals(id, 6.5f, bus.consumeMouseRelativeX(), 0.001f);

            engine.submitAxis(BindingInput.TOUCHPAD_Y, 0.8f);
            assertFalse(id, bus.isKeyDown(VirtualKey.UP));
            org.junit.Assert.assertTrue(id, bus.isKeyDown(VirtualKey.DOWN));
            engine.submitAxis(BindingInput.TOUCHPAD_Y, 0f);
            assertFalse(id, bus.isKeyDown(VirtualKey.DOWN));
            assertFalse(id, bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));
        }
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
    public void dosHeadsetMouseMatchesWorking3dCameraAxes() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_DOS_HEAD_MOUSE));
        engine.submitRelative(BindingInput.HEAD_YAW_DELTA, .01f);
        engine.submitRelative(BindingInput.HEAD_PITCH_DELTA, .01f);
        // +Yaw = physical look right => mouse right.
        // +Pitch = physical look up => mouse up (negative screen Y).
        assertEquals(6.5f, bus.consumeMouseRelativeX(), .001f);
        assertEquals(-6.5f, bus.consumeMouseRelativeY(), .001f);
        // The separate controller-aim profile must retain its own direction.
        VirtualInputBus controller = new VirtualInputBus();
        InputBindingEngine controllerEngine = new InputBindingEngine(controller,
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_DOS_CONTROLLER_MOUSE));
        controllerEngine.submitRelative(BindingInput.CONTROLLER_YAW_DELTA, .01f);
        assertEquals(6.5f, controller.consumeMouseRelativeX(), .001f);
    }

    @Test
    public void allHeadsetDosProfilesShare3dCameraHorizontalPolarity() {
        for (String id : new String[] {
                BuiltInBindingProfiles.ID_DOS_DOOM_SHAREWARE,
                BuiltInBindingProfiles.ID_DOS_CURSOR,
                BuiltInBindingProfiles.ID_DOS_HEAD_MOUSE,
                BuiltInBindingProfiles.ID_DOS_STANDARD,
                BuiltInBindingProfiles.ID_DOS_FPS_HEAD_MOUSE
        }) {
            VirtualInputBus bus = new VirtualInputBus();
            InputBindingEngine engine = new InputBindingEngine(bus,
                BuiltInBindingProfiles.byId(id));
            engine.submitRelative(BindingInput.HEAD_YAW_DELTA, .01f);
            assertEquals(id + " positive yaw", 6.5f,
                bus.consumeMouseRelativeX(), .001f);
            engine.submitRelative(BindingInput.HEAD_YAW_DELTA, -.01f);
            assertEquals(id + " negative yaw", -6.5f,
                bus.consumeMouseRelativeX(), .001f);
        }
    }

    @Test
    public void staleBundledHeadMouseProfilesUpgradeWithoutDiscardingTuning() {
        for (String id : new String[] {
                BuiltInBindingProfiles.ID_DOS_DOOM_SHAREWARE,
                BuiltInBindingProfiles.ID_DOS_CURSOR,
                BuiltInBindingProfiles.ID_DOS_HEAD_MOUSE
        }) {
            BindingProfile current = BuiltInBindingProfiles.byId(id);
            java.util.List<InputBinding> old = new java.util.ArrayList<>();
            for (InputBinding b : current.bindings) {
                boolean staleYaw = b.input == BindingInput.HEAD_YAW_DELTA
                    && b.output.equals(VirtualOutput.mouseRelativeX());
                boolean stalePitch =
                    BuiltInBindingProfiles.ID_DOS_HEAD_MOUSE.equals(id)
                    && b.input == BindingInput.HEAD_PITCH_DELTA
                    && b.output.equals(VirtualOutput.mouseRelativeY());
                old.add(staleYaw || stalePitch
                    ? new InputBinding(b.input, b.output, b.transform,
                        -b.scale, b.deadzone, b.threshold) : b);
            }
            BindingProfile stored = BindingProfileCodec.decode(
                BindingProfileCodec.encode(current.withBindings(old)));
            BindingProfile effective =
                BuiltInBindingProfiles.upgradeLegacyStockHeadMouse(stored);
            VirtualInputBus bus = new VirtualInputBus();
            InputBindingEngine engine = new InputBindingEngine(bus, effective);
            engine.submitRelative(BindingInput.HEAD_YAW_DELTA, .01f);
            assertEquals(id, 6.5f, bus.consumeMouseRelativeX(), .001f);

            BindingProfile tuned = BindingProfileTuner.adjustAnalogScale(
                stored, 1.10f);
            org.junit.Assert.assertSame(id + " user's tuning preserved",
                tuned, BuiltInBindingProfiles.upgradeLegacyStockHeadMouse(tuned));
            org.junit.Assert.assertSame(id + " already-current preserved",
                current, BuiltInBindingProfiles.upgradeLegacyStockHeadMouse(current));
        }
    }

    @Test
    public void everyDosHeadsetMouseModeMovesUpWithPositiveHeadPitch() {
        for (String id : new String[] {
                BuiltInBindingProfiles.ID_DOS_CURSOR,
                BuiltInBindingProfiles.ID_DOS_HEAD_MOUSE,
                BuiltInBindingProfiles.ID_DOS_STANDARD,
                BuiltInBindingProfiles.ID_DOS_FPS_HEAD_MOUSE
        }) {
            VirtualInputBus bus = new VirtualInputBus();
            InputBindingEngine engine = new InputBindingEngine(bus,
                BuiltInBindingProfiles.byId(id));
            engine.submitRelative(BindingInput.HEAD_PITCH_DELTA, .01f);
            assertEquals(id, -6.5f, bus.consumeMouseRelativeY(), .001f);
        }
    }

    @Test
    public void doomHeadTiltNavigatesMenuAndMovesWithNeutralDeadzone() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine engine = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_DOS_DOOM_SHAREWARE));
        // Up is forward and menu UP; Down is backward and menu DOWN.
        engine.submitAxis(BindingInput.HEAD_PITCH_UP_AXIS, .18f);
        assertFalse(bus.isKeyDown(VirtualKey.UP));
        assertFalse(bus.isKeyDown(VirtualKey.DOWN));
        engine.submitAxis(BindingInput.HEAD_PITCH_UP_AXIS, .27f);
        org.junit.Assert.assertTrue(bus.isKeyDown(VirtualKey.UP));
        assertFalse(bus.isKeyDown(VirtualKey.DOWN));
        engine.submitAxis(BindingInput.HEAD_PITCH_UP_AXIS, .17f);
        org.junit.Assert.assertTrue(bus.isKeyDown(VirtualKey.UP));
        engine.submitAxis(BindingInput.HEAD_PITCH_UP_AXIS, .11f);
        assertFalse(bus.isKeyDown(VirtualKey.UP));
        engine.submitAxis(BindingInput.HEAD_PITCH_UP_AXIS, -.26f);
        org.junit.Assert.assertTrue(bus.isKeyDown(VirtualKey.DOWN));
        assertFalse(bus.isKeyDown(VirtualKey.UP));
        engine.submitAxis(BindingInput.HEAD_PITCH_UP_AXIS, 0.0f);
        assertFalse(bus.isKeyDown(VirtualKey.DOWN));

        // Touchpad and headset may hold the same key simultaneously.
        engine.submitAxis(BindingInput.HEAD_PITCH_UP_AXIS, .4f);
        engine.submitAxis(BindingInput.TOUCHPAD_Y, -.8f);
        engine.submitAxis(BindingInput.HEAD_PITCH_UP_AXIS, 0f);
        org.junit.Assert.assertTrue(bus.isKeyDown(VirtualKey.UP));
        engine.submitAxis(BindingInput.TOUCHPAD_Y, 0f);
        assertFalse(bus.isKeyDown(VirtualKey.UP));

        // Only horizontal Doom guest turning was restored; pitch stays held.
        engine.submitRelative(BindingInput.HEAD_YAW_DELTA, .01f);
        assertEquals(6.5f, bus.consumeMouseRelativeX(), .001f);
        assertEquals(0f, bus.consumeMouseRelativeY(), .001f);
    }

    @Test
    public void doomTouchpadAndAppMouseButtonsHoldIndependently() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine e = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_DOS_DOOM_SHAREWARE));
        e.submitDigital(BindingInput.SELECT, true);
        e.submitDigital(BindingInput.DAYDREAM_APP_BUTTON, true);
        org.junit.Assert.assertTrue(bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));
        org.junit.Assert.assertTrue(bus.isMouseButtonDown(VirtualOutput.MOUSE_RIGHT));
        e.submitDigital(BindingInput.SELECT, false);
        org.junit.Assert.assertFalse(bus.isMouseButtonDown(VirtualOutput.MOUSE_LEFT));
        org.junit.Assert.assertTrue(bus.isMouseButtonDown(VirtualOutput.MOUSE_RIGHT));
        e.submitDigital(BindingInput.DAYDREAM_APP_BUTTON, false);
        org.junit.Assert.assertFalse(bus.isMouseButtonDown(VirtualOutput.MOUSE_RIGHT));
    }

    @Test
    public void doomTouchpadDirectionsAreHeldWhileTouched() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine e = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_DOS_DOOM_SHAREWARE));
        for (int i = 0; i < 3; i++) {
            e.submitAxis(BindingInput.TOUCHPAD_X, -.8f);
            e.submitAxis(BindingInput.TOUCHPAD_Y, -.8f);
            org.junit.Assert.assertTrue(bus.isKeyDown(VirtualKey.LEFT));
            org.junit.Assert.assertTrue(bus.isKeyDown(VirtualKey.UP));
        }
        e.submitAxis(BindingInput.TOUCHPAD_X, 0f);
        e.submitAxis(BindingInput.TOUCHPAD_Y, 0f);
        org.junit.Assert.assertFalse(bus.isKeyDown(VirtualKey.LEFT));
        org.junit.Assert.assertFalse(bus.isKeyDown(VirtualKey.UP));
    }

    @Test
    public void doomVolumeCyclesNumericWeaponsNotMenuKeys() {
        VirtualInputBus bus = new VirtualInputBus();
        InputBindingEngine e = new InputBindingEngine(bus,
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_DOS_DOOM_SHAREWARE));
        VirtualInputSnapshot frame = new VirtualInputSnapshot();
        e.submitDigital(BindingInput.VOLUME_UP, true);
        e.submitDigital(BindingInput.VOLUME_UP, true);
        bus.capture(frame);
        org.junit.Assert.assertTrue(frame.keys[VirtualKey.DIGIT_3.ordinal()]);
        org.junit.Assert.assertFalse(frame.keys[VirtualKey.DIGIT_4.ordinal()]);
        org.junit.Assert.assertFalse(frame.keys[VirtualKey.ENTER.ordinal()]);
        org.junit.Assert.assertFalse(frame.keys[VirtualKey.ESCAPE.ordinal()]);
        e.submitDigital(BindingInput.VOLUME_UP, false);
        e.submitDigital(BindingInput.VOLUME_DOWN, true);
        bus.capture(frame);
        org.junit.Assert.assertTrue(frame.keys[VirtualKey.DIGIT_2.ordinal()]);
        e.submitDigital(BindingInput.VOLUME_DOWN, false);
        e.releaseAll();
        e.submitDigital(BindingInput.VOLUME_UP, true);
        bus.capture(frame);
        org.junit.Assert.assertTrue(frame.keys[VirtualKey.DIGIT_3.ordinal()]);
    }

    @Test
    public void previousStockDoomProfileUpgradesButTuningSurvives() {
        BindingProfile current = BuiltInBindingProfiles.byId(
            BuiltInBindingProfiles.ID_DOS_DOOM_SHAREWARE);
        java.util.List<InputBinding> previous = new java.util.ArrayList<>();
        for (InputBinding b : current.bindings) {
            if (b.input == BindingInput.DAYDREAM_APP_BUTTON) continue;
            previous.add(b);
            if (b.input == BindingInput.SELECT
                && b.output.equals(VirtualOutput.joystickButton(0))) {
                previous.add(InputBinding.digital(BindingInput.VOLUME_UP,
                    VirtualOutput.key(VirtualKey.ENTER)));
                previous.add(InputBinding.digital(BindingInput.VOLUME_DOWN,
                    VirtualOutput.key(VirtualKey.ESCAPE)));
            }
        }
        BindingProfile stock = BindingProfileCodec.decode(
            BindingProfileCodec.encode(current.withBindings(previous)));
        org.junit.Assert.assertSame(current,
            BuiltInBindingProfiles.upgradeLegacyStockHeadMouse(stock));
        BindingProfile tuned = BindingProfileTuner.adjustAnalogScale(stock, 1.1f);
        org.junit.Assert.assertSame(tuned,
            BuiltInBindingProfiles.upgradeLegacyStockHeadMouse(tuned));
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
