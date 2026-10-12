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

    @Test public void emptyCatalogIsNotAccepted() throws Exception {
        try {
            parse("# no downloads approved yet\n");
            fail("Empty catalogue should fail");
        } catch (IOException expected) {
            // Fail closed.
        }
    }
}
