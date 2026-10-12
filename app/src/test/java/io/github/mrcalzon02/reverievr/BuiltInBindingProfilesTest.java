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

}
