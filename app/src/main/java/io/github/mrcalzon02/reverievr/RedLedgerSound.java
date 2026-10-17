package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.SoundPool;
import java.util.HashSet;
import java.util.Set;

/** Six owner-exported Red Ledger effects, preloaded once, not generated per eye. */
final class RedLedgerSound implements AutoCloseable {
    static final int FIRST_CUE = 22;
    static final int LAST_CUE = 27;

    private final SoundPool pool;
    private final int[] soundIds = new int[6];
    private final Set<Integer> ready = new HashSet<>();
    private boolean closed;

    RedLedgerSound(Context context) {
        if (context == null) {
            pool = null;
            return;
        }
        SoundPool result = null;
        try {
            result = new SoundPool.Builder()
                .setMaxStreams(4)
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
        } catch (RuntimeException error) {
            if (result != null) result.release();
            result = null;
        }
        pool = result;
    }

    void playCue(int cueId) {
        if (pool == null || closed || cueId < FIRST_CUE || cueId > LAST_CUE)
            return;
        final int sample = soundIds[cueId - FIRST_CUE];
        if (sample == 0) return;
        synchronized (ready) {
            if (!ready.contains(sample)) return;
        }
        try {
            pool.play(sample, .48f, .48f, 1, 0, 1f);
        } catch (RuntimeException ignored) {
            // Audio route loss cannot interrupt a completed bar transaction.
        }
    }

    @Override public void close() {
        synchronized (ready) {
            closed = true;
            ready.clear();
        }
        if (pool != null) pool.release();
    }
}
