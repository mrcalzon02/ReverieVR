package android.media;

import android.content.res.AssetFileDescriptor;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** Small behavioral fake: asynchronous decode, finite voices, gain/priority
 * accounting, natural one-shot expiry, and explicit platform fault injection. */
public final class SoundPool {
    public interface OnLoadCompleteListener {
        void onLoadComplete(SoundPool pool, int sampleId, int status);
    }
    public static final List<SoundPool> instances = new ArrayList<>();
    public static final Set<Integer> rejectBuildLimits = new HashSet<>();
    public static final Set<String> rejectLoadFiles = new HashSet<>();
    public static int buildAttempts;
    public final int maxStreams;
    public final AudioAttributes attributes;
    public final List<Sample> loads = new ArrayList<>();
    public final List<Playback> plays = new ArrayList<>();
    public final List<Integer> stops = new ArrayList<>();
    public final List<Integer> unloads = new ArrayList<>();
    public final Map<Integer, Playback> live = new LinkedHashMap<>();
    public boolean released, rejectPlayback, throwPlayback;
    public int playAttempts, peakConcurrent, releaseCalls, platformSteals;
    private final Map<Integer, Sample> samples = new HashMap<>();
    private final Handler callbackHandler = new Handler(Looper.getMainLooper());
    private OnLoadCompleteListener listener;
    private int nextSound = 1, nextStream = 1;

    public static final class Sample {
        public final int id;
        public final String file;
        public final long duration;
        public boolean completed, ready;
        Sample(int id, AssetFileDescriptor file) {
            this.id = id; this.file = file.path;
            duration = (file.getLength() - 44L) * 1000L / 44_100L;
        }
    }
    public static final class Playback {
        public final int stream, sound, priority, loop;
        public final String file;
        public final long began, until;
        public final float rate;
        public float left, right;
        Playback(int stream, Sample sample, float left, float right, int priority,
                int loop, float rate) {
            this.stream = stream; sound = sample.id; file = sample.file;
            this.left = left; this.right = right; this.priority = priority;
            this.loop = loop; this.rate = rate;
            began = SystemClock.uptimeMillis();
            until = loop == -1 ? Long.MAX_VALUE : began + sample.duration;
        }
    }
    public static final class Builder {
        private int maxStreams;
        private AudioAttributes attributes;
        public Builder setMaxStreams(int value) { maxStreams = value; return this; }
        public Builder setAudioAttributes(AudioAttributes value) { attributes = value; return this; }
        public SoundPool build() {
            buildAttempts++;
            if (rejectBuildLimits.contains(maxStreams)) throw new IllegalStateException("pool unavailable");
            if (maxStreams < 1) throw new IllegalArgumentException("maxStreams");
            SoundPool pool = new SoundPool(maxStreams, attributes);
            instances.add(pool);
            return pool;
        }
    }
    private SoundPool(int maxStreams, AudioAttributes attributes) {
        this.maxStreams = maxStreams; this.attributes = attributes;
    }
    public void setOnLoadCompleteListener(OnLoadCompleteListener value) { listener = value; }
    public int load(AssetFileDescriptor file, int ignoredPriority) {
        requireOpen();
        if (rejectLoadFiles.contains(file.path)) return 0;
        Sample sample = new Sample(nextSound++, file);
        loads.add(sample); samples.put(sample.id, sample);
        return sample.id;
    }
    public boolean unload(int soundId) {
        unloads.add(soundId);
        return samples.remove(soundId) != null;
    }
    public int play(int soundId, float left, float right, int priority, int loop, float rate) {
        requireOpen(); playAttempts++;
        if (throwPlayback) throw new IllegalStateException("playback unavailable");
        if (rejectPlayback) return 0;
        Sample sample = samples.get(soundId);
        if (sample == null || !sample.ready) return 0;
        checkGain(left); checkGain(right);
        reap();
        if (live.size() >= maxStreams) {
            Playback victim = null;
            for (Playback candidate : live.values()) {
                if (victim == null || candidate.priority < victim.priority
                        || candidate.priority == victim.priority && candidate.began < victim.began)
                    victim = candidate;
            }
            if (victim != null && victim.priority > priority) return 0;
            if (victim != null) live.remove(victim.stream);
            platformSteals++;
        }
        Playback playback = new Playback(nextStream++, sample, left, right, priority, loop, rate);
        plays.add(playback); live.put(playback.stream, playback);
        peakConcurrent = Math.max(peakConcurrent, live.size());
        return playback.stream;
    }
    public void stop(int stream) { requireOpen(); stops.add(stream); live.remove(stream); }
    public void setVolume(int stream, float left, float right) {
        requireOpen(); checkGain(left); checkGain(right);
        Playback playback = live.get(stream);
        if (playback == null) throw new IllegalArgumentException("unknown stream");
        playback.left = left; playback.right = right;
    }
    public void release() { releaseCalls++; released = true; live.clear(); }
    private void requireOpen() { if (released) throw new IllegalStateException("released pool used"); }
    private static void checkGain(float value) {
        if (!Float.isFinite(value) || value < 0f || value > 1f)
            throw new AssertionError("invalid playback gain: " + value);
    }
    public void reap() { live.values().removeIf(playback -> playback.until <= SystemClock.uptimeMillis()); }
    public int liveCount() { reap(); return live.size(); }
    public Playback playingFile(String file) {
        reap();
        for (Playback playback : live.values()) if (playback.file.endsWith("/" + file)) return playback;
        return null;
    }
    public Sample lastSample(String file) {
        for (int i = loads.size() - 1; i >= 0; i--)
            if (loads.get(i).file.endsWith("/" + file)) return loads.get(i);
        throw new AssertionError("fixture not loaded: " + file);
    }
    /** Also permits delivery after release, to expose stale callback bugs. */
    public void complete(Sample sample, int status) {
        sample.completed = true; sample.ready = status == 0;
        OnLoadCompleteListener completion = listener;
        if (completion != null) callbackHandler.post(() -> completion.onLoadComplete(this, sample.id, status));
    }
    public void completeAll(int status) {
        for (Sample sample : new ArrayList<>(loads)) if (!sample.completed) complete(sample, status);
    }
    public static void completeAllLoads() {
        for (SoundPool pool : new ArrayList<>(instances)) if (!pool.released) pool.completeAll(0);
    }
    public static SoundPool current(int maxStreams) {
        for (int i = instances.size() - 1; i >= 0; i--) {
            SoundPool pool = instances.get(i);
            if (!pool.released && pool.maxStreams == maxStreams) return pool;
        }
        throw new AssertionError("missing pool with voice budget " + maxStreams);
    }
    public static int totalLive() {
        int result = 0;
        for (SoundPool pool : instances) result += pool.liveCount();
        return result;
    }
    public static int totalPlays() {
        int result = 0;
        for (SoundPool pool : instances) result += pool.plays.size();
        return result;
    }
    public static void reset() {
        instances.clear(); rejectBuildLimits.clear(); rejectLoadFiles.clear(); buildAttempts = 0;
    }
}
