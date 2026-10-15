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

    // The same original ZIP legitimately contains two complete DOS games.
    // Independent module IDs, launch executables, saves and binding profiles
    // share its exact same archive SHA and unchanged packaged asset.
    static final Entry JETPACK = new Entry(
        "bundled-jetpack-1.5",
        "Jetpack v1.5 (Freeware)",
        "default-content/jetpak15.zip",
        "jetpak15.zip",
        "03f4979149bdeeb43091517e0f33f92b14cc2e5d7531e0c524749c3383fc9d4c",
        "JETPACK.EXE",
        ".reverie-jetpack-bootstrap-v1",
        BuiltInBindingProfiles.ID_DOS_JETPACK,
        256, 64L * 1024L * 1024L,
        BuildConfig.JETPACK_BUNDLED
    );

    static final Entry SQUAREZ = new Entry(
        "bundled-squarez-deluxe",
        "Squarez Deluxe (Freeware)",
        "default-content/jetpak15.zip",
        "jetpak15.zip",
        "03f4979149bdeeb43091517e0f33f92b14cc2e5d7531e0c524749c3383fc9d4c",
        "SQUAREZ.EXE",
        ".reverie-squarez-bootstrap-v1",
        BuiltInBindingProfiles.ID_DOS_SQUAREZ,
        256, 64L * 1024L * 1024L,
        BuildConfig.JETPACK_BUNDLED
    );

    // Game media rights, original provenance, verified hashes and bindings
    // remain independently declared before titles enter the live library.
    private static final List<Entry> ALL = Collections.unmodifiableList(
        Arrays.asList(KENS_LABYRINTH, JETPACK, SQUAREZ));

    static List<Entry> all() { return ALL; }

    static Entry findById(String id) {
        if (id == null) return null;
        for (Entry entry : ALL)
            if (entry.id.equals(id)) return entry;
        return null;
    }

    private BundledDosArchiveCatalog() {}
}
