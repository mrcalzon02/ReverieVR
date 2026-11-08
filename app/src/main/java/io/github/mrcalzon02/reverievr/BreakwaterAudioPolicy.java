package io.github.mrcalzon02.reverievr;

import java.util.Arrays;

/** Pure, bounded playback policy. IDs 1..27 retain their existing ABI meaning. */
final class BreakwaterAudioPolicy {
    static final String ASSET_ROOT = "native/breakwater/audio/";
    static final int LAST_NATIVE_CUE = 47;
    static final int FIRST_LOOP = 41, LAST_LOOP = 47, LOOP_COUNT = 7;
    static final int CONTROL = 0, EFFECT = 1, LOOP = 2;
    static final int CONTROL_VOICES = 2, EFFECT_VOICES = 4, LOOP_VOICES = 7;
    static final long LOOP_LEASE_MS = 900L;
    static final int FOCUS = 48, ACTIVATE = 49, FAILURE = 50;
    static final int RELOAD_START = 51, RELOAD_READY = 52;
    static final int ARTILLERY = 53, AIRSTRIKE = 54;
    static final int CUE_COUNT = 55;

    static final class Cue {
        final int id, pool, priority;
        final String file;
        final float gain;
        final long minimumIntervalMs;
        Cue(int id, String file, int pool, float gain, int priority, long interval) {
            this.id=id; this.file=file; this.pool=pool; this.gain=gain;
            this.priority=priority; this.minimumIntervalMs=interval;
        }
    }

    private static final Cue[] CUES = new Cue[CUE_COUNT];
    static {
        effect(1,"weapon_machine",3,45); effect(2,"weapon_heavy",3,60);
        effect(3,"weapon_flak",3,60); effect(4,"weapon_missile",3,100);
        effect(5,"weapon_cannon",3,100); effect(6,"impact_hull",1,65);
        effect(7,"impact_water",1,100); effect(8,"boat_destroyed",2,100);
        effect(9,"gunboat_salvo",1,120); effect(10,"infantry_volley",1,120);
        effect(11,"aircraft_bomb",2,120); effect(12,"aircraft_strafe",1,120);
        effect(13,"sam_launch",3,100); control(14,"wave_start",3,300);
        effect(15,"shore_collision",1,120); control(16,"shop_purchase",3,80);
        control(17,"wave_clear",3,300); control(18,"victory",3,500);
        control(19,"defeat",3,500); effect(20,"armored_gun",2,100);
        effect(21,"armor_destroyed",2,100);
        // Red Ledger's 22..27 belong to its existing bank, never this one.
        control(28,"equip",2,90); control(29,"dry_fire",3,180);
        control(30,"aim_limit",3,180); control(31,"support_arm",2,100);
        control(32,"support_disarm",2,100); control(33,"station_move",2,100);
        effect(34,"sand_impact",1,80); control(35,"menu_open",2,100);
        control(36,"menu_back",2,100); control(37,"ui_activate",2,100);
        control(38,"menu_open",2,150); control(39,"cooldown",3,180);
        control(40,"support_unavailable",3,180);
        loop(41,"surf_loop",.12f); loop(42,"wind_loop",.10f);
        loop(43,"night_loop",.08f); loop(44,"boat_loop",.08f);
        loop(45,"rowing_loop",.08f); loop(46,"aircraft_loop",.08f);
        loop(47,"traverse_loop",.06f);
        control(FOCUS,"ui_focus",1,100); control(ACTIVATE,"ui_activate",2,75);
        control(FAILURE,"ui_failure",3,150);
        control(RELOAD_START,"reload_start",2,100);
        control(RELOAD_READY,"reload_ready",2,100);
        effect(ARTILLERY,"artillery",3,150); effect(AIRSTRIKE,"airstrike",3,150);
    }
    private static void effect(int id,String file,int priority,long interval) {
        CUES[id]=new Cue(id,file+".wav",EFFECT,.22f,priority,interval);
    }
    private static void control(int id,String file,int priority,long interval) {
        CUES[id]=new Cue(id,file+".wav",CONTROL,.20f,priority,interval);
    }
    private static void loop(int id,String file,float gain) {
        CUES[id]=new Cue(id,file+".wav",LOOP,gain,1,0L);
    }
    static Cue cue(int id) { return id>=0 && id<CUES.length ? CUES[id] : null; }
    static boolean isLoop(int id) { return id>=FIRST_LOOP && id<=LAST_LOOP; }
    static boolean isSpatialCue(int id) { return id>=1 && id<=21 || isLoop(id); }
    static boolean validGains(float left,float right) {
        return Float.isFinite(left) && Float.isFinite(right)
            && left>=0f && left<=1f && right>=0f && right<=1f;
    }
    /** An unsuccessful action must never sound like both success and failure. */
    static int controlForFeedback(int flags) {
        if((flags & 4)!=0) return FAILURE;
        if((flags & 2)!=0) return ACTIVATE;
        return (flags & 1)!=0 ? FOCUS : -1;
    }

    private final long[] lastPlayed = new long[CUE_COUNT];
    private final long[] loopUntil = new long[LOOP_COUNT];
    private final float[] loopLeft = new float[LOOP_COUNT];
    private final float[] loopRight = new float[LOOP_COUNT];
    private boolean sessionActive, lifecycleActive, suspended;

    BreakwaterAudioPolicy() { resetTransient(); }
    boolean enabled() { return sessionActive && lifecycleActive && !suspended; }
    boolean shellEnabled() { return sessionActive && lifecycleActive; }
    void setSessionActive(boolean value) { sessionActive=value; resetTransient(); }
    void setLifecycleActive(boolean value) { lifecycleActive=value; resetTransient(); }
    void setSuspended(boolean value) {
        if(suspended!=value) { suspended=value; resetTransient(); }
    }
    void resetTransient() {
        Arrays.fill(lastPlayed,Long.MIN_VALUE);
        clearLoops();
    }
    void clearLoops() {
        Arrays.fill(loopUntil,0L); Arrays.fill(loopLeft,0f); Arrays.fill(loopRight,0f);
    }
    boolean canPlayOneShot(int id,long now,boolean shellControl) {
        Cue definition=cue(id);
        if(!(shellControl?shellEnabled():enabled()) || definition==null ||
           definition.pool==LOOP || shellControl && definition.pool!=CONTROL || now<0L) return false;
        long prior=lastPlayed[id];
        if(prior!=Long.MIN_VALUE && now-prior<definition.minimumIntervalMs) return false;
        return true;
    }
    void recordOneShot(int id,long now) {
        lastPlayed[id]=now;
    }
    boolean allowOneShot(int id,long now) {
        if(!canPlayOneShot(id,now,false)) return false;
        recordOneShot(id,now);
        return true;
    }
    boolean refreshLoop(int id,float left,float right,long now) {
        if(!enabled() || !isLoop(id) || !validGains(left,right) || now<0L) return false;
        int i=id-FIRST_LOOP;
        loopUntil[i]=now+LOOP_LEASE_MS;
        loopLeft[i]=left; loopRight[i]=right;
        return true;
    }
    float loopGain(int id,boolean left,long now) {
        if(!enabled() || !isLoop(id) || now<0L) return 0f;
        int i=id-FIRST_LOOP;
        if(now>=loopUntil[i]) return 0f;
        return (left?loopLeft[i]:loopRight[i])*cue(id).gain;
    }

    /** Worst-case sum with verified per-category PCM peak ceilings.
     * MusicVolumePolicy caps the independent soundtrack at 0.50. */
    static double maximumEffectsPeak() {
        double loops=0.;
        for(int id=FIRST_LOOP;id<=LAST_LOOP;id++) loops+=cue(id).gain;
        return EFFECT_VOICES*.22*Math.pow(10.,-9./20.)
            + CONTROL_VOICES*.20*Math.pow(10.,-12./20.)
            + loops*Math.pow(10.,-18./20.);
    }
}
