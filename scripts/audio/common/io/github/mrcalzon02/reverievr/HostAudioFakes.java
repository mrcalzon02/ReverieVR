package io.github.mrcalzon02.reverievr;

import android.content.Context;
import java.util.ArrayList;
import java.util.List;

final class ControllerSpringSound { void play() { } void stop() { } void close() { } }
final class NativeCombatWaveform {
    static final int FIRE=1, RELOAD=2, READY=3, ARTILLERY=4, AIRSTRIKE=5;
}
final class NativeCombatSound {
    static final List<Integer> played = new ArrayList<>();
    void play(int id) { played.add(id); }
    void playNativeCue(int id) { played.add(id); }
    void playNativeCueSpatial(int id, float left, float right) { played.add(id); }
    void stop() { } void resumeForLifecycle() { } void close() { }
}
final class RedLedgerSound {
    static final int FIRST_CUE=22, LAST_CUE=27;
    static final List<Integer> played = new ArrayList<>();
    RedLedgerSound(Context context) { }
    void playCue(int id) { played.add(id); }
    void close() { }
}
final class NativeModuleRuntime {
    static final int FEEDBACK_FOCUS=1, FEEDBACK_ACTIVATION=2, FEEDBACK_FAILURE=4,
        FEEDBACK_WEAPON_FIRE=8, FEEDBACK_RELOAD_START=16, FEEDBACK_RELOAD_READY=32,
        FEEDBACK_ARTILLERY=64, FEEDBACK_AIRSTRIKE=128, FEEDBACK_ALL=255;
}
final class ReverieLog {
    static final List<String> messages = new ArrayList<>();
    static void incident(String tag, String message) { messages.add(message); }
    static void milestone(String tag, String message) { messages.add(message); }
    static void error(String tag, String message, Throwable error) { messages.add(message); }
}
