package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.media.SoundPool;
import android.os.Handler;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;

/** Real RedLedgerSound, UiFeedback and Breakwater bank; only Android edges are
 * named platform doubles. WAV metadata comes from the actual repository files.
 * This does not claim Android decoder, physical audio, focus or device results. */
public final class RedLedgerAudioLifecycleHarness {
    private static final String[] FILES={"red_ledger_click.wav", "red_ledger_metal.wav",
        "red_ledger_coins.wav", "red_ledger_book.wav", "red_ledger_door.wav",
        "red_ledger_drawer.wav"};
    private static final int[] RESOURCES={R.raw.red_ledger_click, R.raw.red_ledger_metal,
        R.raw.red_ledger_coins, R.raw.red_ledger_book, R.raw.red_ledger_door,
        R.raw.red_ledger_drawer};
    private static final long[] DURATION_MS=new long[FILES.length];
    private static int checks, scenarios;

    private static void check(boolean value, String message) {
        checks++;
        if(!value) throw new AssertionError(message);
    }
    private static void scenario(String name, Runnable test) {
        Handler.reset(); SoundPool.reset();
        for(int i=0;i<FILES.length;i++)
            SoundPool.registerRawSample(RESOURCES[i], "raw/"+FILES[i], DURATION_MS[i]);
        try {
            test.run();
            check(SoundPool.totalLive()==0, "no live stream after "+name);
            for(SoundPool pool:SoundPool.instances)
                check(pool.released && pool.releaseCalls==1, "pool released once: "+name);
            scenarios++;
            System.out.println("PASS: "+name);
        } catch(Throwable failure) {
            throw new AssertionError("Scenario: "+name, failure);
        }
    }
    private static final class Fixture implements AutoCloseable {
        final UiFeedback ui=new UiFeedback(new Context());
        final SoundPool ledger=SoundPool.instances.get(0);
        Fixture() { this(true); }
        Fixture(boolean ready) {
            start("red-ledger");
            if(ready) { SoundPool.completeAllLoads(); Handler.drain(); }
        }
        void start(String module) {
            ui.setNativeModuleSession(module);
            ui.setAudioFocusAllowed(true);
            ui.setNativeAudioSuspended(false);
            Handler.drain();
        }
        void play(int cue) { ui.nativeSoundCue(cue); Handler.drain(); }
        @Override public void close() { ui.close(); Handler.drain(); }
    }

    private static void lifecyclePause() {
        try(Fixture f=new Fixture()) {
            f.play(27); Handler.advanceBy(100L);
            check(f.ledger.liveCount()==1, "actual drawer recording still live at 100 ms");
            System.out.println("Drawer source duration="+DURATION_MS[5]
                +" ms; live before pause at 100 ms="+f.ledger.liveCount());
            f.ui.pauseForLifecycle();
            check(f.ledger.liveCount()==0, "Activity pause stops already-playing Red Ledger stream");
            f.ui.resumeForLifecycle(); Handler.drain();
            check(f.ledger.liveCount()==0, "resume cannot resurrect an interrupted sample");
            f.play(27);
            check(f.ledger.liveCount()==0, "resume waits for fresh interaction/listener frame");
            f.ui.setNativeAudioSuspended(false); Handler.drain(); f.play(27);
            check(f.ledger.liveCount()==1, "fresh validated cue works after resume");
        }
    }
    private static void quickMenuAndPointerLoss() {
        try(Fixture f=new Fixture()) {
            f.play(27); f.ui.nativeSoundCue(26);
            f.ui.setNativeAudioSuspended(true); Handler.drain();
            check(f.ledger.liveCount()==0, "Quick Menu/pointer loss stops active stream");
            check(f.ledger.plays.size()==1, "queued pre-menu cue is discarded");
            int stops=f.ledger.stops.size();
            f.ui.setNativeAudioSuspended(true); Handler.drain();
            check(f.ledger.stops.size()==stops, "repeated suspended frame adds no stop work");
            f.ui.setNativeAudioSuspended(false); Handler.drain();
            check(f.ledger.liveCount()==0, "unsuspend has no automatic replay");
            f.play(26); check(f.ledger.liveCount()==1, "fresh cue after menu works");
        }
    }
    private static void homeAndReplacement() {
        try(Fixture f=new Fixture()) {
            f.play(27); f.ui.nativeSoundCue(26);
            f.ui.setNativeModuleSession(null); Handler.drain();
            check(f.ledger.liveCount()==0, "Home stops old stream");
            check(f.ledger.plays.size()==1, "old queued cue cannot cross Home boundary");
            f.start("red-ledger");
            check(f.ledger.liveCount()==0, "new session begins quiet");
            f.play(22); check(f.ledger.liveCount()==1, "new session remains playable");
        }
    }
    private static void collapsedRestart() {
        try(Fixture f=new Fixture()) {
            f.play(27); f.ui.nativeSoundCue(26);
            f.ui.setNativeModuleSession(null);
            f.ui.setNativeModuleSession("red-ledger");
            f.ui.setNativeAudioSuspended(false); Handler.drain();
            check(f.ledger.liveCount()==0, "stop/start in one UI turn stops previous voice");
            check(f.ledger.plays.size()==1, "generation drops prior queued cue");
            f.play(25); check(f.ledger.liveCount()==1, "replacement accepts fresh cue");
        }
    }
    private static void focusLoss() {
        try(Fixture f=new Fixture()) {
            f.play(27); f.ui.nativeSoundCue(26);
            f.ui.setAudioFocusAllowed(false); Handler.drain();
            check(f.ledger.liveCount()==0, "focus suspension stops recorded sample");
            check(f.ledger.plays.size()==1, "focus loss cancels queued sample");
            f.ui.setAudioFocusAllowed(true); Handler.drain();
            check(f.ledger.liveCount()==0, "focus gain cannot revive stale audio");
            f.play(24); check(f.ledger.liveCount()==1, "new event after focus gain works");
        }
    }
    private static void breakwaterRouting() {
        try(Fixture f=new Fixture()) {
            f.play(27); f.start("breakwater-battery");
            SoundPool.completeAllLoads(); Handler.drain();
            check(f.ledger.liveCount()==0, "entering Breakwater stops old Red Ledger sample");
            f.play(1);
            check(SoundPool.totalLive()==1, "real Breakwater bank owns fresh weapon cue");
            check(f.ledger.plays.size()==1, "weapon cue did not reach Red Ledger");
            f.play(22);
            check(f.ledger.plays.size()==1, "Red Ledger cue is excluded in Breakwater session");
            f.start("red-ledger");
            check(SoundPool.totalLive()==0, "leaving Breakwater stops its real bank");
            f.play(27); check(f.ledger.liveCount()==1, "Red Ledger routing recovers");
        }
    }
    private static void boundedStreams() {
        try(Fixture f=new Fixture()) {
            for(int i=0;i<10;i++) f.play(22+i%6);
            check(f.ledger.plays.size()==10, "ten fresh cues reach real bank");
            check(f.ledger.maxStreams==4 && f.ledger.peakConcurrent==4,
                "existing four-voice budget remains bounded");
            check(f.ledger.platformSteals==0, "bank retires oldest tracked stream before replacement");
            check(f.ledger.liveCount()==4, "four latest overlapping streams are retained");
            f.ui.pauseForLifecycle();
            check(f.ledger.liveCount()==0 && f.ledger.stops.size()==10,
                "all ten started streams were explicitly stopped across replacement and pause");
            check(!f.ledger.stops.contains(0), "zero is never treated as stream ID");
        }
    }
    private static void rejectedPlayback() {
        try(Fixture f=new Fixture()) {
            f.play(27); f.play(26);
            f.ledger.rejectPlayback=true; f.play(25);
            f.ledger.rejectPlayback=false; f.ledger.throwPlayback=true; f.play(24);
            f.ledger.throwPlayback=false; f.play(22);
            check(f.ledger.plays.size()==3, "rejected and throwing playback do not create phantom IDs");
            f.play(21); f.play(28);
            check(f.ledger.plays.size()==3, "other banks cannot enter Red Ledger pool");
            f.ui.pauseForLifecycle();
            check(f.ledger.liveCount()==0 && !f.ledger.stops.contains(0),
                "surviving streams stop after platform playback failure");
        }
    }
    private static void pendingDecode() {
        try(Fixture f=new Fixture(false)) {
            f.play(27); check(f.ledger.plays.isEmpty(), "pending decode does not play");
            SoundPool.completeAllLoads();
            f.ui.nativeSoundCue(27); f.ui.pauseForLifecycle(); Handler.drain();
            check(f.ledger.liveCount()==0 && f.ledger.plays.isEmpty(),
                "late decode and queued cue do not revive paused bank");
            f.ui.resumeForLifecycle(); f.ui.setNativeAudioSuspended(false); Handler.drain();
            f.play(27); check(f.ledger.liveCount()==1, "completed sample accepts fresh post-resume cue");
        }
    }
    private static void failedStopRetry() {
        try(Fixture f=new Fixture()) {
            for(int i=0;i<4;i++) f.play(22+i);
            int blocked=f.ledger.plays.get(0).stream;
            f.ledger.throwStops.add(blocked);
            f.ui.setNativeAudioSuspended(true); Handler.drain();
            check(f.ledger.liveCount()==1 && f.ledger.live.containsKey(blocked),
                "a platform stop failure does not prevent stopping other owned streams");
            f.ui.setNativeAudioSuspended(false); Handler.drain(); f.play(27);
            check(f.ledger.plays.size()==4, "failed stop retains ID and cannot be overwritten by new playback");
            f.ledger.throwStops.clear(); f.ui.pauseForLifecycle();
            check(f.ledger.liveCount()==0, "later lifecycle flush retries retained failed stream ID");
        }
        try(Fixture f=new Fixture()) {
            f.play(27); f.ledger.throwStops.add(f.ledger.plays.get(0).stream);
            f.ui.close();
            check(f.ledger.liveCount()==0, "pool release closes an uncooperative stream");
        }
    }
    private static void closeAndStaleDecode() {
        RedLedgerSound bank=new RedLedgerSound(new Context());
        SoundPool pool=SoundPool.instances.get(0);
        SoundPool.completeAllLoads();
        bank.close(); bank.close(); Handler.drain(); bank.playCue(27);
        check(pool.releaseCalls==1 && pool.plays.isEmpty(),
            "idempotent close rejects late decode and later playback");
        RedLedgerSound unavailable=new RedLedgerSound(null);
        unavailable.playCue(27); unavailable.close(); unavailable.close();
    }
    private static void naturalCompletion() {
        try(Fixture f=new Fixture()) {
            f.play(23); Handler.advanceBy(DURATION_MS[1]+1L);
            check(f.ledger.liveCount()==0, "real metal duration expires naturally");
            f.play(27); check(f.ledger.liveCount()==1, "new sample after natural expiry works");
            f.ui.pauseForLifecycle();
            check(f.ledger.liveCount()==0, "stale finished ID cannot prevent stopping later stream");
        }
    }

    private static long waveDurationMillis(Path file) throws IOException {
        byte[] bytes=Files.readAllBytes(file);
        ByteBuffer b=ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
        if(bytes.length<12 || b.getInt(0)!=0x46464952 || b.getInt(8)!=0x45564157)
            throw new IOException("Not RIFF/WAVE: "+file);
        long byteRate=0L, dataBytes=0L;
        for(long at=12;at+8<=bytes.length;) {
            int offset=(int)at, kind=b.getInt(offset);
            long length=Integer.toUnsignedLong(b.getInt(offset+4));
            if(at+8+length>bytes.length) throw new IOException("Truncated WAV: "+file);
            if(kind==0x20746d66 && length>=16) {
                if(b.getShort(offset+8)!=1) throw new IOException("Expected PCM WAV: "+file);
                byteRate=Integer.toUnsignedLong(b.getInt(offset+16));
            } else if(kind==0x61746164) dataBytes+=length;
            at+=8+length+(length&1L);
        }
        if(byteRate<=0 || dataBytes<=0) throw new IOException("Missing WAV metadata: "+file);
        return (dataBytes*1000L+byteRate-1L)/byteRate;
    }
    public static void main(String[] args) throws Exception {
        if(args.length!=1) throw new IllegalArgumentException("Pass actual res/raw directory");
        for(int i=0;i<FILES.length;i++) {
            DURATION_MS[i]=waveDurationMillis(Path.of(args[0],FILES[i]));
            System.out.println("SOURCE: "+FILES[i]+" duration_ms="+DURATION_MS[i]);
        }
        scenario("Activity pause and fresh resume", RedLedgerAudioLifecycleHarness::lifecyclePause);
        scenario("Quick Menu and pointer loss", RedLedgerAudioLifecycleHarness::quickMenuAndPointerLoss);
        scenario("Home and new session", RedLedgerAudioLifecycleHarness::homeAndReplacement);
        scenario("collapsed same-title restart", RedLedgerAudioLifecycleHarness::collapsedRestart);
        scenario("audio focus suspension", RedLedgerAudioLifecycleHarness::focusLoss);
        scenario("unchanged real Breakwater routing", RedLedgerAudioLifecycleHarness::breakwaterRouting);
        scenario("bounded recorded stream ownership", RedLedgerAudioLifecycleHarness::boundedStreams);
        scenario("rejected and throwing playback", RedLedgerAudioLifecycleHarness::rejectedPlayback);
        scenario("asynchronous decode during pause", RedLedgerAudioLifecycleHarness::pendingDecode);
        scenario("bounded retry after platform stop failure", RedLedgerAudioLifecycleHarness::failedStopRetry);
        scenario("idempotent close and stale decode", RedLedgerAudioLifecycleHarness::closeAndStaleDecode);
        scenario("natural sample completion", RedLedgerAudioLifecycleHarness::naturalCompletion);
        System.out.println("Red Ledger real-source lifecycle harness: "+scenarios+" scenarios, "
            +checks+" checks passed; named Android platform double, no device claim.");
    }
}
