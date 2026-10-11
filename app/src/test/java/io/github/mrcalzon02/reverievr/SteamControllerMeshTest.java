package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.List;
import static org.junit.Assert.*;

public class SteamControllerMeshTest {
    @Test public void rejectsIncorrectMagic() throws Exception {
        try {
            SteamControllerMesh.read(new ByteArrayInputStream(
                new byte[]{0,1,2,3,4,5,6,7,8,9,10,11}));
            fail("bad magic accepted");
        } catch (IOException expected) {}
    }
    @Test public void rejectsMissingParts() throws Exception {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        DataOutputStream out = new DataOutputStream(bytes);
        out.write("RVSC0001".getBytes("US-ASCII"));
        out.writeInt(Integer.reverseBytes(1));
        try {
            SteamControllerMesh.read(new ByteArrayInputStream(bytes.toByteArray()));
            fail("one-part asset accepted");
        } catch (IOException expected) {}
    }
    @Test public void identifiesOnlySpecificValveController() {
        assertTrue(SteamControllerIdentity.matches(0x28de, "Steam Controller"));
        assertFalse(SteamControllerIdentity.matches(0x28de, "Steam Deck"));
        assertFalse(SteamControllerIdentity.matches(0x1234, "Steam Controller"));
        assertFalse(SteamControllerIdentity.matches(0x28de, "Valve HID"));
        assertTrue(SteamControllerIdentity.matchesAndroidHid(0, "Steam Controller"));
        assertTrue(SteamControllerIdentity.matchesAndroidHid(0x28de, "Valve Steam Controller"));
        assertFalse(SteamControllerIdentity.matchesAndroidHid(0x1234, "Steam Controller"));
        assertFalse(SteamControllerIdentity.matchesAndroidHid(0, "Steam Deck"));
        assertFalse(SteamControllerIdentity.matchesAndroidHid(0, "Steam Controller 2026"));
    }
}
