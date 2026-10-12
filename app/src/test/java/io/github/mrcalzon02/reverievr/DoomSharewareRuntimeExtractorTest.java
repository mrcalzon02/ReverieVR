package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public final class DoomSharewareRuntimeExtractorTest {
    @Rule public TemporaryFolder temporary = new TemporaryFolder();

    private byte[] zipWith(String entryName) throws IOException {
        File zip=temporary.newFile("fixture.zip");
        try(ZipOutputStream out=new ZipOutputStream(
            new FileOutputStream(zip))) {
            out.putNextEntry(new ZipEntry(entryName));
            out.write(new byte[]{1,2,3,4,5});
            out.closeEntry();
        }
        return Files.readAllBytes(zip.toPath());
    }
    private File[] split(byte[] zip, boolean sfxPrefix) throws IOException {
        byte[] wrapped=zip;
        if(sfxPrefix) {
            wrapped=new byte[23+zip.length];
            Arrays.fill(wrapped,0,23,(byte)'M');
            System.arraycopy(zip,0,wrapped,23,zip.length);
        }
        final int middle=wrapped.length/2;
        File first=temporary.newFile("DOOMS_19.1");
        File second=temporary.newFile("DOOMS_19.2");
        try(FileOutputStream out=new FileOutputStream(first)){
            out.write(wrapped,0,middle);
        }
        try(FileOutputStream out=new FileOutputStream(second)){
            out.write(wrapped,middle,wrapped.length-middle);
        }
        return new File[]{first,second};
    }
    @Test public void extractsSplitArchiveWithoutKeyboardOrDos() throws Exception {
        File[] parts=split(zipWith("DOOM.EXE"),true);
        File destination=temporary.newFolder("installed");
        BoundedZipExtractor.ExtractionResult result=
            DoomSharewareRuntimeExtractor.extract(parts[0],parts[1],destination);
        assertEquals(1,result.entryCount);
        assertEquals(5L,result.totalBytes);
        assertArrayEquals(new byte[]{1,2,3,4,5},
            Files.readAllBytes(new File(destination,"DOOM.EXE").toPath()));
    }
    @Test public void rejectsPathTraversal() throws Exception {
        File[] parts=split(zipWith("../outside.exe"),false);
        File destination=temporary.newFolder("blocked");
        try {
            DoomSharewareRuntimeExtractor.extract(parts[0],parts[1],destination);
            fail("Unsafe archive escaped extraction root.");
        } catch(IOException expected) {
            assertFalse(new File(temporary.getRoot(),"outside.exe").exists());
        }
    }
    @Test public void invalidPayloadFailsRatherThanRunningInstaller() throws Exception {
        File[] parts=split(new byte[]{4,3,2,1,0},false);
        try {
            DoomSharewareRuntimeExtractor.extract(
                parts[0],parts[1],temporary.newFolder("invalid"));
            fail("Missing ZIP signature accepted.");
        } catch(IOException expected) {
            assertTrue(expected.getMessage().contains("no supported ZIP payload"));
        }
    }
}
