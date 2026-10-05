package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

import org.junit.Test;

public final class BoundedZipExtractorTest {
    @Test
    public void extractsFilesWithinRootAndReportsBounds()
        throws Exception {
        File root =
            Files.createTempDirectory("rv-zip-ok")
                .toFile();
        try {
            Map<String, byte[]> entries =
                new LinkedHashMap<>();
            entries.put(
                "INSTALL.BAT",
                "echo install".getBytes(StandardCharsets.US_ASCII)
            );
            entries.put(
                "DATA/DOOM.DAT",
                new byte[] {1, 2, 3, 4}
            );

            BoundedZipExtractor.ExtractionResult result =
                BoundedZipExtractor.extract(
                    new ByteArrayInputStream(zip(entries)),
                    root,
                    8,
                    1024
                );

            assertEquals(2, result.entryCount);
            assertEquals(16L, result.totalBytes);
            assertTrue(
                new File(root, "INSTALL.BAT").isFile()
            );
            assertTrue(
                new File(root, "DATA/DOOM.DAT").isFile()
            );
        } finally {
            deleteRecursively(root);
        }
    }

    @Test
    public void rejectsTraversalEntries()
        throws Exception {
        File parent =
            Files.createTempDirectory("rv-zip-parent")
                .toFile();
        File root = new File(parent, "runtime");
        File escaped = new File(parent, "escaped.txt");

        try {
            Map<String, byte[]> entries =
                new LinkedHashMap<>();
            entries.put(
                "../escaped.txt",
                "bad".getBytes(StandardCharsets.US_ASCII)
            );

            try {
                BoundedZipExtractor.extract(
                    new ByteArrayInputStream(zip(entries)),
                    root,
                    8,
                    1024
                );
                fail("Traversal entry should fail.");
            } catch (IOException expected) {
                assertFalse(escaped.exists());
            }
        } finally {
            deleteRecursively(parent);
        }
    }

    @Test
    public void rejectsUncompressedSizeOverflow()
        throws Exception {
        File root =
            Files.createTempDirectory("rv-zip-limit")
                .toFile();

        try {
            Map<String, byte[]> entries =
                new LinkedHashMap<>();
            entries.put(
                "BIG.DAT",
                new byte[2048]
            );

            try {
                BoundedZipExtractor.extract(
                    new ByteArrayInputStream(zip(entries)),
                    root,
                    8,
                    128
                );
                fail("Oversized extraction should fail.");
            } catch (IOException expected) {
                assertFalse(
                    new File(root, "BIG.DAT").exists()
                );
            }
        } finally {
            deleteRecursively(root);
        }
    }

    private static byte[] zip(
        Map<String, byte[]> entries
    ) throws IOException {
        ByteArrayOutputStream output =
            new ByteArrayOutputStream();
        try (ZipOutputStream zip =
                 new ZipOutputStream(output)) {
            for (Map.Entry<String, byte[]> entry
                : entries.entrySet()) {
                zip.putNextEntry(
                    new ZipEntry(entry.getKey())
                );
                zip.write(entry.getValue());
                zip.closeEntry();
            }
        }
        return output.toByteArray();
    }

    private static void deleteRecursively(File file) {
        if (file == null || !file.exists()) {
            return;
        }
        if (file.isDirectory()) {
            File[] children = file.listFiles();
            if (children != null) {
                for (File child : children) {
                    deleteRecursively(child);
                }
            }
        }
        file.delete();
    }
}
