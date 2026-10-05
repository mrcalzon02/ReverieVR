package io.github.mrcalzon02.reverievr;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

final class BoundedZipExtractor {
    private static final int BUFFER_SIZE = 16 * 1024;

    private BoundedZipExtractor() {
    }

    static ExtractionResult extract(
        InputStream source,
        File destination,
        int maxEntries,
        long maxUncompressedBytes
    ) throws IOException {
        if (source == null) {
            throw new IllegalArgumentException(
                "ZIP source is required."
            );
        }
        if (destination == null) {
            throw new IllegalArgumentException(
                "ZIP destination is required."
            );
        }
        if (maxEntries <= 0 || maxUncompressedBytes <= 0L) {
            throw new IllegalArgumentException(
                "ZIP extraction bounds must be positive."
            );
        }
        if (!destination.isDirectory()
            && !destination.mkdirs()) {
            throw new IOException(
                "Could not create ZIP extraction directory."
            );
        }

        File canonicalRoot =
            destination.getCanonicalFile();
        String rootPrefix =
            canonicalRoot.getPath() + File.separator;
        byte[] buffer = new byte[BUFFER_SIZE];
        int entries = 0;
        long totalBytes = 0L;

        try (ZipInputStream zip =
                 new ZipInputStream(source)) {
            ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) {
                if (++entries > maxEntries) {
                    throw new IOException(
                        "ZIP contains too many entries."
                    );
                }

                File target =
                    new File(canonicalRoot, entry.getName())
                        .getCanonicalFile();
                String targetPath = target.getPath();
                if (!targetPath.equals(canonicalRoot.getPath())
                    && !targetPath.startsWith(rootPrefix)) {
                    throw new IOException(
                        "ZIP entry escapes extraction root: "
                            + entry.getName()
                    );
                }

                if (entry.isDirectory()) {
                    if (!target.isDirectory()
                        && !target.mkdirs()) {
                        throw new IOException(
                            "Could not create ZIP directory: "
                                + entry.getName()
                        );
                    }
                    zip.closeEntry();
                    continue;
                }

                File parent = target.getParentFile();
                if (parent != null
                    && !parent.isDirectory()
                    && !parent.mkdirs()) {
                    throw new IOException(
                        "Could not create ZIP parent directory."
                    );
                }

                try (FileOutputStream output =
                         new FileOutputStream(target)) {
                    int read;
                    while ((read = zip.read(buffer)) != -1) {
                        totalBytes += read;
                        if (totalBytes > maxUncompressedBytes) {
                            throw new IOException(
                                "ZIP exceeds extraction size limit."
                            );
                        }
                        output.write(buffer, 0, read);
                    }
                    output.getFD().sync();
                } catch (IOException exception) {
                    target.delete();
                    throw exception;
                }

                zip.closeEntry();
            }
        }

        return new ExtractionResult(
            entries,
            totalBytes
        );
    }

    static final class ExtractionResult {
        final int entryCount;
        final long totalBytes;

        ExtractionResult(
            int entryCount,
            long totalBytes
        ) {
            this.entryCount = entryCount;
            this.totalBytes = totalBytes;
        }
    }
}
