package io.github.mrcalzon02.reverievr;
import static org.junit.Assert.*;
import org.junit.Test;
public class NativeDevelopmentAdmissionTest {
    @Test public void newShooterAndBarAreDevOnly() {
        assertTrue(NativeModuleRuntime.isDevelopmentOnly(NativeModuleRuntime.ID_BREAKWATER_BATTERY));
        assertTrue(NativeModuleRuntime.isDevelopmentOnly(NativeModuleRuntime.ID_RED_LEDGER));
        assertFalse(NativeModuleRuntime.isDevelopmentOnly("procedural-test-chamber"));
    }
}