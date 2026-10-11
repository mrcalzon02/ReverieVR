package io.github.mrcalzon02.reverievr;
import org.junit.Test;
import static org.junit.Assert.*;

public class ScrcpyHidIdentityTest {
    @Test public void recognizesScrcpyDevices() {
        assertTrue(ScrcpyHidIdentity.matchesName("scrcpy"));
        assertTrue(ScrcpyHidIdentity.matchesName("scrcpy keyboard"));
        assertTrue(ScrcpyHidIdentity.matchesName("scrcpy mouse"));
        assertTrue(ScrcpyHidIdentity.matchesName("scrcpy gamepad"));
        assertTrue(ScrcpyHidIdentity.matchesName("Scrcpy UHID Keyboard"));
        assertTrue(ScrcpyHidIdentity.matchesName("scrcpy-uhid-mouse"));
        assertTrue(ScrcpyHidIdentity.matchesName(" SCRCPY_MOUSE "));
    }
    @Test public void explicitlyRecognizesUpstreamScrcpyXboxCompatibleGamepad() {
        // Genymobile/scrcpy app/src/uhid/gamepad_uhid.c:
        // the UHID gamepad identifies itself as Microsoft X-Box 360 Pad.
        assertTrue(ScrcpyHidIdentity.matchesCompatibleGamepad(
            0x045e, 0x028e, "Microsoft X-Box 360 Pad"));
        assertTrue(ScrcpyHidIdentity.matchesCompatibleGamepad(
            0x045e, 0x028e, "microsoft x-box 360 pad"));
        assertFalse(ScrcpyHidIdentity.matchesCompatibleGamepad(
            0x045e, 0x028e, "scrcpy"));
        assertFalse(ScrcpyHidIdentity.matchesCompatibleGamepad(
            0x045e, 0x028e, "Xbox Wireless Controller"));
        assertFalse(ScrcpyHidIdentity.matchesCompatibleGamepad(
            0x1234, 0x028e, "Microsoft X-Box 360 Pad"));
        assertFalse(ScrcpyHidIdentity.matchesCompatibleGamepad(
            0x045e, 0x1234, "Microsoft X-Box 360 Pad"));
        assertFalse(ScrcpyHidIdentity.matchesCompatibleGamepad(
            0x045e, 0x028e, null));
    }
    @Test public void rejectsOrdinaryDevices() {
        assertFalse(ScrcpyHidIdentity.matchesName(null));
        assertFalse(ScrcpyHidIdentity.matchesName(""));
        assertFalse(ScrcpyHidIdentity.matchesName("UHID Mouse"));
        assertFalse(ScrcpyHidIdentity.matchesName("Keyboard"));
        assertFalse(ScrcpyHidIdentity.matchesName("SteamController"));
        assertFalse(ScrcpyHidIdentity.matchesName("myscrcpy mouse"));
        assertFalse(ScrcpyHidIdentity.matchesName("scrcpyKeyboard"));
    }
}
