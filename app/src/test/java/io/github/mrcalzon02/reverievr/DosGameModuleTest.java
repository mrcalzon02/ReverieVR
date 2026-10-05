package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertTrue;

import java.io.File;
import java.nio.file.Files;

import org.junit.Test;

public final class DosGameModuleTest {
    @Test
    public void readableDirectoryIsValidContentRoot()
        throws Exception {
        File root =
            Files.createTempDirectory("rv-dos-dir")
                .toFile();
        try {
            DosGameModule module =
                new DosGameModule(
                    "test-directory",
                    "Test directory",
                    "source.zip",
                    root.getAbsolutePath(),
                    "test-profile",
                    0L
                );

            assertTrue(module.isContentPresent());
            assertTrue(module.isDirectoryContent());
        } finally {
            root.delete();
        }
    }
}
