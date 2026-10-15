package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import java.io.File;
import java.io.FileOutputStream;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

public final class DosModuleRepositoryCapacityTest {
    @Rule public TemporaryFolder temp = new TemporaryFolder();

    @Test
    public void keepsMoreThanFiftyGamesWithIndependentBindingsAndSaves() throws Exception {
        File root = temp.newFolder("dos-library");
        DosModuleRepository library = new DosModuleRepository(root);
        for (int i = 0; i < 51; i++) {
            String id = "game-" + i;
            File directory = new File(root, id);
            assertTrue(directory.mkdirs());
            File content = new File(directory, "game.zip");
            try (FileOutputStream output = new FileOutputStream(content)) {
                output.write(i);
            }
            String binding = (i & 1) == 0
                ? BuiltInBindingProfiles.ID_DOS_STANDARD
                : BuiltInBindingProfiles.ID_DOS_KENS_LABYRINTH;
            library.save(new DosGameModule(id, "Game " + i,
                "game.zip", content.getAbsolutePath(), binding, i));
        }
        List<DosGameModule> found = library.list();
        assertEquals(51, found.size());
        Set<String> ids = new HashSet<>();
        for (DosGameModule module : found) {
            assertTrue(ids.add(module.id));
            assertNotNull(library.findById(module.id));
            assertTrue(module.isContentPresent());
        }
        assertEquals("game-50", found.get(0).id);
        assertEquals("game-0", found.get(50).id);

        assertTrue(library.updateBindingProfileId(
            "game-9", BuiltInBindingProfiles.ID_DOS_STANDARD));
        assertEquals(BuiltInBindingProfiles.ID_DOS_STANDARD,
            library.findById("game-9").bindingProfileId);
        assertEquals(BuiltInBindingProfiles.ID_DOS_KENS_LABYRINTH,
            library.findById("game-11").bindingProfileId);

        // A new repository instance must recover the same full library.
        DosModuleRepository reopened = new DosModuleRepository(root);
        assertEquals(51, reopened.list().size());
        assertEquals(BuiltInBindingProfiles.ID_DOS_STANDARD,
            reopened.findById("game-9").bindingProfileId);
    }
    @Test
    public void equalTimestampBundledGamesKeepStableAlphabeticalOrder()
        throws Exception {
        File root = temp.newFolder("freeware-collection");
        DosModuleRepository library = new DosModuleRepository(root);
        String[][] games = {
            {"z-jet", "Jetpack"},
            {"a-squarez", "Squarez Deluxe"},
            {"z-doom", "DOOM Shareware"},
            {"b-jet", "Jetpack"}
        };
        for (String[] game : games) {
            File directory = new File(root, game[0]);
            assertTrue(directory.mkdirs());
            File zip = new File(directory, "original.zip");
            try (FileOutputStream output = new FileOutputStream(zip)) {
                output.write(1);
            }
            library.save(new DosGameModule(
                game[0], game[1], "original.zip",
                zip.getAbsolutePath(), BuiltInBindingProfiles.ID_DOS_STANDARD,
                0L));
        }
        List<DosGameModule> expected = library.list();
        assertEquals("DOOM Shareware", expected.get(0).displayName);
        assertEquals("a-squarez", expected.get(3).id);
        assertEquals("b-jet", expected.get(1).id);
        assertEquals("z-jet", expected.get(2).id);
        for (int iteration = 0; iteration < 3; iteration++) {
            List<DosGameModule> again = new DosModuleRepository(root).list();
            for (int i = 0; i < expected.size(); i++) {
                assertEquals(expected.get(i).id, again.get(i).id);
            }
        }
    }
}
