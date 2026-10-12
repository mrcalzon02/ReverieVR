package io.github.mrcalzon02.reverievr;

/** Explicit soundtrack routing. This never touches remote services or reads
 * screen pixels. Unpackaged cues are not advertised as playable. */
final class NativeMusicPlaybackPolicy {
    static final String[] LOBBY_IDS = {
        "starry-cereal", "sky-lobby-drift", "glass-lobby-drift"
    };

    private NativeMusicPlaybackPolicy() {}

    static String cueForNativeModule(String moduleId) {
        if ("breakwater-battery".equals(moduleId))
            return "mars-des-souvenirs";
        // These module IDs are only future cue contracts until admitted
        // native games actually register them with the shell.
        if ("ministry-of-intelligence".equals(moduleId))
            return "last-bugle-call";
        if ("bar-project".equals(moduleId))
            return "the-last-note-hangs-unresolved";
        return null;
    }

    static int[] availableLobbyTracks(NativeMusicCatalog catalog) {
        if (catalog == null) return new int[0];
        int count=0;
        int[] candidates = new int[LOBBY_IDS.length];
        for (String id : LOBBY_IDS) {
            int index = catalog.indexOfId(id);
            if (index >= 0) candidates[count++] = index;
        }
        int[] found = new int[count];
        System.arraycopy(candidates, 0, found, 0, count);
        return found;
    }

    static int initialLobbyIndex(NativeMusicCatalog catalog, int sessionNumber) {
        int[] indices = availableLobbyTracks(catalog);
        return indices.length == 0 ? 0 :
            indices[Math.floorMod(sessionNumber, indices.length)];
    }

    static int nextLobbyIndex(NativeMusicCatalog catalog, int current) {
        int[] indices = availableLobbyTracks(catalog);
        if (indices.length == 0) return 0;
        for (int i=0;i<indices.length;i++) {
            if (indices[i] == current)
                return indices[(i + 1) % indices.length];
        }
        return indices[0];
    }
}
