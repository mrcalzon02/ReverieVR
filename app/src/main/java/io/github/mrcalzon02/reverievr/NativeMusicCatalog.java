package io.github.mrcalzon02.reverievr;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Versioned offline listing of packaged audio. The approved catalog and
 * separately licensed owner-original music catalog remain distinct.
 * A public Suno URL is not an audio download or a license grant.
 */
final class NativeMusicCatalog {
    static final String PATH = "music/catalog.tsv";
    static final int MAX_TRACKS = 1000;

    static final class Track {
        final String id;
        final String title;
        final String source;
        final String license;
        final String sha256;

        Track(String id, String title, String source, String license,
              String sha256) {
            this.id = id;
            this.title = title;
            this.source = source;
            this.license = license;
            this.sha256 = sha256;
        }

        boolean packagedResource() {
            return source.startsWith("resource:");
        }
    }

    private final List<Track> tracks;

    private NativeMusicCatalog(List<Track> tracks) {
        this.tracks = Collections.unmodifiableList(new ArrayList<>(tracks));
    }

    static NativeMusicCatalog parse(InputStream input) throws IOException {
        return parseCatalog(input, false);
    }

    /** Commercially authorized owner-original music remains separately
     * identified from older Suno-labelled entries, without CC relicensing. */
    static NativeMusicCatalog parseOwnerMusic(InputStream input)
        throws IOException {
        return parseCatalog(input, true);
    }

    private static NativeMusicCatalog parseCatalog(InputStream input,
                                                   boolean ownerMusic)
        throws IOException {
        if (input == null) throw new IOException("Missing music catalog");
        List<Track> rows = new ArrayList<>();
        Set<String> seen = new HashSet<>();
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(input, StandardCharsets.UTF_8))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.isEmpty() || line.startsWith("#")) continue;
                if (line.length() > 1100 || rows.size() >= MAX_TRACKS)
                    throw new IOException("Invalid music catalog size");
                String[] columns = line.split("\\t", -1);
                if (columns.length != 5)
                    throw new IOException("Invalid music catalog row");
                String id = columns[0];
                String title = columns[1];
                String source = columns[2];
                String license = columns[3];
                String digest = columns[4];
                if (!id.matches("[a-z0-9][a-z0-9_-]{0,79}")
                    || !seen.add(id)
                    || title.isEmpty() || title.length() > 170
                    || title.indexOf((char) 13) >= 0
                    || !license.equals(ownerMusic
                        ? "LicenseRef-Owner-Original-Music"
                        : "LicenseRef-Suno-Commercial-Use")
                    || !digest.matches("[a-f0-9]{64}")
                    || !(ownerMusic
                        ? source.matches(
                            "music/tracks/[a-z0-9][a-z0-9_-]{0,79}\\.mp3")
                        : source.equals("resource:starry_cereal_menu")
                            || source.matches(
                                "music/tracks/[a-z0-9][a-z0-9_-]{0,79}\\.(mp3|wav)")))
                    throw new IOException("Unapproved or duplicate music catalog entry");
                rows.add(new Track(id, title, source, license, digest));
            }
        }
        if (rows.isEmpty()) throw new IOException("Empty music catalog");
        return new NativeMusicCatalog(rows);
    }

    static NativeMusicCatalog combine(NativeMusicCatalog approved,
                                      NativeMusicCatalog preview)
        throws IOException {
        if (approved == null) throw new IOException("Missing approved catalog");
        if (preview == null) return approved;
        if (approved.size() + preview.size() > MAX_TRACKS)
            throw new IOException("Too many combined music tracks");
        ArrayList<Track> merged = new ArrayList<>(approved.tracks);
        Set<String> ids = new HashSet<>();
        Set<String> sources = new HashSet<>();
        for (Track track : merged) {
            ids.add(track.id);
            sources.add(track.source);
        }
        for (Track track : preview.tracks) {
            if (!ids.add(track.id) || !sources.add(track.source))
                throw new IOException("Duplicate music id or binary path");
            merged.add(track);
        }
        return new NativeMusicCatalog(merged);
    }

    int indexOfId(String id) {
        if (id == null) return -1;
        for (int i = 0; i < tracks.size(); i++)
            if (id.equals(tracks.get(i).id)) return i;
        return -1;
    }

    int size() {
        return tracks.size();
    }

    Track at(int index) {
        return tracks.get(index);
    }

    String[] titles() {
        String[] result = new String[tracks.size()];
        for (int i = 0; i < tracks.size(); i++) {
            result[i] = tracks.get(i).title;
        }
        return result;
    }
}
