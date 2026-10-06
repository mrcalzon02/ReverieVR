package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;

import org.junit.Test;

public final class BuiltInBindingProfilesTest {
    @Test
    public void redLedgerUsesControllerButtons() {
        BindingProfile profile =
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_NATIVE_RED_LEDGER
            );

        assertNotNull(profile);
        assertEquals(
            BuiltInBindingProfiles.ID_NATIVE_RED_LEDGER,
            profile.id
        );
        assertEquals(3, profile.bindings.size());

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
    public void nativeProfilesDoNotLeakIntoDosProfileList() {
        assertFalse(
            BuiltInBindingProfiles.isDosProfileId(
                BuiltInBindingProfiles.ID_NATIVE_TEST_CHAMBER
            )
        );
        assertFalse(
            BuiltInBindingProfiles.isDosProfileId(
                BuiltInBindingProfiles.ID_NATIVE_RED_LEDGER
            )
        );
    }
}
