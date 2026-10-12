package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.MediaPlayer;

import java.io.IOException;
import java.io.InputStream;

/**
 * Phone-resident native music player shared by VR Home and Music Library.
 * Never streams Suno or retrieves audio from a public profile page.
 */
final class ShellMenuMusic implements AutoCloseable {
    private static final float MUSIC_GAIN = 0.22f;

    private final Context context;
    private final NativeMusicCatalog catalog;
    private MediaPlayer player;
    private boolean lifecycleActive;
    private boolean shellActive = true;
    private volatile boolean userPaused;
    private volatile int selectedIndex;
    private boolean closed;

    ShellMenuMusic(Context context) {
        this.context = context == null ? null : context.getApplicationContext();
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
        catalog = loaded;
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
            + catalog.at(selectedIndex).title;
    }

    void chooseTrack(int index) {
        if (closed || catalog == null || index < 0 || index >= catalog.size())
            return;
        selectedIndex = index;
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
        shellActive = active;
        syncPlayback();
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
        final NativeMusicCatalog.Track track = catalog.at(selectedIndex);
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
            created.setLooping(catalog.size() == 1);
            created.setVolume(MUSIC_GAIN, MUSIC_GAIN);
            created.setOnCompletionListener(finished -> {
                if (catalog.size() > 1) {
                    selectedIndex = (selectedIndex + 1) % catalog.size();
                    releasePlayer();
                    syncPlayback();
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
        lifecycleActive = false;
        shellActive = false;
        releasePlayer();
    }
}
