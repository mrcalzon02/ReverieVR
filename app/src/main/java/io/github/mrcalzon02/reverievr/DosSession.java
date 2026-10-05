package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

import java.io.File;
import java.nio.ByteBuffer;
import java.util.concurrent.locks.LockSupport;

final class DosSession implements AutoCloseable {
    interface Listener {
        void onDosSessionEnded(String message, boolean error);
    }

    private static final int AUDIO_CHUNK_FRAMES = 2048;
    private static final int MAX_AUDIO_DRAINS_PER_FRAME = 8;
    private static final long STOP_JOIN_MILLIS = 1500L;

    private final Context context;
    private final VirtualInputBus inputBus;
    private final Listener listener;
    private final Object lifecycleLock = new Object();
    private final Object runtimeAccessLock = new Object();
    private final Object audioLock = new Object();
    private final DosPauseGate pauseGate = new DosPauseGate();

    private volatile DosNativeRuntime runtime;
    private volatile Thread worker;
    private volatile boolean stopRequested;
    private volatile boolean running;
    private volatile String lastError = "";
    private volatile DosGameModule activeModule;

    private AudioTrack audioTrack;
    private int audioSampleRate;

    DosSession(Context context, VirtualInputBus inputBus, Listener listener) {
        this.context = context.getApplicationContext();
        this.inputBus = inputBus;
        this.listener = listener;
    }

    boolean start(DosGameModule module) {
        if (module == null) {
            lastError = "No DOS module was selected.";
            return false;
        }
        if (!module.isContentPresent()) {
            lastError = "The selected DOS module content is missing.";
            return false;
        }
        if (!DosNativeRuntime.isAvailable()) {
            lastError = "The native DOS runtime is not built into this APK.";
            return false;
        }

        stop();
        if (running) {
            lastError = "The previous DOS session did not stop cleanly.";
            return false;
        }

        File systemDirectory = new File(context.getFilesDir(), "dos-system");
        File saveDirectory = new File(
            new File(context.getFilesDir(), "dos-saves"),
            module.id
        );

        if (!ensureDirectory(systemDirectory) || !ensureDirectory(saveDirectory)) {
            lastError = "Could not create DOS runtime directories.";
            return false;
        }

        DosNativeRuntime created = null;
        try {
            created = new DosNativeRuntime(
                systemDirectory.getAbsolutePath(),
                saveDirectory.getAbsolutePath()
            );
            if (!created.loadContent(module.contentPath)) {
                String detail = created.getLastError();
                lastError = detail == null || detail.trim().isEmpty()
                    ? "DOSBox Pure rejected the selected module."
                    : detail.trim();
                created.close();
                return false;
            }
        } catch (RuntimeException exception) {
            if (created != null) {
                created.close();
            }
            lastError = exception.getMessage() == null
                ? "Could not start the DOS runtime."
                : exception.getMessage();
            ReverieLog.error(
                "DOS_SESSION",
                "DOS session startup failed.",
                exception
            );
            return false;
        }

        synchronized (lifecycleLock) {
            runtime = created;
            activeModule = module;
            stopRequested = false;
            pauseGate.clear();
            running = true;
            lastError = "";

            Thread thread = new Thread(this::runLoop, "ReverieVR-DOS");
            worker = thread;
            thread.start();
        }

        ReverieLog.milestone(
            "DOS_SESSION",
            "Started module=" + module.id + " name=" + module.displayName
        );
        return true;
    }

    boolean isRunning() {
        return running;
    }

    String getLastError() {
        return lastError == null ? "" : lastError;
    }

    void pauseForLifecycle() {
        pauseGate.setLifecyclePaused(true);
        releaseGuestInput();
        applyAudioPauseState();
    }

    void resumeForLifecycle() {
        pauseGate.setLifecyclePaused(false);
        applyAudioPauseState();
    }

    void pauseForOverlay() {
        pauseGate.setOverlayPaused(true);
        releaseGuestInput();
        applyAudioPauseState();
        ReverieLog.milestone(
            "DOS_SESSION",
            "Paused for VR quick overlay."
        );
    }

    void resumeFromOverlay() {
        pauseGate.setOverlayPaused(false);
        applyAudioPauseState();
        ReverieLog.milestone(
            "DOS_SESSION",
            "Resumed from VR quick overlay."
        );
    }

    private void releaseGuestInput() {
        DosNativeRuntime active = runtime;
        if (active != null) {
            try {
                active.releaseAllInput();
            } catch (RuntimeException ignored) {
            }
        }
    }

    private void applyAudioPauseState() {
        boolean shouldPause = pauseGate.isPaused();

        synchronized (audioLock) {
            if (audioTrack == null
                || audioTrack.getState()
                    != AudioTrack.STATE_INITIALIZED) {
                return;
            }

            try {
                if (shouldPause) {
                    audioTrack.pause();
                } else {
                    audioTrack.play();
                }
            } catch (IllegalStateException ignored) {
            }
        }
    }

    long getFrameSerial() {
        DosNativeRuntime active = runtime;
        if (active == null) {
            return 0L;
        }
        synchronized (runtimeAccessLock) {
            try {
                return active.getFrameSerial();
            } catch (RuntimeException ignored) {
                return 0L;
            }
        }
    }

    int getFrameWidth() {
        DosNativeRuntime active = runtime;
        if (active == null) {
            return 0;
        }
        synchronized (runtimeAccessLock) {
            try {
                return active.getFrameWidth();
            } catch (RuntimeException ignored) {
                return 0;
            }
        }
    }

    int getFrameHeight() {
        DosNativeRuntime active = runtime;
        if (active == null) {
            return 0;
        }
        synchronized (runtimeAccessLock) {
            try {
                return active.getFrameHeight();
            } catch (RuntimeException ignored) {
                return 0;
            }
        }
    }

    ByteBuffer copyLatestFrame() {
        DosNativeRuntime active = runtime;
        if (active == null) {
            return null;
        }
        synchronized (runtimeAccessLock) {
            try {
                return active.copyLatestFrame();
            } catch (RuntimeException ignored) {
                return null;
            }
        }
    }

    void stop() {
        Thread thread;
        synchronized (lifecycleLock) {
            stopRequested = true;
            pauseGate.clear();
            thread = worker;
        }

        DosNativeRuntime active = runtime;
        if (active != null) {
            try {
                active.releaseAllInput();
            } catch (RuntimeException ignored) {
            }
        }

        synchronized (audioLock) {
            if (audioTrack != null) {
                try {
                    audioTrack.pause();
                    audioTrack.flush();
                } catch (IllegalStateException ignored) {
                }
            }
        }

        if (thread != null && thread != Thread.currentThread()) {
            thread.interrupt();
            try {
                thread.join(STOP_JOIN_MILLIS);
            } catch (InterruptedException exception) {
                Thread.currentThread().interrupt();
            }
        }
    }

    @Override
    public void close() {
        stop();
    }

    private void runLoop() {
        String completionMessage = "DOS session stopped.";
        boolean error = false;
        long nextFrameNanos = System.nanoTime();

        try {
            while (!stopRequested) {
                if (pauseGate.isPaused()) {
                    LockSupport.parkNanos(10_000_000L);
                    nextFrameNanos = System.nanoTime();
                    continue;
                }

                DosNativeRuntime active = runtime;
                if (active == null) {
                    break;
                }

                active.updateInput(inputBus);
                active.runFrame();
                drainAudio(active);

                if (active.isShutdownRequested()) {
                    completionMessage = "DOSBox Pure requested session shutdown.";
                    break;
                }

                double framesPerSecond = active.getFramesPerSecond();
                if (!Double.isFinite(framesPerSecond)
                    || framesPerSecond < 10.0
                    || framesPerSecond > 240.0) {
                    framesPerSecond = 60.0;
                }

                long frameNanos = (long) (1_000_000_000.0 / framesPerSecond);
                nextFrameNanos += frameNanos;
                long sleepNanos = nextFrameNanos - System.nanoTime();

                if (sleepNanos > 0L) {
                    LockSupport.parkNanos(sleepNanos);
                } else {
                    nextFrameNanos = System.nanoTime();
                }

                if (Thread.interrupted() && stopRequested) {
                    break;
                }
            }
        } catch (Throwable throwable) {
            error = true;
            completionMessage = throwable.getMessage() == null
                ? "DOS session failed."
                : throwable.getMessage();
            lastError = completionMessage;
            ReverieLog.error(
                "DOS_SESSION",
                "DOS worker failed.",
                throwable
            );
        } finally {
            releaseAudio();

            synchronized (runtimeAccessLock) {
                DosNativeRuntime active = runtime;
                runtime = null;
                if (active != null) {
                    try {
                        active.close();
                    } catch (RuntimeException exception) {
                        ReverieLog.error(
                            "DOS_SESSION",
                            "DOS runtime close failed.",
                            exception
                        );
                    }
                }
            }

            synchronized (lifecycleLock) {
                worker = null;
                running = false;
                activeModule = null;
            }

            ReverieLog.milestone("DOS_SESSION", completionMessage);

            if (listener != null) {
                listener.onDosSessionEnded(completionMessage, error);
            }
        }
    }

    private void drainAudio(DosNativeRuntime active) {
        double reportedRate = active.getAudioSampleRate();
        if (!Double.isFinite(reportedRate)
            || reportedRate < 8000.0
            || reportedRate > 192000.0) {
            return;
        }

        int sampleRate = (int) Math.round(reportedRate);
        ensureAudioTrack(sampleRate);

        for (int drain = 0;
             drain < MAX_AUDIO_DRAINS_PER_FRAME;
             drain++) {
            ByteBuffer audio = active.readAudio(AUDIO_CHUNK_FRAMES);
            if (audio == null || !audio.hasRemaining()) {
                return;
            }

            synchronized (audioLock) {
                if (audioTrack == null) {
                    return;
                }

                while (audio.hasRemaining() && !stopRequested) {
                    int written = audioTrack.write(
                        audio,
                        audio.remaining(),
                        AudioTrack.WRITE_BLOCKING
                    );
                    if (written < 0) {
                        throw new IllegalStateException(
                            "AudioTrack write failed: " + written
                        );
                    }
                    if (written == 0) {
                        break;
                    }
                }
            }
        }
    }

    private void ensureAudioTrack(int sampleRate) {
        synchronized (audioLock) {
            if (audioTrack != null && audioSampleRate == sampleRate) {
                if (audioTrack.getPlayState() != AudioTrack.PLAYSTATE_PLAYING
                    && !paused) {
                    audioTrack.play();
                }
                return;
            }

            releaseAudioLocked();

            int minimum = AudioTrack.getMinBufferSize(
                sampleRate,
                AudioFormat.CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_16BIT
            );
            if (minimum <= 0) {
                throw new IllegalStateException(
                    "AudioTrack reported invalid minimum buffer size: " + minimum
                );
            }

            int bufferBytes = Math.max(minimum * 2, sampleRate);

            AudioTrack created = new AudioTrack.Builder()
                .setAudioAttributes(
                    new AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_GAME)
                        .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                        .build()
                )
                .setAudioFormat(
                    new AudioFormat.Builder()
                        .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                        .setSampleRate(sampleRate)
                        .setChannelMask(AudioFormat.CHANNEL_OUT_STEREO)
                        .build()
                )
                .setTransferMode(AudioTrack.MODE_STREAM)
                .setBufferSizeInBytes(bufferBytes)
                .setSessionId(AudioManager.AUDIO_SESSION_ID_GENERATE)
                .build();

            if (created.getState() != AudioTrack.STATE_INITIALIZED) {
                created.release();
                throw new IllegalStateException(
                    "AudioTrack failed to initialize."
                );
            }

            audioTrack = created;
            audioSampleRate = sampleRate;
            if (!paused) {
                created.play();
            }
        }
    }

    private void releaseAudio() {
        synchronized (audioLock) {
            releaseAudioLocked();
        }
    }

    private void releaseAudioLocked() {
        AudioTrack track = audioTrack;
        audioTrack = null;
        audioSampleRate = 0;

        if (track != null) {
            try {
                track.pause();
                track.flush();
            } catch (IllegalStateException ignored) {
            }
            track.release();
        }
    }

    private static boolean ensureDirectory(File directory) {
        return directory.isDirectory() || directory.mkdirs();
    }
}
