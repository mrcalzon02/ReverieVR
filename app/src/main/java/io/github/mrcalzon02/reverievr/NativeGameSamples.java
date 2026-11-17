package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.SoundPool;
import java.util.HashSet;
import java.util.Set;

/** Shared recorded one-shots for Red Ledger and Guess the Door. */
final class NativeGameSamples implements AutoCloseable {
    static final int FIRST_CUE = 22;
    static final int LAST_CUE = 27;
    static final int GUESS_LEVER_VOICE = 48;
    static boolean supports(int cueId) {
        return (cueId >= FIRST_CUE && cueId <= LAST_CUE) || cueId == GUESS_LEVER_VOICE;
    }
    private static final int MAX_STREAMS = 4;

    private final SoundPool pool;
    private final int[] soundIds = new int[7];
    private final Set<Integer> ready = new HashSet<>();
    private final int[] streams = new int[MAX_STREAMS];
    private int nextStream;
    private boolean closed;

    NativeGameSamples(Context context) {
        if (context == null) {
            pool = null;
            return;
        }
        SoundPool result = null;
        try {
            result = new SoundPool.Builder()
                .setMaxStreams(MAX_STREAMS)
                .setAudioAttributes(new AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_GAME)
                    .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                    .build())
                .build();
            final SoundPool active = result;
            result.setOnLoadCompleteListener((soundPool, sampleId, status) -> {
                synchronized (ready) {
                    if (!closed && soundPool == active && status == 0)
                        ready.add(sampleId);
                }
            });
            soundIds[0] = result.load(context, R.raw.red_ledger_click, 1);
            soundIds[1] = result.load(context, R.raw.red_ledger_metal, 1);
            soundIds[2] = result.load(context, R.raw.red_ledger_coins, 1);
            soundIds[3] = result.load(context, R.raw.red_ledger_book, 1);
            soundIds[4] = result.load(context, R.raw.red_ledger_door, 1);
            soundIds[5] = result.load(context, R.raw.red_ledger_drawer, 1);
            soundIds[6] = result.load(context, R.raw.guess_lever_crunk, 1);
        } catch (RuntimeException error) {
            if (result != null) result.release();
            result = null;
        }
        pool = result;
    }

    void playCue(int cueId) {
        if (pool == null || !supports(cueId))
            return;
        final int sample = soundIds[cueId == GUESS_LEVER_VOICE ? 6 : cueId - FIRST_CUE];
        if (sample == 0) return;
        synchronized (ready) {
            if (closed || !ready.contains(sample)) return;
            // Explicitly retire the oldest owned voice before reusing its
            // bounded slot. Do not depend on SoundPool's implicit eviction.
            if (!stopStream(nextStream)) return;
            try {
                final int stream = pool.play(sample, .48f, .48f, 1, 0, 1f);
                if (stream != 0) {
                    streams[nextStream] = stream;
                    nextStream = (nextStream + 1) % streams.length;
                }
            } catch (RuntimeException ignored) {
                // Audio route loss cannot interrupt a completed bar transaction.
            }
        }
    }

    // Caller holds ready; a failed stop retains its ID for a bounded retry.
    private boolean stopStream(int slot) {
        if (streams[slot] == 0) return true;
        try {
            pool.stop(streams[slot]);
            streams[slot] = 0;
            return true;
        } catch (RuntimeException ignored) {
            return false;
        }
    }

    void stop() {
        synchronized (ready) {
            if (closed) return;
            for (int i = 0; i < streams.length; i++) stopStream(i);
            nextStream = 0;
        }
    }

    @Override public void close() {
        synchronized (ready) {
            if (closed) return;
            stop();
            closed = true;
            ready.clear();
            if (pool != null) {
                try { pool.release(); } catch (RuntimeException ignored) { }
            }
        }
    }
}
