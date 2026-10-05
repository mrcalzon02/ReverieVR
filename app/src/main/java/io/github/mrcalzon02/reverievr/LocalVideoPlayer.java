package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.media.AudioAttributes;
import android.media.MediaPlayer;
import android.net.Uri;
import android.view.Surface;

import java.io.IOException;

final class LocalVideoPlayer {
    interface Listener {
        void onVideoPrepared(float aspectRatio);
        void onVideoStateChanged(String message);
        void onVideoError(String message);
    }

    private final Context context;
    private final Listener listener;

    private Surface surface;
    private MediaPlayer player;
    private Uri requestedUri;
    private boolean prepared;
    private boolean resumeAfterLifecyclePause;

    LocalVideoPlayer(Context context, Listener listener) {
        this.context = context.getApplicationContext();
        this.listener = listener;
    }

    void attachSurfaceTexture(SurfaceTexture surfaceTexture) {
        releasePlayer();
        releaseSurface();

        if (surfaceTexture == null) {
            return;
        }

        surface = new Surface(surfaceTexture);
        if (requestedUri != null) {
            openRequestedUri();
        }
    }

    void play(String uriString) {
        if (uriString == null || uriString.trim().isEmpty()) {
            listener.onVideoError("No local video is selected.");
            return;
        }

        requestedUri = Uri.parse(uriString);
        releasePlayer();

        if (surface == null) {
            listener.onVideoStateChanged("Waiting for the VR video surface…");
            return;
        }

        openRequestedUri();
    }

    void togglePause() {
        MediaPlayer active = player;
        if (!prepared || active == null) {
            return;
        }

        try {
            if (active.isPlaying()) {
                active.pause();
                listener.onVideoStateChanged("Video paused.");
            } else {
                active.start();
                listener.onVideoStateChanged("Video playing.");
            }
        } catch (IllegalStateException exception) {
            listener.onVideoError("Video playback state became invalid.");
        }
    }

    void pauseForLifecycle() {
        MediaPlayer active = player;
        resumeAfterLifecyclePause = false;

        if (!prepared || active == null) {
            return;
        }

        try {
            if (active.isPlaying()) {
                active.pause();
                resumeAfterLifecyclePause = true;
            }
        } catch (IllegalStateException ignored) {
            resumeAfterLifecyclePause = false;
        }
    }

    void resumeForLifecycle() {
        MediaPlayer active = player;
        if (!resumeAfterLifecyclePause || !prepared || active == null) {
            return;
        }

        resumeAfterLifecyclePause = false;
        try {
            active.start();
        } catch (IllegalStateException exception) {
            listener.onVideoError("Video could not resume.");
        }
    }

    void stop() {
        requestedUri = null;
        resumeAfterLifecyclePause = false;
        releasePlayer();
    }

    void release() {
        stop();
        releaseSurface();
    }

    private void openRequestedUri() {
        Uri uri = requestedUri;
        if (uri == null || surface == null) {
            return;
        }

        MediaPlayer created = new MediaPlayer();
        player = created;
        prepared = false;

        created.setAudioAttributes(
            new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_MEDIA)
                .setContentType(AudioAttributes.CONTENT_TYPE_MOVIE)
                .build()
        );
        created.setSurface(surface);
        created.setOnPreparedListener(this::onPrepared);
        created.setOnCompletionListener(mediaPlayer ->
            listener.onVideoStateChanged("Video finished.")
        );
        created.setOnErrorListener((mediaPlayer, what, extra) -> {
            prepared = false;
            listener.onVideoError(
                "MediaPlayer error " + what + " / " + extra + "."
            );
            return true;
        });

        try {
            created.setDataSource(context, uri);
            listener.onVideoStateChanged("Preparing local video…");
            created.prepareAsync();
        } catch (IOException | SecurityException | IllegalArgumentException
            | IllegalStateException exception) {
            listener.onVideoError(
                "Could not open the selected local video: "
                    + exception.getClass().getSimpleName()
            );
            releasePlayer();
        }
    }

    private void onPrepared(MediaPlayer preparedPlayer) {
        if (preparedPlayer != player) {
            return;
        }

        int width = preparedPlayer.getVideoWidth();
        int height = preparedPlayer.getVideoHeight();
        if (width <= 0 || height <= 0) {
            listener.onVideoError("The selected file has no usable video track.");
            releasePlayer();
            return;
        }

        prepared = true;
        listener.onVideoPrepared(width / (float) height);

        try {
            preparedPlayer.start();
            listener.onVideoStateChanged("Video playing.");
        } catch (IllegalStateException exception) {
            listener.onVideoError("The selected video could not start.");
        }
    }

    private void releasePlayer() {
        prepared = false;
        MediaPlayer active = player;
        player = null;

        if (active != null) {
            try {
                active.release();
            } catch (RuntimeException ignored) {
                // Decoder teardown can race Android's media service during shutdown.
            }
        }
    }

    private void releaseSurface() {
        Surface active = surface;
        surface = null;
        if (active != null) {
            active.release();
        }
    }
}
