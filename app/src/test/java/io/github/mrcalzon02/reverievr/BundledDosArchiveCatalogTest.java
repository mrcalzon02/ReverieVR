package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import java.util.HashSet;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import org.junit.Test;

public final class BundledDosArchiveCatalogTest {
    @Test
    public void catalogEntriesHaveIndependentCompleteInstallMetadata() {
        Set<String> ids = new HashSet<>();
        Map<String, String> archives = new HashMap<>();
        assertFalse(BundledDosArchiveCatalog.all().isEmpty());
        for (BundledDosArchiveCatalog.Entry entry :
                BundledDosArchiveCatalog.all()) {
            assertTrue(ids.add(entry.id));
            // Multiple games may use the same unchanged ZIP, but never
            // silently claim two different hashes for the same path.
            String previous = archives.putIfAbsent(
                entry.assetPath, entry.archiveSha256);
            if (previous != null) assertEquals(previous, entry.archiveSha256);
            assertTrue(entry.assetPath.startsWith("default-content/"));
            assertTrue(entry.archiveFileName.endsWith(".zip"));
            assertTrue(entry.archiveSha256.matches("[a-f0-9]{64}"));
            assertTrue(entry.executableName.toUpperCase().endsWith(".EXE"));
            assertTrue(entry.maxEntries > 0);
            assertTrue(entry.maxExpandedBytes > 0L);
            assertTrue(BuiltInBindingProfiles.isDosProfileId(
                entry.bindingProfileId));
            assertEquals(entry, BundledDosArchiveCatalog.findById(entry.id));
        }
    }

    @Test
    public void jetpackAndSquarezHaveDifferentExecutablesInOneZip() {
        BundledDosArchiveCatalog.Entry jet = BundledDosArchiveCatalog.JETPACK;
        BundledDosArchiveCatalog.Entry sq = BundledDosArchiveCatalog.SQUAREZ;
        assertEquals(jet.assetPath, sq.assetPath);
        assertEquals(jet.archiveSha256, sq.archiveSha256);
        assertFalse(jet.id.equals(sq.id));
        assertFalse(jet.executableName.equals(sq.executableName));
        assertFalse(jet.bindingProfileId.equals(sq.bindingProfileId));
        assertEquals(BuildConfig.JETPACK_BUNDLED, jet.packaged);
        assertEquals(jet.packaged, sq.packaged);
    }

    @Test
    public void catalogDoesNotPretendUninstalledTitlesArePlayable() {
        assertEquals(null, BundledDosArchiveCatalog.findById("skyroads"));
        assertEquals(BuildConfig.KENS_LABYRINTH_BUNDLED,
            BundledDosArchiveCatalog.KENS_LABYRINTH.packaged);
    }
}
