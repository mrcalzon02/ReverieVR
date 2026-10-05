package io.github.mrcalzon02.reverievr;

import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;
import java.util.UUID;

final class DosModuleImporter {
    private static final int BUFFER_SIZE = 64 * 1024;

    private final Context context;
    private final DosModuleRepository repository;

    DosModuleImporter(
        Context context,
        DosModuleRepository repository
    ) {
        this.context = context.getApplicationContext();
        this.repository = repository;
    }

    DosGameModule importDocument(
        Uri uri,
        String displayName
    ) throws IOException {
        if (uri == null) {
            throw new IOException(
                "DOS content URI is missing."
            );
        }

        String safeName =
            sanitizeFileName(displayName);
        if (!DosContentSupport.isSupportedFileName(safeName)) {
            throw new IOException(
                "Unsupported DOS content type. Supported: "
                    + DosContentSupport
                        .supportedExtensionsSummary()
            );
        }

        String id = UUID.randomUUID().toString();

        ReverieLog.milestone(
            "DOS_IMPORT",
            "Import start name="
                + safeName
                + " uriScheme="
                + (uri.getScheme() == null
                    ? "unknown"
                    : uri.getScheme())
        );
        File directory =
            new File(repository.rootDirectory(), id);
        File contentDirectory =
            new File(directory, "content");

        if (!contentDirectory.mkdirs()
            && !contentDirectory.isDirectory()) {
            throw new IOException(
                "Could not create DOS content directory."
            );
        }

        File destination =
            new File(contentDirectory, safeName);

        try {
            copy(uri, destination);

            if (!destination.isFile()
                || destination.length() <= 0L) {
                throw new IOException(
                    "Imported DOS content is empty."
                );
            }

            DosGameModule module =
                new DosGameModule(
                    id,
                    displayNameWithoutExtension(safeName),
                    safeName,
                    destination.getAbsolutePath(),
                    BuiltInBindingProfiles.ID_NONE,
                    System.currentTimeMillis()
                );

            repository.save(module);

            ReverieLog.milestone(
                "DOS_IMPORT",
                "Import complete id="
                    + module.id
                    + " name="
                    + module.displayName
                    + " bytes="
                    + destination.length()
                    + " path="
                    + destination.getAbsolutePath()
            );

            return module;
        } catch (IOException exception) {
            ReverieLog.error(
                "DOS_IMPORT",
                "Import failed for " + safeName,
                exception
            );
            deleteRecursively(directory);
            throw exception;
        }
    }

    private void copy(Uri uri, File destination)
        throws IOException {
        ContentResolver resolver =
            context.getContentResolver();

        InputStream raw = resolver.openInputStream(uri);
        if (raw == null) {
            throw new IOException(
                "Android could not open the selected file."
            );
        }

        byte[] buffer = new byte[BUFFER_SIZE];
        try (BufferedInputStream input =
                 new BufferedInputStream(raw);
             BufferedOutputStream output =
                 new BufferedOutputStream(
                     new FileOutputStream(destination)
                 )) {
            int read;
            while ((read = input.read(buffer)) >= 0) {
                if (read > 0) {
                    output.write(buffer, 0, read);
                }
            }
        }
    }

    private static String sanitizeFileName(
        String displayName
    ) {
        String value =
            displayName == null
                ? "dos-content"
                : displayName.trim();

        if (value.isEmpty()) {
            value = "dos-content";
        }

        value = value.replaceAll(
            "[^A-Za-z0-9._()\\- ]",
            "_"
        );

        if (value.length() > 160) {
            value = value.substring(
                value.length() - 160
            );
        }

        return value;
    }

    private static String displayNameWithoutExtension(
        String fileName
    ) {
        int dot = fileName.lastIndexOf('.');
        String value =
            dot > 0
                ? fileName.substring(0, dot)
                : fileName;

        value = value.trim();
        return value.isEmpty()
            ? "DOS game"
            : value;
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
