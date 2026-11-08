package io.github.mrcalzon02.reverievr;

import android.content.Context;
import java.util.ArrayList;
import java.util.List;

/** Routing double only. The Red Ledger lifecycle harness excludes this file
 * and compiles the production bank with the named SoundPool platform double. */
final class RedLedgerSound {
    static final int FIRST_CUE=22, LAST_CUE=27;
    static final List<Integer> played = new ArrayList<>();
    RedLedgerSound(Context context) { }
    void playCue(int id) { played.add(id); }
    void stop() { }
    void close() { }
}
