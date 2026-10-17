package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public final class NativeMusicCatalogTest {
    private static final String DIGEST =
        "dd0240297ea62fe2002b736ce2b43002cafc307ad76e32d795587012ffb99c00";

    private NativeMusicCatalog parse(String source) throws IOException {
        return NativeMusicCatalog.parse(new ByteArrayInputStream(
            source.getBytes(StandardCharsets.UTF_8)));
    }

    @Test public void readsBundledAndOfficialApprovedFiles() throws Exception {
        NativeMusicCatalog catalog = parse(
            "# offline licensed tracks\\n".replace("\\n", "\n") +
            "starry-cereal\tStarry Cereal\tresource:starry_cereal_menu\t" +
                "LicenseRef-Suno-Commercial-Use\t" + DIGEST + "\n" +
            "track-002\tA Starry Night\tmusic/tracks/track-002.mp3\t" +
                "LicenseRef-Suno-Commercial-Use\t" + DIGEST + "\n");
        assertEquals(2, catalog.size());
        assertEquals("Starry Cereal", catalog.at(0).title);
        assertEquals("A Starry Night", catalog.titles()[1]);
        assertTrue(catalog.at(0).packagedResource());
        assertFalse(catalog.at(1).packagedResource());
    }

    private void invalid(String row) throws IOException {
        try {
            parse(row + "\n");
            fail("Invalid catalog should not load");
        } catch (IOException expected) {
            // This is the expected validation boundary.
        }
    }

    @Test public void rejectsUnlicensedOrRemoteStream() throws Exception {
        invalid("bad\tFake\thttps://cdn1.suno.ai/track.mp3\t" +
            "LicenseRef-Suno-Commercial-Use\t" + DIGEST);
        invalid("bad\tFake\tmusic/tracks/../../fake.mp3\t" +
            "LicenseRef-Suno-Commercial-Use\t" + DIGEST);
        invalid("bad\tFake\tmusic/tracks/bad.mp3\tCC-BY-4.0\t" + DIGEST);
    }

    @Test public void rejectsDuplicateIdentifiers() throws Exception {
        try {
            parse("song\tA\tmusic/tracks/song.mp3\t" +
                    "LicenseRef-Suno-Commercial-Use\t" + DIGEST + "\n" +
                  "song\tB\tmusic/tracks/song.wav\t" +
                    "LicenseRef-Suno-Commercial-Use\t" + DIGEST + "\n");
            fail("Duplicate track");
        } catch (IOException expected) {
            // Fail closed.
        }
    }

    @Test public void ownerOriginalCatalogRequiresSpecificCommercialLicense()
        throws Exception {
        String row = "sky-lobby-drift\tSky Lobby Drift\t" +
            "music/tracks/sky-lobby-drift.mp3\t" +
            "LicenseRef-Pending-Asset-Rights\t" + DIGEST + "\n";
        try {
            parse(row);
            fail("Owner-original audio must not be mislabeled Suno licensed");
        } catch (IOException expected) {
            // The licensed catalogs remain strictly separated.
        }
        try {
            NativeMusicCatalog.parseOwnerMusic(
                new ByteArrayInputStream(row.getBytes(StandardCharsets.UTF_8)));
            fail("Pending-rights track must not be admitted to owner music");
        } catch (IOException expected) {
            // Previous provisional asset rights are no longer accepted.
        }
        String ownerRow = row.replace("LicenseRef-Pending-Asset-Rights",
            "LicenseRef-Owner-Original-Music");
        NativeMusicCatalog preview = NativeMusicCatalog.parseOwnerMusic(
            new ByteArrayInputStream(ownerRow.getBytes(StandardCharsets.UTF_8)));
        assertEquals(1, preview.size());
        invalid("unapproved\tBad\tmusic/tracks/unapproved.mp3\t" +
            "LicenseRef-Pending-Asset-Rights\t" + DIGEST);
        NativeMusicCatalog approved = parse(
            "starry-cereal\tStarry Cereal\tresource:starry_cereal_menu\t" +
            "LicenseRef-Suno-Commercial-Use\t" + DIGEST + "\n");
        NativeMusicCatalog combined = NativeMusicCatalog.combine(approved,preview);
        assertEquals(2,combined.size());
        assertEquals(1,combined.indexOfId("sky-lobby-drift"));
        assertEquals(-1,combined.indexOfId("missing"));
        try {
            NativeMusicCatalog.combine(combined,preview);
            fail("Duplicate IDs must fail across preview catalogs");
        } catch(IOException expected){}
    }

    @Test public void lobbyUsesOnlyLobbySongsAndCyclesAcrossSessions()
        throws Exception {
        String licensed = "starry-cereal\tStarry Cereal\tresource:starry_cereal_menu\t" +
            "LicenseRef-Suno-Commercial-Use\t" + DIGEST + "\n";
        String preview =
            "sky-lobby-drift\tSky Lobby Drift\tmusic/tracks/sky-lobby-drift.mp3\t" +
            "LicenseRef-Owner-Original-Music\t" + DIGEST + "\n" +
            "mars-des-souvenirs\tMars des Souvenirs\tmusic/tracks/mars-des-souvenirs.mp3\t" +
            "LicenseRef-Owner-Original-Music\t" + DIGEST + "\n" +
            "glass-lobby-drift\tGlass Lobby Drift\tmusic/tracks/glass-lobby-drift.mp3\t" +
            "LicenseRef-Owner-Original-Music\t" + DIGEST + "\n";
        NativeMusicCatalog library = NativeMusicCatalog.combine(
            parse(licensed),NativeMusicCatalog.parseOwnerMusic(
                new ByteArrayInputStream(preview.getBytes(StandardCharsets.UTF_8))));
        assertArrayEquals(new int[]{0,1,3},
            NativeMusicPlaybackPolicy.availableLobbyTracks(library));
        assertEquals(0,NativeMusicPlaybackPolicy.initialLobbyIndex(library,0));
        assertEquals(1,NativeMusicPlaybackPolicy.initialLobbyIndex(library,1));
        assertEquals(3,NativeMusicPlaybackPolicy.initialLobbyIndex(library,2));
        assertEquals(0,NativeMusicPlaybackPolicy.nextLobbyIndex(library,3));
        assertEquals(0,NativeMusicPlaybackPolicy.initialLobbyIndex(library,3));
        assertEquals("mars-des-souvenirs",
            NativeMusicPlaybackPolicy.cueForNativeModule("breakwater-battery"));
        assertEquals("last-bugle-call",
            NativeMusicPlaybackPolicy.cueForNativeModule("ministry-of-intelligence"));
        assertNull(NativeMusicPlaybackPolicy.cueForNativeModule("classic-chess"));
    }

    @Test public void admittedRedLedgerUsesItsOriginalPackagedMusic()
        throws Exception {
        // Match the registered module, not an obsolete project label.
        assertEquals("between-deliveries-red-ledger",
            NativeModuleRuntime.ID_RED_LEDGER);
        String cue = NativeMusicPlaybackPolicy.cueForNativeModule(
            NativeModuleRuntime.ID_RED_LEDGER);
        assertEquals("the-last-note-hangs-unresolved", cue);
        assertNull(NativeMusicPlaybackPolicy.cueForNativeModule("bar-project"));
        String originalRow = cue + "\tThe Last Note Hangs Unresolved\t" +
            "music/tracks/the-last-note-hangs-unresolved.mp3\t" +
            "LicenseRef-Owner-Original-Music\t" +
            "6be461d1d39bfd5f84d63a00562eb1fec905b121c231a4a2fa2c919800f6987d\n";
        NativeMusicCatalog originals = NativeMusicCatalog.parseOwnerMusic(
            new ByteArrayInputStream(originalRow.getBytes(StandardCharsets.UTF_8)));
        assertEquals(0, originals.indexOfId(cue));
        assertEquals("music/tracks/the-last-note-hangs-unresolved.mp3",
            originals.at(0).source);
        assertFalse(originals.at(0).packagedResource());
        assertEquals("LicenseRef-Owner-Original-Music", originals.at(0).license);
        assertEquals(-1, originals.indexOfId("mars-des-souvenirs"));
    }

    @Test public void emptyCatalogIsNotAccepted() throws Exception {
        try {
            parse("# no downloads approved yet\n");
            fail("Empty catalogue should fail");
        } catch (IOException expected) {
            // Fail closed.
        }
    }
}
