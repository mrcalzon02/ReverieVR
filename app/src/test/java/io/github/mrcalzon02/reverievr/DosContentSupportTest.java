package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public final class DosContentSupportTest {
    @Test
    public void acceptsAdvertisedDosboxPureContentTypes() {
        assertTrue(DosContentSupport.isSupportedFileName("doom.zip"));
        assertTrue(DosContentSupport.isSupportedFileName("DOOM.EXE"));
        assertTrue(DosContentSupport.isSupportedFileName("setup.com"));
        assertTrue(DosContentSupport.isSupportedFileName("start.bat"));
        assertTrue(DosContentSupport.isSupportedFileName("disc.chd"));
        assertTrue(DosContentSupport.isSupportedFileName("game.cue"));
        assertTrue(DosContentSupport.isSupportedFileName("disk.vhd"));
        assertTrue(DosContentSupport.isSupportedFileName("game.conf"));
    }

    @Test
    public void rejectsUnadvertisedContentTypes() {
        assertFalse(DosContentSupport.isSupportedFileName("notes.txt"));
        assertFalse(DosContentSupport.isSupportedFileName("game.apk"));
        assertFalse(DosContentSupport.isSupportedFileName("noextension"));
    }

    @Test
    public void extensionMatchingIsCaseInsensitive() {
        assertEquals(
            "exe",
            DosContentSupport.extensionOf("GAME.ExE")
        );
    }
}
