package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;

import org.junit.Test;

public final class BuiltInBindingProfilesTest {
    @Test
    public void redLedgerUsesSelectOnly() {
        BindingProfile profile =
            BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_NATIVE_RED_LEDGER
            );

        assertNotNull(profile);
        assertEquals(
            BuiltInBindingProfiles.ID_NATIVE_RED_LEDGER,
            profile.id
        );
        assertEquals(1, profile.bindings.size());

        InputBinding binding =
            profile.bindings.get(0);
        assertEquals(
            BindingInput.SELECT,
            binding.input
        );
        assertEquals(
            VirtualOutputKind.JOYSTICK_BUTTON,
            binding.output.kind
        );
        assertEquals(
            0,
            binding.output.code
        );
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
