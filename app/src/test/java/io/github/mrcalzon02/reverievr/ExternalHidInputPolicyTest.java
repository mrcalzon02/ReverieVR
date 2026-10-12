package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public class ExternalHidInputPolicyTest {
    @Test public void enabledDoesNotBlockKeyboardOrMouse() {
        assertFalse(ExternalHidInputPolicy.shouldConsume(true,true,false,false,false));
        assertFalse(ExternalHidInputPolicy.shouldConsume(true,false,true,false,false));
    }
    @Test public void disabledBlocksOnlyExternalHidTraffic() {
        assertTrue(ExternalHidInputPolicy.shouldConsume(false,true,false,false,false));
        assertTrue(ExternalHidInputPolicy.shouldConsume(false,false,true,false,false));
        assertFalse(ExternalHidInputPolicy.shouldConsume(false,false,false,false,false));
        assertFalse(ExternalHidInputPolicy.shouldConsume(false,true,true,true,false));
    }
    @Test public void physicalAndScrcpyKeyboardsEditHomeWhenEnabled() {
        assertTrue(ExternalHidInputPolicy.shouldEditHomeKeyboard(
            true,true,true,true,false,false)); // Bluetooth, USB or scrcpy UHID
        assertFalse(ExternalHidInputPolicy.shouldEditHomeKeyboard(
            false,true,true,true,false,false)); // user disabled external input
    }
    @Test public void guestSessionsAndControllerKeysKeepTheirOwnership() {
        assertFalse(ExternalHidInputPolicy.shouldEditHomeKeyboard(
            true,false,true,true,false,false)); // DOS/native scene, not Home
        assertFalse(ExternalHidInputPolicy.shouldEditHomeKeyboard(
            true,true,true,true,true,false)); // Steam/gamepad hybrid
        assertFalse(ExternalHidInputPolicy.shouldEditHomeKeyboard(
            true,true,true,false,false,false)); // Android virtual keyboard
        assertFalse(ExternalHidInputPolicy.shouldEditHomeKeyboard(
            true,true,false,true,false,false)); // mouse alone
        assertFalse(ExternalHidInputPolicy.shouldEditHomeKeyboard(
            true,true,true,true,false,true)); // Ctrl/Alt/Meta shortcut
    }
    @Test public void shellRecoveryStaysAvailableEvenWhenDisabled() {
        assertFalse(ExternalHidInputPolicy.shouldConsume(false,true,false,false,true));
        assertFalse(ExternalHidInputPolicy.shouldConsume(false,false,true,false,true));
    }
}
