package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.media.MediaPlayer;

final class ShellMenuMusic implements AutoCloseable {
    private static final float MENU_GAIN = 0.22f;

    private final Context context;

    private MediaPlayer player;
    private boolean lifecycleActive;
    private boolean shellActive = true;
    private boolean closed;

    ShellMenuMusic(Context context) {
        this.context =
            context == null
                ? null
                : context.getApplicationContext();
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
        if (closed) {
            return;
        }

        if (!lifecycleActive || !shellActive) {
            pausePlayer();
            return;
        }

        ensurePlayer();
        MediaPlayer active = player;
        if (active == null) {
            return;
        }

        try {
            if (!active.isPlaying()) {
                active.start();
            }
        } catch (IllegalStateException exception) {
            ReverieLog.error(
                "MENU_MUSIC",
                "Could not start VR shell menu music.",
                exception
            );
            releasePlayer();
        }
    }

    private void ensurePlayer() {
        if (player != null || context == null || closed) {
            return;
        }

        try {
            MediaPlayer created =
                MediaPlayer.create(
                    context,
                    R.raw.starry_cereal_menu
                );
            if (created == null) {
                ReverieLog.incident(
                    "MENU_MUSIC",
                    "Android MediaPlayer could not create the packaged Starry Cereal menu track."
                );
                return;
            }

            created.setLooping(true);
            created.setVolume(
                MENU_GAIN,
                MENU_GAIN
            );
            created.setOnErrorListener(
                (failed, what, extra) -> {
                    ReverieLog.incident(
                        "MENU_MUSIC",
                        "Menu music playback error what="
                            + what
                            + " extra="
                            + extra
                    );
                    if (player == failed) {
                        player = null;
                    }
                    try {
                        failed.release();
                    } catch (RuntimeException ignored) {
                    }
                    return true;
                }
            );
            player = created;

            ReverieLog.milestone(
                "MENU_MUSIC",
                "Loaded packaged Starry Cereal VR shell ambience."
            );
        } catch (RuntimeException exception) {
            ReverieLog.error(
                "MENU_MUSIC",
                "Could not initialize VR shell menu music.",
                exception
            );
            releasePlayer();
        }
    }

    private void pausePlayer() {
        MediaPlayer active = player;
        if (active == null) {
            return;
        }

        try {
            if (active.isPlaying()) {
                active.pause();
            }
        } catch (IllegalStateException exception) {
            ReverieLog.error(
                "MENU_MUSIC",
                "Could not pause VR shell menu music.",
                exception
            );
            releasePlayer();
        }
    }

    private void releasePlayer() {
        MediaPlayer active = player;
        player = null;
        if (active == null) {
            return;
        }

        try {
            active.release();
        } catch (RuntimeException ignored) {
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
