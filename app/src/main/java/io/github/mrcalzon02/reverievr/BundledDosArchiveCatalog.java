package io.github.mrcalzon02.reverievr;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * Descriptor-only catalog for original, SHA-pinned DOS archives packaged in
 * the APK. No entry is shown in the game library until its verified local
 * extraction has been registered in DosModuleRepository.
 *
 * DOOM's historical split installer remains in BundledDosContentInstaller.
 */
final class BundledDosArchiveCatalog {
    static final class Entry {
        final String id;
        final String displayName;
        final String assetPath;
        final String archiveFileName;
        final String archiveSha256;
        final String executableName;
        final String markerName;
        final String bindingProfileId;
        final int maxEntries;
        final long maxExpandedBytes;
        final boolean packaged;

        Entry(String id, String displayName, String assetPath,
              String archiveFileName, String archiveSha256,
              String executableName, String markerName, String bindingProfileId,
              int maxEntries, long maxExpandedBytes, boolean packaged) {
            this.id = id;
            this.displayName = displayName;
            this.assetPath = assetPath;
            this.archiveFileName = archiveFileName;
            this.archiveSha256 = archiveSha256;
            this.executableName = executableName;
            this.markerName = markerName;
            this.bindingProfileId = bindingProfileId;
            this.maxEntries = maxEntries;
            this.maxExpandedBytes = maxExpandedBytes;
            this.packaged = packaged;
        }
    }

    static final Entry KENS_LABYRINTH = new Entry(
        "bundled-kens-labyrinth-2.1",
        "Ken's Labyrinth v2.1 (Freeware)",
        "default-content/labfull.zip",
        "labfull.zip",
        "643aa0ca8d1a7d2c88afecd952da00f3b912be8b83c4c1d2f4ab9b50491fadfd",
        "KEN.EXE",
        ".reverie-ken-bootstrap-v1",
        BuiltInBindingProfiles.ID_DOS_KENS_LABYRINTH,
        256,
        64L * 1024L * 1024L,
        BuildConfig.KENS_LABYRINTH_BUNDLED
    );

    // Add verified future default titles here. Game rights, original archive
    // provenance, digest, Gradle packaging and bindings remain independent.
    private static final List<Entry> ALL = Collections.unmodifiableList(
        Arrays.asList(KENS_LABYRINTH));

    static List<Entry> all() { return ALL; }

    static Entry findById(String id) {
        if (id == null) return null;
        for (Entry entry : ALL)
            if (entry.id.equals(id)) return entry;
        return null;
    }

    private BundledDosArchiveCatalog() {}
}
