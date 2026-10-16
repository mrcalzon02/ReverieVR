package io.github.mrcalzon02.reverievr;
import static org.junit.Assert.*;
import org.junit.Test;
public class NativeDevelopmentAdmissionTest {
    @Test public void newShooterAndBarAreDevOnly() {
        assertTrue(NativeModuleRuntime.isDevelopmentOnly(NativeModuleRuntime.ID_BREAKWATER_BATTERY));
        assertTrue(NativeModuleRuntime.isDevelopmentOnly(NativeModuleRuntime.ID_GUMBALL_ARMADA));
        assertTrue(NativeModuleRuntime.isDevelopmentOnly(NativeModuleRuntime.ID_OMEGA_FRONT));
        assertTrue(NativeModuleRuntime.isDevelopmentOnly(NativeModuleRuntime.ID_RED_LEDGER));
        assertTrue(NativeModuleRuntime.isDevelopmentOnly(NativeModuleRuntime.ID_LANTERN_DESK));
        assertTrue(NativeModuleRuntime.isDevelopmentOnly(NativeModuleRuntime.ID_FORWARD_DETACHMENT));
        assertFalse(NativeModuleRuntime.isDevelopmentOnly("procedural-test-chamber"));
        assertEquals(BuiltInBindingProfiles.ID_NATIVE_CLASSICS,
            BuiltInBindingProfiles.nativeProfileForModule(NativeModuleRuntime.ID_GUMBALL_ARMADA));
        assertEquals(BuiltInBindingProfiles.ID_NATIVE_CLASSICS,
            BuiltInBindingProfiles.nativeProfileForModule(NativeModuleRuntime.ID_OMEGA_FRONT));
        assertEquals(BuiltInBindingProfiles.ID_NATIVE_STANDARD,
            BuiltInBindingProfiles.nativeProfileForModule(NativeModuleRuntime.ID_BREAKWATER_BATTERY));
    }
}