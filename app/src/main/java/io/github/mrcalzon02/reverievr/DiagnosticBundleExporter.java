package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.net.Uri;
import android.os.Build;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.Arrays;
import java.util.Comparator;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

final class DiagnosticBundleExporter {
    private static final int BUFFER_SIZE = 64 * 1024;

    private final Context context;

    DiagnosticBundleExporter(Context context) {
        this.context = context.getApplicationContext();
    }

    void export(Uri destination) throws IOException {
        if (destination == null) {
            throw new IOException(
                "Diagnostic export destination is missing."
            );
        }

        OutputStream raw =
            context.getContentResolver()
                .openOutputStream(destination, "w");
        if (raw == null) {
            throw new IOException(
                "Android could not open the diagnostic export destination."
            );
        }

        try (ZipOutputStream zip =
                 new ZipOutputStream(
                     new BufferedOutputStream(raw)
                 )) {
            addManifest(zip);
            addLogFiles(zip);
        }
    }

    private void addManifest(ZipOutputStream zip)
        throws IOException {
        StringBuilder manifest =
            new StringBuilder(1024);

        manifest.append("ReverieVR diagnostic bundle\n")
            .append("created=")
            .append(Instant.now())
            .append('\n')
            .append("appVersion=")
            .append(BuildConfig.VERSION_NAME)
            .append('\n')
            .append("versionCode=")
            .append(BuildConfig.VERSION_CODE)
            .append('\n')
            .append("buildType=")
            .append(BuildConfig.BUILD_TYPE)
            .append('\n')
            .append("dosRuntimeBuilt=")
            .append(BuildConfig.DOS_RUNTIME_BUILT)
            .append('\n')
            .append("loggingMode=")
            .append(ReverieLog.getMode().name())
            .append('\n')
            .append("manufacturer=")
            .append(Build.MANUFACTURER)
            .append('\n')
            .append("model=")
            .append(Build.MODEL)
            .append('\n')
            .append("device=")
            .append(Build.DEVICE)
            .append('\n')
            .append("androidRelease=")
            .append(Build.VERSION.RELEASE)
            .append('\n')
            .append("apiLevel=")
            .append(Build.VERSION.SDK_INT)
            .append('\n')
            .append("abi=")
            .append(
                Build.SUPPORTED_ABIS.length == 0
                    ? "unknown"
                    : Build.SUPPORTED_ABIS[0]
            )
            .append('\n')
            .append('\n')
            .append("Privacy note: development logs may contain device names, ")
            .append("local filenames, module names, controller state, timing, ")
            .append("and diagnostic application events. Review before sharing.\n");

        ZipEntry entry =
            new ZipEntry("manifest.txt");
        zip.putNextEntry(entry);
        zip.write(
            manifest.toString()
                .getBytes(StandardCharsets.UTF_8)
        );
        zip.closeEntry();
    }

    private void addLogFiles(ZipOutputStream zip)
        throws IOException {
        String path =
            ReverieLog.getLogDirectoryPath();
        if (path == null || path.trim().isEmpty()) {
            return;
        }

        File directory = new File(path);
        File[] files = directory.listFiles(
            file ->
                file.isFile()
                    && file.getName()
                        .startsWith("reverie-")
        );
        if (files == null || files.length == 0) {
            return;
        }

        Arrays.sort(
            files,
            Comparator.comparing(File::getName)
        );

        byte[] buffer = new byte[BUFFER_SIZE];
        for (File file : files) {
            ZipEntry entry =
                new ZipEntry(
                    "logs/" + file.getName()
                );
            zip.putNextEntry(entry);

            try (BufferedInputStream input =
                     new BufferedInputStream(
                         new FileInputStream(file)
                     )) {
                int read;
                while ((read = input.read(buffer)) >= 0) {
                    if (read > 0) {
                        zip.write(buffer, 0, read);
                    }
                }
            }

            zip.closeEntry();
        }
    }
}
