package io.github.mrcalzon02.reverievr;

import java.io.File;

final class DosGameModule {
    final String id;
    final String displayName;
    final String originalFileName;
    final String contentPath;
    final String bindingProfileId;
    final long importedAtMillis;

    DosGameModule(
        String id,
        String displayName,
        String originalFileName,
        String contentPath,
        String bindingProfileId,
        long importedAtMillis
    ) {
        this.id = required(id, "id");
        this.displayName = required(displayName, "displayName");
        this.originalFileName =
            required(originalFileName, "originalFileName");
        this.contentPath = required(contentPath, "contentPath");
        this.bindingProfileId =
            bindingProfileId == null
                || bindingProfileId.trim().isEmpty()
                    ? BuiltInBindingProfiles.ID_NONE
                    : bindingProfileId.trim();
        this.importedAtMillis = importedAtMillis;
    }

    File contentFile() {
        return new File(contentPath);
    }

    boolean isContentPresent() {
        File file = contentFile();
        return file.isFile() && file.canRead();
    }

    private static String required(
        String value,
        String label
    ) {
        String safe = value == null ? "" : value.trim();
        if (safe.isEmpty()) {
            throw new IllegalArgumentException(
                label + " is required."
            );
        }
        return safe;
    }
}
