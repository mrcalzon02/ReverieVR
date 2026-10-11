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
