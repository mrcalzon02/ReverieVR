package io.github.mrcalzon02.reverievr;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/** Exports the APK-bundled, human-readable Windows helper, never executes it. */
final class LaptopLinkBundle {
    static final String EXPORT_NAME = "ReverieVR-Laptop-Link.zip";
    private static final String ASSET_DIRECTORY = "reverie-link/";
    private static final String ZIP_DIRECTORY = "ReverieLink/";
    private static final long MAX_FILE_BYTES = 128 * 1024L;
    private static final String[] INCLUDED = {
        "README.txt", "Start-ReverieLink.cmd", "ReverieLink.ps1",
        "SteamVrPreflight.ps1"
    };

    interface Assets {
        InputStream open(String path) throws IOException;
    }

    private LaptopLinkBundle() {}

    static void write(Assets assets, OutputStream output) throws IOException {
        if (assets == null || output == null) {
            throw new IOException("Laptop Link export output unavailable.");
        }
        try (ZipOutputStream zip = new ZipOutputStream(output)) {
            byte[] block = new byte[8192];
            for (String name : INCLUDED) {
                // Hardcoded whitelist: no filesystem traversal or arbitrary APK
                // material can enter the publicly exported Windows bundle.
                ZipEntry entry = new ZipEntry(ZIP_DIRECTORY + name);
                entry.setTime(0L);
                zip.putNextEntry(entry);
                long size = 0L;
                try (InputStream asset = assets.open(ASSET_DIRECTORY + name)) {
                    if (asset == null) throw new IOException("Missing laptop-link asset.");
                    int count;
                    while ((count = asset.read(block)) != -1) {
                        if (count <= 0) continue;
                        size += count;
                        if (size > MAX_FILE_BYTES) {
                            throw new IOException("Bundled laptop utility exceeds size cap.");
                        }
                        zip.write(block, 0, count);
                    }
                }
                if (size == 0L) throw new IOException("Empty laptop-link asset.");
                zip.closeEntry();
            }
            zip.finish();
        }
    }
}
