package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.AssetFileDescriptor;
import android.media.MediaPlayer;

import java.io.IOException;
import java.io.InputStream;

/**
 * Phone-resident native music player shared by VR Home and Music Library.
 * Never streams Suno or retrieves audio from a public profile page.
 */
final class ShellMenuMusic implements AutoCloseable {
    private static final String LOBBY_PREFS = "reverie_music";
    private static final String LOBBY_COUNTER = "lobby_session";

    private final Context context;
    private final NativeMusicCatalog catalog;
    private final ReveriePreferences musicPreferences;
    private volatile int volumePercent = MusicVolumePolicy.DEFAULT_PERCENT;
    private volatile boolean muted;
    private MediaPlayer player;
    private boolean lifecycleActive;
    private boolean shellActive = true;
    private volatile boolean userPaused;
    private volatile int selectedIndex;
    private int lobbyIndex;
    private int hostedCueIndex = -1;
    private boolean manualSelection;
    private Runnable libraryRefresh;
    private boolean closed;

    ShellMenuMusic(Context context) {
        this.context = context == null ? null : context.getApplicationContext();
        musicPreferences = this.context == null
            ? null : new ReveriePreferences(this.context);
        if (musicPreferences != null) {
            volumePercent = musicPreferences.getMusicVolumePercent();
            muted = musicPreferences.isMusicMuted();
        }
        NativeMusicCatalog loaded = null;
        if (this.context != null) {
            try (InputStream file = this.context.getAssets().open(
                    NativeMusicCatalog.PATH)) {
                loaded = NativeMusicCatalog.parse(file);
            } catch (IOException failure) {
                ReverieLog.error("MENU_MUSIC",
                    "Packaged native music catalog is missing or invalid", failure);
            }
        }
        if (loaded != null && this.context != null) {
            // All builds contain the separate owner-original audio catalog.
            // Keep legacy Suno and owner-commercial provenance distinct.
            try (InputStream ownerAudio =
                     this.context.getAssets().open("music/owner_library.tsv")) {
                loaded = NativeMusicCatalog.combine(loaded,
                    NativeMusicCatalog.parseOwnerMusic(ownerAudio));
            } catch (IOException problem) {
                ReverieLog.error("MENU_MUSIC",
                    "Owner-original music catalog missing or invalid", problem);
            }
        }
        catalog = loaded;
        if (catalog != null && catalog.size() > 0 && this.context != null) {
            SharedPreferences prefs =
                this.context.getSharedPreferences(LOBBY_PREFS, Context.MODE_PRIVATE);
            int previous = prefs.getInt(LOBBY_COUNTER, -1);
            int session = previous == Integer.MAX_VALUE ? 0 : previous + 1;
            lobbyIndex = NativeMusicPlaybackPolicy.initialLobbyIndex(
                catalog, session);
            selectedIndex = lobbyIndex;
            prefs.edit().putInt(LOBBY_COUNTER, session).apply();
        }
    }

    /** A single UI refresh follows automatic song completion, rather than
     * polling the player's state on every headset render frame. */
    void setLibraryRefresh(Runnable callback) {
        libraryRefresh = callback;
    }

    private void notifyLibraryRefresh() {
        Runnable callback = libraryRefresh;
        if (callback != null) callback.run();
    }

    String[] trackNames() {
        return catalog == null ? new String[0] : catalog.titles();
    }

    int trackIndex() {
        return selectedIndex;
    }

    String playbackStatus() {
        if (catalog == null || catalog.size() == 0) {
            return "NO PACKAGED MUSIC";
        }
        return (userPaused ? "PAUSED: " : "READY: ")
            + catalog.at(currentTrackIndex()).title;
    }

    String musicVolumeStatus() {
        return MusicVolumePolicy.display(volumePercent, muted);
    }

    void adjustMusicVolume(int direction) {
        if (closed || direction == 0) return;
        volumePercent = MusicVolumePolicy.adjust(volumePercent, direction);
        if (musicPreferences != null)
            musicPreferences.setMusicVolumePercent(volumePercent);
        applyMusicGain();
    }

    void toggleMusicMute() {
        if (closed) return;
        muted = !muted;
        if (musicPreferences != null) musicPreferences.setMusicMuted(muted);
        applyMusicGain();
    }

    private void applyMusicGain() {
        MediaPlayer active = player;
        if (active == null) return;
        float gain = MusicVolumePolicy.gain(volumePercent, muted);
        try {
            active.setVolume(gain, gain);
        } catch (IllegalStateException exception) {
            ReverieLog.error("MENU_MUSIC",
                "Could not apply music-only gain", exception);
            releasePlayer();
        }
    }

    void chooseTrack(int index) {
        if (closed || catalog == null || index < 0 || index >= catalog.size())
            return;
        selectedIndex = index;
        manualSelection = true;
        userPaused = false;
        releasePlayer();
        syncPlayback();
    }

    void stepTrack(int direction) {
        if (catalog == null || catalog.size() == 0) return;
        int size = catalog.size();
        chooseTrack(Math.floorMod(selectedIndex + (direction < 0 ? -1 : 1), size));
    }

    void toggleUserPause() {
        userPaused = !userPaused;
        syncPlayback();
    }

    void resumeForLifecycle() {
        lifecycleActive = true;
        syncPlayback();
    }

    void pauseForLifecycle() {
        lifecycleActive = false;
        syncPlayback();
    }

    void setShellActive(boolean active) {
        hostedCueIndex = -1;
        if (active) {
            // Returning from hosted content restores ambient lobby rotation,
            // rather than leaving a game's theme playing in Home.
            manualSelection = false;
            selectedIndex = lobbyIndex;
        }
        shellActive = active;
        releasePlayer();
        syncPlayback();
    }

    void setHostedNativeModule(String moduleId) {
        final String id = NativeMusicPlaybackPolicy.cueForNativeModule(moduleId);
        hostedCueIndex = catalog == null ? -1 : catalog.indexOfId(id);
        if(hostedCueIndex < 0 && catalog!=null && catalog.size()>0) {
            // Failure to stage a game's theme must not silently mute the
            // entire native game. Fall back to an existing packaged track.
            ReverieLog.incident("MENU_MUSIC",
                "Missing hosted soundtrack for "+moduleId+
                "; playing packaged lobby music instead");
            hostedCueIndex = Math.max(0,Math.min(lobbyIndex,catalog.size()-1));
        }
        shellActive = hostedCueIndex >= 0;
        releasePlayer();
        syncPlayback();
    }

    private int currentTrackIndex() {
        return hostedCueIndex >= 0 ? hostedCueIndex : selectedIndex;
    }

    private void syncPlayback() {
        if (closed) return;
        if (!lifecycleActive || !shellActive || userPaused) {
            pausePlayer();
            return;
        }
        ensurePlayer();
        MediaPlayer active = player;
        if (active == null) return;
        try {
            if (!active.isPlaying()) active.start();
        } catch (IllegalStateException exception) {
            ReverieLog.error("MENU_MUSIC",
                "Could not start native music player", exception);
            releasePlayer();
        }
    }

    private void ensurePlayer() {
        if (player != null || context == null || closed || catalog == null)
            return;
        final NativeMusicCatalog.Track track = catalog.at(currentTrackIndex());
        MediaPlayer created = null;
        try {
            if (track.packagedResource()) {
                created = MediaPlayer.create(context, R.raw.starry_cereal_menu);
                if (created == null) throw new IOException(
                    "Unable to open packaged Starry Cereal music");
            } else {
                created = new MediaPlayer();
                try (AssetFileDescriptor fd =
                         context.getAssets().openFd(track.source)) {
                    created.setDataSource(fd.getFileDescriptor(),
                        fd.getStartOffset(), fd.getLength());
                }
                created.prepare();
            }
            created.setLooping(hostedCueIndex >= 0 || catalog.size() == 1);
            float gain = MusicVolumePolicy.gain(volumePercent, muted);
            created.setVolume(gain, gain);
            created.setOnCompletionListener(finished -> {
                if (catalog.size() > 1 && hostedCueIndex < 0) {
                    if (manualSelection) {
                        selectedIndex = (selectedIndex + 1) % catalog.size();
                    } else {
                        lobbyIndex = NativeMusicPlaybackPolicy.nextLobbyIndex(
                            catalog, lobbyIndex);
                        selectedIndex = lobbyIndex;
                    }
                    releasePlayer();
                    syncPlayback();
                    notifyLibraryRefresh();
                }
            });
            created.setOnErrorListener((failed, what, extra) -> {
                ReverieLog.incident("MENU_MUSIC",
                    "Native music decoding error " + what + "/" + extra);
                if (player == failed) player = null;
                try {
                    failed.release();
                } catch (RuntimeException ignored) {
                    // Already released by MediaPlayer.
                }
                return true;
            });
            player = created;
            ReverieLog.milestone("MENU_MUSIC",
                "Native music loaded: " + track.title);
        } catch (IOException | RuntimeException failure) {
            ReverieLog.error("MENU_MUSIC",
                "Unable to open licensed local audio track " + track.id,
                failure);
            if (created != null) {
                try {
                    created.release();
                } catch (RuntimeException ignored) {
                }
            }
        }
    }

    private void pausePlayer() {
        MediaPlayer active = player;
        if (active == null) return;
        try {
            if (active.isPlaying()) active.pause();
        } catch (IllegalStateException exception) {
            ReverieLog.error("MENU_MUSIC",
                "Unable to pause native music", exception);
            releasePlayer();
        }
    }

    private void releasePlayer() {
        MediaPlayer active = player;
        player = null;
        if (active != null) {
            try {
                active.release();
            } catch (RuntimeException ignored) {
                // Do not keep an invalid player across a VR lifecycle change.
            }
        }
    }

    @Override
    public void close() {
        closed = true;
        libraryRefresh = null;
        lifecycleActive = false;
        shellActive = false;
        releasePlayer();
    }
}
