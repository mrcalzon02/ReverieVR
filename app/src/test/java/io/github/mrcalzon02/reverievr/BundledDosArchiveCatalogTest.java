package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import java.util.HashSet;
import java.util.Set;
import org.junit.Test;

public final class BundledDosArchiveCatalogTest {
    @Test
    public void catalogEntriesHaveIndependentCompleteInstallMetadata() {
        Set<String> ids = new HashSet<>();
        Set<String> archives = new HashSet<>();
        assertFalse(BundledDosArchiveCatalog.all().isEmpty());
        for (BundledDosArchiveCatalog.Entry entry :
                BundledDosArchiveCatalog.all()) {
            assertTrue(ids.add(entry.id));
            assertTrue(archives.add(entry.assetPath));
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
    public void catalogDoesNotPretendUninstalledTitlesArePlayable() {
        assertEquals(null, BundledDosArchiveCatalog.findById("skyroads"));
        assertEquals(BuildConfig.KENS_LABYRINTH_BUNDLED,
            BundledDosArchiveCatalog.KENS_LABYRINTH.packaged);
    }
}
