package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import java.io.IOException;
import static org.junit.Assert.*;

public final class LaptopSharePathTest {
    @Test public void acceptsOrdinaryWindowsSharePaths() throws Exception {
        assertEquals("192.168.137.1",LaptopSharePath.host("192.168.137.1"));
        assertEquals("LAN-LAPTOP",LaptopSharePath.host("LAN-LAPTOP"));
        assertEquals("DOS Games",LaptopSharePath.share("DOS Games"));
        assertEquals("Games\\Doom.zip",LaptopSharePath.child("Games","Doom.zip"));
        assertEquals("Games",LaptopSharePath.parent("Games\\Doom.zip"));
        assertEquals("Doom.zip",LaptopSharePath.fileName("Games\\Doom.zip"));
        assertEquals("",LaptopSharePath.parent("Doom.zip"));
    }
    private static void reject(String path) {
        try{LaptopSharePath.directory(path);fail("accepted "+path);}
        catch(IOException expected){}
    }
    @Test public void preventsTraversalAndUncPaths() {
        reject("..");
        reject("one\\..\\game.iso");
        reject("\\\\server\\share");
        reject("dir\\");
        reject("dir\\\\game.iso");
        reject("dir/other");
        reject("file:");
    }
    @Test public void rejectsBadServerAndShareNames() {
        try{LaptopSharePath.host("evil.invalid/path");fail();}
        catch(IOException expected){}
        try{LaptopSharePath.share("C:\\");fail();}
        catch(IOException expected){}
    }
    @Test public void rejectsUnsupportedContentOnBrowse() {
        assertTrue(DosContentSupport.isSupportedFileName("demo.zip"));
        assertFalse(DosContentSupport.isSupportedFileName("malware.dll"));
    }
}
