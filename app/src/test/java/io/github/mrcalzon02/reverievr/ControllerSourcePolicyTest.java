package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

public final class ControllerSourcePolicyTest {
    @Test public void explicitScrcpyDoesNotAcceptDaydreamSteamOrUnrelatedPad() {
        String s=ReveriePreferences.CONTROLLER_SCRCPY;
        assertFalse(ControllerSourcePolicy.acceptsProvider(s,"daydream_ble"));
        assertTrue(ControllerSourcePolicy.acceptsGamepad(s,false,false,true,false,false,false));
        assertTrue(ControllerSourcePolicy.acceptsGamepad(s,false,false,false,true,true,false));
        assertFalse(ControllerSourcePolicy.acceptsGamepad(s,false,true,false,false,true,true));
        assertFalse(ControllerSourcePolicy.acceptsGamepad(s,false,false,false,false,true,false));
    }

    @Test public void explicitDaydreamRejectsEveryAndroidGamepad() {
        String s=ReveriePreferences.CONTROLLER_DAYDREAM;
        assertTrue(ControllerSourcePolicy.acceptsProvider(s,"daydream_ble"));
        assertFalse(ControllerSourcePolicy.acceptsProvider(s,"daydream_phone_emulator"));
        for (boolean steam: new boolean[]{false,true})
            assertFalse(ControllerSourcePolicy.acceptsGamepad(s,true,steam,true,true,true,steam));
    }

    @Test public void autoPrioritizesReadyProviderThenSteamThenHid() {
        String s=ReveriePreferences.CONTROLLER_AUTO;
        assertTrue(ControllerSourcePolicy.acceptsProvider(s,"daydream_ble"));
        assertFalse(ControllerSourcePolicy.acceptsGamepad(s,true,false,true,true,true,false));
        assertFalse(ControllerSourcePolicy.acceptsGamepad(s,false,false,true,true,true,true));
        assertTrue(ControllerSourcePolicy.acceptsGamepad(s,false,true,false,false,true,true));
        assertTrue(ControllerSourcePolicy.acceptsGamepad(s,false,false,false,false,true,false));
    }

    @Test public void androidPadAndPhoneAreSeparateAndChoicesAreUnique() {
        String android=ReveriePreferences.CONTROLLER_ANDROID;
        assertTrue(ControllerSourcePolicy.acceptsGamepad(android,false,false,false,false,true,false));
        assertFalse(ControllerSourcePolicy.acceptsGamepad(android,false,false,true,false,false,false));
        String phone=ReveriePreferences.CONTROLLER_PHONE;
        assertTrue(ControllerSourcePolicy.acceptsProvider(phone,"daydream_phone_emulator"));
        assertFalse(ControllerSourcePolicy.acceptsGamepad(phone,false,false,false,false,true,false));
        java.util.HashSet<String> seen=new java.util.HashSet<>();
        for(String choice:ControllerSourcePolicy.CHOICES) {
            assertTrue(seen.add(choice));
            assertFalse(ControllerSourcePolicy.label(choice).isEmpty());
        }
        assertEquals(6,seen.size());
    }
}
