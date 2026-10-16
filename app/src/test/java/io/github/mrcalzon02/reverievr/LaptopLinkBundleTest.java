package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public final class LaptopLinkBundleTest {
    @Test public void zipContainsOnlyWhitelistedPortableHostFiles() throws Exception {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        LaptopLinkBundle.write(name -> new ByteArrayInputStream(
            ("Content for "+name).getBytes(StandardCharsets.UTF_8)), bytes);
        List<String> files=new ArrayList<>();
        try (ZipInputStream zip = new ZipInputStream(
                new ByteArrayInputStream(bytes.toByteArray()))) {
            ZipEntry entry;
            while((entry=zip.getNextEntry())!=null) {
                assertFalse(entry.getName().contains(".."));
                byte[] value=zip.readAllBytes();
                assertEquals("Content for reverie-link/"+
                    entry.getName().substring("ReverieLink/".length()),
                    new String(value,StandardCharsets.UTF_8));
                files.add(entry.getName());
            }
        }
        assertEquals(java.util.Arrays.asList(
            "ReverieLink/README.txt",
            "ReverieLink/Start-ReverieLink.cmd",
            "ReverieLink/ReverieLink.ps1",
            "ReverieLink/SteamVrPreflight.ps1",
            "ReverieLink/SteamVrLinkProbeHost.ps1"),files);
    }

    @Test public void brokenAssetOrExcessivePayloadFailsClosed() throws Exception {
        try {
            LaptopLinkBundle.write(name -> { throw new IOException("missing"); },
                new ByteArrayOutputStream());
            fail("Missing host asset accepted");
        } catch (IOException expected) {
            assertTrue(expected.getMessage().contains("missing"));
        }
        byte[] tooLarge=new byte[130*1024];
        try {
            LaptopLinkBundle.write(name -> new ByteArrayInputStream(tooLarge),
                new ByteArrayOutputStream());
            fail("Oversize helper accepted");
        } catch (IOException expected) {
            assertTrue(expected.getMessage().contains("size cap"));
        }
    }
}
