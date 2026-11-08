package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;

/** Shell-owned Breakwater recording bank. All calls and callbacks use the UI
 * thread; decode is asynchronous and no work runs in stereo draw callbacks. */
final class BreakwaterAudio implements AutoCloseable {
    private static final String TAG = "BREAKWATER_AUDIO";
    private static final long MIX_TICK_MS = 50L;
    private static final float FADE_STEP = .04f;
    private final Context context;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final BreakwaterAudioPolicy policy = new BreakwaterAudioPolicy();
    private final SoundPool[] pools = new SoundPool[3];
    private final Sample[] byCue = new Sample[BreakwaterAudioPolicy.CUE_COUNT];
    private final ArrayList<Sample> samples = new ArrayList<>();
    private final Voice[][] voices = {
        new Voice[BreakwaterAudioPolicy.CONTROL_VOICES],
        new Voice[BreakwaterAudioPolicy.EFFECT_VOICES]
    };
    private final int[] loopStreams = new int[BreakwaterAudioPolicy.LOOP_COUNT];
    private final float[] leftGain = new float[BreakwaterAudioPolicy.LOOP_COUNT];
    private final float[] rightGain = new float[BreakwaterAudioPolicy.LOOP_COUNT];
    private final long[] poolRetryAt = new long[3];
    private final long[] loadRetryAt = new long[3];
    private boolean sessionActive, lifecycleActive, suspended, closed, tickPending;
    private boolean readyLogged;
    private long diagnosticAfterMs;

    private static final class Sample {
        final String file;
        final int pool;
        int soundId, durationMs;
        boolean ready;
        Sample(String file,int pool) { this.file=file; this.pool=pool; }
    }
    private static final class Voice {
        int stream, priority;
        long until;
    }

    BreakwaterAudio(Context context) {
        this.context=context==null?null:context.getApplicationContext();
        HashMap<String,Sample> unique=new HashMap<>();
        for(int id=0;id<byCue.length;id++) {
            BreakwaterAudioPolicy.Cue cue=BreakwaterAudioPolicy.cue(id);
            if(cue==null) continue;
            String key=cue.pool+":"+cue.file;
            Sample sample=unique.get(key);
            if(sample==null) {
                sample=new Sample(cue.file,cue.pool);
                unique.put(key,sample); samples.add(sample);
            }
            byCue[id]=sample;
        }
        for(Voice[] group:voices) for(int i=0;i<group.length;i++) group[i]=new Voice();
    }

    void setSessionActive(boolean active) {
        if(closed || sessionActive==active) return;
        sessionActive=active; suspended=false;
        policy.setSessionActive(active); policy.setSuspended(false);
        if(active) {
            if(lifecycleActive) prepare();
        } else releasePools();
    }
    void setSuspended(boolean value) {
        if(closed || suspended==value) return;
        suspended=value; policy.setSuspended(value);
        // Paused playback is discarded, never auto-resumed as an old gunshot.
        if(value) stopAll();
    }
    void pauseForLifecycle() {
        lifecycleActive=false; policy.setLifecycleActive(false); stopAll();
    }
    void resumeForLifecycle() {
        if(closed) return;
        if(!lifecycleActive) {
            lifecycleActive=true; policy.setLifecycleActive(true);
        }
        if(sessionActive) prepare();
    }

    /** False reports unavailable playback to the caller; Breakwater events
     * remain exclusively owned by this normalized bank while it recovers. */
    boolean playNativeCue(int id,float left,float right) {
        if(id<1 || id>BreakwaterAudioPolicy.LAST_NATIVE_CUE ||
           BreakwaterAudioPolicy.cue(id)==null ||
           !BreakwaterAudioPolicy.validGains(left,right)) return false;
        if(!policy.enabled()) return false;
        long now=SystemClock.uptimeMillis();
        prepareIfDue(now);
        if(BreakwaterAudioPolicy.isLoop(id)) {
            policy.refreshLoop(id,left,right,now);
            scheduleTick();
            return true;
        }
        return playOneShot(id,left,right,now,false);
    }

    /** Shell controls share the same bounded control pool while game audio is
     * suspended. The host also gates this entry point on foreground/audio focus. */
    boolean playShellFeedback(int flags) {
        if(!policy.shellEnabled()) return false;
        int control=BreakwaterAudioPolicy.controlForFeedback(flags & 7);
        if(control<0) return false;
        long now=SystemClock.uptimeMillis();
        prepareIfDue(now);
        return playOneShot(control,1f,1f,now,true);
    }

    boolean playFeedback(int flags) {
        if(!policy.enabled()) return false;
        long now=SystemClock.uptimeMillis();
        prepareIfDue(now);
        boolean handled=true;
        int control=BreakwaterAudioPolicy.controlForFeedback(flags);
        // Preflight the complete request. Runtime failure may still happen
        // after one component, so the host never replays it through another bank.
        if(control>=0 && !ready(control) ||
           (flags & 8)!=0 && !ready(1) ||
           (flags & 16)!=0 && !ready(BreakwaterAudioPolicy.RELOAD_START) ||
           (flags & 32)!=0 && !ready(BreakwaterAudioPolicy.RELOAD_READY) ||
           (flags & 64)!=0 && !ready(BreakwaterAudioPolicy.ARTILLERY) ||
           (flags & 128)!=0 && !ready(BreakwaterAudioPolicy.AIRSTRIKE)) return false;
        if(control>=0) handled=playOneShot(control,1f,1f,now,false);
        if((flags & 8)!=0) handled=playOneShot(1,1f,1f,now,false) && handled;
        if((flags & 16)!=0) handled=playOneShot(BreakwaterAudioPolicy.RELOAD_START,1f,1f,now,false) && handled;
        if((flags & 32)!=0) handled=playOneShot(BreakwaterAudioPolicy.RELOAD_READY,1f,1f,now,false) && handled;
        if((flags & 64)!=0) handled=playOneShot(BreakwaterAudioPolicy.ARTILLERY,1f,1f,now,false) && handled;
        if((flags & 128)!=0) handled=playOneShot(BreakwaterAudioPolicy.AIRSTRIKE,1f,1f,now,false) && handled;
        return handled;
    }
    private boolean ready(int id) {
        Sample sample=byCue[id];
        return sample!=null && sample.ready && pools[sample.pool]!=null;
    }

    private boolean playOneShot(int id,float left,float right,long now,boolean shellControl) {
        Sample sample=byCue[id];
        if(sample==null || !sample.ready || pools[sample.pool]==null) return false;
        if(!policy.canPlayOneShot(id,now,shellControl)) return true;
        BreakwaterAudioPolicy.Cue cue=BreakwaterAudioPolicy.cue(id);
        Voice[] group=voices[cue.pool];
        Voice chosen=null;
        for(Voice voice:group) {
            if(voice.stream==0 || voice.until<=now) { chosen=voice; break; }
            if(chosen==null || voice.priority<chosen.priority ||
               voice.priority==chosen.priority && voice.until<chosen.until) chosen=voice;
        }
        if(chosen==null || chosen.until>now && chosen.priority>cue.priority) {
            policy.recordOneShot(id,now); // Intentional priority suppression.
            return true;
        }
        SoundPool pool=pools[cue.pool];
        try {
            if(chosen.stream!=0) pool.stop(chosen.stream);
            int stream=pool.play(sample.soundId,left*cue.gain,
                right*cue.gain,cue.priority,0,1f);
            if(stream==0) {
                recoverPool(cue.pool,now,"No playback voice for "+sample.file);
                return false;
            }
            chosen.stream=stream; chosen.priority=cue.priority;
            chosen.until=now+sample.durationMs+20L;
            policy.recordOneShot(id,now); // Failed platform calls never consume this deadline.
            return true;
        } catch(RuntimeException failure) {
            recoverPool(cue.pool,now,"Playback rejected for "+sample.file+": "+failure);
            return false;
        }
    }

    private void prepareIfDue(long now) {
        for(int group=0;group<pools.length;group++) {
            if(pools[group]==null && now>=poolRetryAt[group] ||
               loadRetryAt[group]!=0L && now>=loadRetryAt[group]) {
                prepare(); return;
            }
        }
    }

    private void prepare() {
        if(closed || !sessionActive || !lifecycleActive || context==null) return;
        final long now=SystemClock.uptimeMillis();
        final int[] limits={BreakwaterAudioPolicy.CONTROL_VOICES,
            BreakwaterAudioPolicy.EFFECT_VOICES,BreakwaterAudioPolicy.LOOP_VOICES};
        AudioAttributes attributes=new AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_GAME)
            .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build();
        for(int group=0;group<pools.length;group++) {
            if(pools[group]!=null || now<poolRetryAt[group]) continue;
            final int poolIndex=group;
            try {
                SoundPool created=new SoundPool.Builder().setMaxStreams(limits[group])
                    .setAudioAttributes(attributes).build();
                pools[group]=created; poolRetryAt[group]=0L;
                created.setOnLoadCompleteListener((pool,soundId,status)-> {
                    // Pool identity is the generation: old completions remain inert.
                    if(closed || pools[poolIndex]!=pool || !sessionActive) return;
                    for(Sample sample:samples) {
                        if(sample.pool!=poolIndex || sample.soundId!=soundId) continue;
                        sample.ready=status==0;
                        if(status!=0) {
                            sample.soundId=0;
                            loadRetryAt[poolIndex]=SystemClock.uptimeMillis()+3000L;
                            try { pool.unload(soundId); }
                            catch(RuntimeException failure) {
                                recoverPool(poolIndex,SystemClock.uptimeMillis(),
                                    "Could not unload failed decode: "+sample.file);
                            }
                            ReverieLog.incident(TAG,"Decode failed: "+sample.file+" status="+status);
                        }
                        break;
                    }
                    if(!readyLogged) {
                        boolean all=true;
                        for(Sample sample:samples) if(!sample.ready) all=false;
                        if(all) {
                            readyLogged=true;
                            ReverieLog.milestone(TAG,"Loaded "+samples.size()+
                                " normalized clips; voices=2 control/4 combat/7 environment");
                        }
                    }
                    if(status==0 && policy.enabled()) scheduleTick();
                });
            } catch(RuntimeException failure) {
                recoverPool(group,now,"Could not create sound group "+group+": "+failure);
            }
        }
        // Retry deadlines are per pool and survive repeated lifecycle/state sync.
        for(int group=0;group<pools.length;group++) {
            if(pools[group]==null || now<loadRetryAt[group]) continue;
            loadRetryAt[group]=0L;
            for(Sample sample:samples) {
                if(sample.pool!=group || sample.soundId!=0) continue;
                try(AssetFileDescriptor file=context.getAssets().openFd(
                        BreakwaterAudioPolicy.ASSET_ROOT+sample.file)) {
                    long bytes=file.getLength();
                    // Canonical checked WAVs: PCM16 mono 22050 Hz, 44-byte header.
                    if(bytes<=44L || bytes>1000000L) throw new IOException("Invalid bank size "+bytes);
                    sample.durationMs=(int)((bytes-44L)*1000L/44100L);
                    sample.soundId=pools[group].load(file,1);
                    if(sample.soundId==0) {
                        recoverPool(group,now,"SoundPool rejected load: "+sample.file);
                        break;
                    }
                } catch(IOException failure) {
                    sample.soundId=0; sample.ready=false;
                    loadRetryAt[group]=now+3000L;
                    ReverieLog.error(TAG,"Could not load "+sample.file,failure);
                } catch(RuntimeException failure) {
                    recoverPool(group,now,"Load failed for "+sample.file+": "+failure);
                    break;
                }
            }
        }
    }

    /** Invalidate just the failing category; unrelated streams keep playing. */
    private void recoverPool(int group,long now,String message) {
        SoundPool failed=pools[group]; pools[group]=null;
        poolRetryAt[group]=now+3000L; loadRetryAt[group]=0L;
        if(group<voices.length) {
            for(Voice voice:voices[group]) {
                if(voice.stream!=0 && failed!=null) {
                    try { failed.stop(voice.stream); } catch(RuntimeException ignored) { }
                }
                voice.stream=0; voice.until=0L;
            }
        } else {
            handler.removeCallbacks(mixTick); tickPending=false;
            policy.clearLoops();
            for(int i=0;i<loopStreams.length;i++) {
                if(loopStreams[i]!=0 && failed!=null) {
                    try { failed.stop(loopStreams[i]); } catch(RuntimeException ignored) { }
                }
                loopStreams[i]=0; leftGain[i]=0f; rightGain[i]=0f;
            }
        }
        for(Sample sample:samples) if(sample.pool==group) {
            sample.soundId=0; sample.ready=false;
        }
        readyLogged=false;
        if(failed!=null) {
            try { failed.release(); } catch(RuntimeException ignored) { }
        }
        if(now>=diagnosticAfterMs) {
            diagnosticAfterMs=now+3000L;
            ReverieLog.incident(TAG,message+"; retrying sound group after 3 seconds");
        }
    }
    private void scheduleTick() {
        if(!tickPending && policy.enabled() && !closed) {
            tickPending=true; handler.post(mixTick);
        }
    }
    private final Runnable mixTick = new Runnable() {
        @Override public void run() {
            tickPending=false;
            if(closed || !policy.enabled()) return;
            long now=SystemClock.uptimeMillis();
            prepareIfDue(now);
            SoundPool pool=pools[BreakwaterAudioPolicy.LOOP];
            if(pool==null) return;
            boolean keepTicking=false;
            for(int i=0;i<loopStreams.length;i++) {
                int id=BreakwaterAudioPolicy.FIRST_LOOP+i;
                float targetLeft=policy.loopGain(id,true,now);
                float targetRight=policy.loopGain(id,false,now);
                leftGain[i]=approach(leftGain[i],targetLeft);
                rightGain[i]=approach(rightGain[i],targetRight);
                Sample sample=byCue[id];
                try {
                    if(leftGain[i]==0f && rightGain[i]==0f) {
                        if(loopStreams[i]!=0) pool.stop(loopStreams[i]);
                        loopStreams[i]=0; continue;
                    }
                    keepTicking=true;
                    if(!sample.ready) continue;
                    if(loopStreams[i]==0) {
                        loopStreams[i]=pool.play(sample.soundId,0f,0f,1,-1,1f);
                        if(loopStreams[i]==0) {
                            recoverPool(BreakwaterAudioPolicy.LOOP,now,
                                "Loop voice rejected: "+sample.file);
                            return;
                        }
                    }
                    if(loopStreams[i]!=0) pool.setVolume(loopStreams[i],leftGain[i],rightGain[i]);
                } catch(RuntimeException failure) {
                    recoverPool(BreakwaterAudioPolicy.LOOP,now,
                        "Loop playback failed: "+sample.file+": "+failure);
                    return;
                }
            }
            if(keepTicking && policy.enabled()) {
                tickPending=true; handler.postDelayed(this,MIX_TICK_MS);
            }
        }
    };
    private static float approach(float current,float target) {
        return Math.abs(target-current)<=FADE_STEP ? target
            : current+Math.copySign(FADE_STEP,target-current);
    }

    private void stopAll() {
        handler.removeCallbacks(mixTick); tickPending=false;
        for(int group=0;group<voices.length;group++) for(Voice voice:voices[group]) {
            if(voice.stream!=0 && pools[group]!=null) {
                try { pools[group].stop(voice.stream); } catch(RuntimeException ignored) { }
            }
            voice.stream=0; voice.until=0L;
        }
        SoundPool loopPool=pools[BreakwaterAudioPolicy.LOOP];
        for(int i=0;i<loopStreams.length;i++) {
            if(loopStreams[i]!=0 && loopPool!=null) {
                try { loopPool.stop(loopStreams[i]); } catch(RuntimeException ignored) { }
            }
            loopStreams[i]=0; leftGain[i]=0f; rightGain[i]=0f;
        }
    }
    private void releasePools() {
        stopAll();
        for(int i=0;i<pools.length;i++) {
            SoundPool previous=pools[i]; pools[i]=null;
            if(previous!=null) {
                try { previous.release(); } catch(RuntimeException ignored) { }
            }
            poolRetryAt[i]=0L; loadRetryAt[i]=0L;
        }
        for(Sample sample:samples) { sample.soundId=0; sample.ready=false; }
        readyLogged=false; diagnosticAfterMs=0L;
    }
    @Override public void close() {
        if(closed) return;
        closed=true; sessionActive=false;
        policy.setSessionActive(false); releasePools();
    }
}
