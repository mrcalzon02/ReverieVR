package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.media.ToneGenerator;
import android.os.Handler;
import android.os.SystemClock;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** Executes the actual bank, playback policy, music policy, and UiFeedback.
 * Fakes cover Android's edges only; all bank decisions remain production code. */
public final class BreakwaterAudioBackendHarness {
    private static int checks, scenarios;
    private static final String ROOT = BreakwaterAudioPolicy.ASSET_ROOT;
    private static final int CONTROL = BreakwaterAudioPolicy.CONTROL_VOICES;
    private static final int EFFECT = BreakwaterAudioPolicy.EFFECT_VOICES;
    private static final int LOOP = BreakwaterAudioPolicy.LOOP_VOICES;

    private static final class Fixture implements AutoCloseable {
        final Context context = new Context();
        final BreakwaterAudio bank = new BreakwaterAudio(context);
        void start(boolean ready) {
            bank.setSessionActive(true);
            bank.resumeForLifecycle();
            if (ready) { SoundPool.completeAllLoads(); Handler.drain(); }
        }
        @Override public void close() { bank.close(); Handler.drain(); }
    }

    private static void check(boolean value, String explanation) {
        checks++;
        if (!value) throw new AssertionError(explanation);
    }
    private static void near(double actual, double expected, String explanation) {
        check(Math.abs(actual - expected) < .000001, explanation + ": " + actual + " != " + expected);
    }
    private static void scenario(String name, Runnable test) {
        Handler.reset(); SoundPool.reset(); ReverieLog.messages.clear();
        NativeCombatSound.played.clear(); RedLedgerSound.played.clear(); ToneGenerator.tones.clear();
        try {
            test.run();
            check(SoundPool.totalLive() == 0, name + " leaked a playing voice");
            for (SoundPool pool : SoundPool.instances)
                check(pool.released && pool.releaseCalls == 1, name + " leaked or twice released a pool");
            scenarios++;
        } catch (Throwable failure) {
            throw new AssertionError("Scenario: " + name, failure);
        }
    }
    private static int loadedFiles(SoundPool pool, String file) {
        int result = 0;
        for (SoundPool.Sample sample : pool.loads) if (sample.file.equals(ROOT + file)) result++;
        return result;
    }
    private static void complete(SoundPool pool, String file, int status) {
        pool.complete(pool.lastSample(file), status); Handler.drain();
    }
    private static void refreshAllLoops(BreakwaterAudio bank) {
        for (int id = BreakwaterAudioPolicy.FIRST_LOOP; id <= BreakwaterAudioPolicy.LAST_LOOP; id++)
            check(bank.playNativeCue(id, 1f, 1f), "loop lease accepted: " + id);
        Handler.drain(); Handler.advanceBy(200L);
    }

    private static void asyncPreparation() {
        try (Fixture f = new Fixture()) {
            check(SoundPool.instances.isEmpty(), "construction is lazy");
            f.bank.setSessionActive(true);
            check(SoundPool.instances.isEmpty(), "inactive lifecycle does not create pools");
            check(!f.bank.playNativeCue(1, 1f, 1f), "inactive lifecycle rejects playback");
            f.bank.resumeForLifecycle();
            check(SoundPool.instances.size() == 3, "three independently bounded categories");
            for (SoundPool pool : SoundPool.instances)
                check(pool.attributes.usage == AudioAttributes.USAGE_GAME, "game usage applied");
            SoundPool effects = SoundPool.current(EFFECT), controls = SoundPool.current(CONTROL);
            check(loadedFiles(controls, "ui_activate.wav") == 1
                    && loadedFiles(controls, "menu_open.wav") == 1, "shared clips load once per pool");
            check(!f.bank.playNativeCue(1, .5f, .25f), "pending decode reports unavailable");
            check(SoundPool.totalPlays() == 0, "pending decode cannot play");
            complete(effects, "weapon_machine.wav", 0);
            check(f.bank.playNativeCue(1, .5f, .25f), "only completed sample becomes playable");
            SoundPool.Playback shot = effects.playingFile("weapon_machine.wav");
            check(shot != null && shot.loop == 0 && shot.priority == 3, "one-shot mode and priority retained");
            near(shot.left, .11, "left spatial gain"); near(shot.right, .055, "right spatial gain");
            check(!f.bank.playNativeCue(2, 1f, 1f), "other pending sample stays unavailable");
            check(!f.bank.playNativeCue(22, 1f, 1f), "Red Ledger cue not claimed by Breakwater");
            check(!f.bank.playNativeCue(48, 1f, 1f), "internal feedback cue not accepted over native ABI");
            check(!f.bank.playNativeCue(1, Float.NaN, 1f)
                    && !f.bank.playNativeCue(1, 1.01f, 1f), "non-finite or out-of-range gains rejected");
        }
    }

    private static void feedbackAtomicity() {
        try (Fixture f = new Fixture()) {
            f.start(false);
            SoundPool controls = SoundPool.current(CONTROL);
            complete(controls, "ui_activate.wav", 0);
            check(!f.bank.playFeedback(2 | 8), "partially loaded compound request reports unavailable");
            check(SoundPool.totalPlays() == 0, "compound preflight never plays a loaded half");
            SoundPool.completeAllLoads(); Handler.drain();
            check(f.bank.playFeedback(1 | 2 | 4), "combined control result handled");
            check(controls.plays.size() == 1 && controls.plays.get(0).file.endsWith("/ui_failure.wav"),
                "failure has one sound and wins focus/activation");
            Handler.advanceBy(200L);
            int before = SoundPool.totalPlays();
            check(f.bank.playFeedback(2 | 8), "fully loaded compound request handled");
            check(SoundPool.totalPlays() == before + 2, "activation and weapon play exactly once each");
        }
    }

    private static void loopLeasesAndFades() {
        try (Fixture f = new Fixture()) {
            f.start(true);
            SoundPool loops = SoundPool.current(LOOP);
            check(f.bank.playNativeCue(41, .5f, .25f), "initial surf lease handled");
            Handler.drain(); Handler.advanceBy(100L);
            SoundPool.Playback surf = loops.playingFile("surf_loop.wav");
            check(surf != null && surf.loop == -1 && surf.rate == 1f, "environment uses a stable loop stream");
            near(surf.left, .06, "surf left ceiling includes spatial gain");
            near(surf.right, .03, "surf right ceiling includes spatial gain");
            Handler.advanceBy(300L);
            long refreshed = SystemClock.uptimeMillis();
            f.bank.playNativeCue(41, .2f, 1f);
            Handler.advanceBy(150L);
            check(loops.plays.size() == 1, "heartbeat changes gain without retriggering sample");
            near(surf.left, .024, "updated left loop gain"); near(surf.right, .12, "updated right loop gain");
            Handler.advanceTo(refreshed + BreakwaterAudioPolicy.LOOP_LEASE_MS - 1L);
            check(loops.liveCount() == 1, "renewed lease stays alive until expiry");
            Handler.advanceTo(refreshed + BreakwaterAudioPolicy.LOOP_LEASE_MS + 200L);
            check(loops.liveCount() == 0 && loops.stops.contains(surf.stream), "expired lease fades and stops");
            check(Handler.pendingCount() == 0, "silent bank stops mix timer");
            f.bank.playNativeCue(41, 1f, 1f); Handler.drain(); Handler.advanceBy(150L);
            f.bank.playNativeCue(41, 0f, 0f); Handler.advanceBy(200L);
            check(loops.liveCount() == 0, "zero-gain heartbeat relinquishes voice");
        }
    }

    private static void loopsWaitingForDecode() {
        try (Fixture f = new Fixture()) {
            f.start(false);
            SoundPool loops = SoundPool.current(LOOP);
            f.bank.playNativeCue(43, .5f, 1f); Handler.drain(); Handler.advanceBy(100L);
            check(loops.plays.isEmpty(), "lease can wait for an asynchronous decode");
            complete(loops, "night_loop.wav", 0); Handler.advanceBy(50L);
            check(loops.liveCount() == 1, "live lease starts when its sample becomes ready");
            f.bank.playNativeCue(44, 1f, 1f); Handler.advanceBy(1_200L);
            complete(loops, "boat_loop.wav", 0); Handler.advanceBy(100L);
            check(loops.playingFile("boat_loop.wav") == null, "late decode cannot revive expired lease");
            check(loops.liveCount() == 0 && Handler.pendingCount() == 0, "expired pending loops leave no work");
        }
    }

    private static void prioritiesAndHeadroom() {
        try (Fixture f = new Fixture()) {
            f.start(true); refreshAllLoops(f.bank);
            SoundPool controls = SoundPool.current(CONTROL), effects = SoundPool.current(EFFECT);
            SoundPool loops = SoundPool.current(LOOP);
            Set<Integer> loopStreams = new HashSet<>(loops.live.keySet());
            f.bank.playFeedback(1); f.bank.playFeedback(2);
            for (int id = 1; id <= 4; id++) f.bank.playNativeCue(id, 1f, 1f);
            check(controls.liveCount() == 2 && effects.liveCount() == 4 && loops.liveCount() == 7,
                "full traffic obeys separate 2/4/7 voice budgets");
            int before = effects.plays.size();
            check(f.bank.playNativeCue(6, 1f, 1f), "low priority overload is quietly owned");
            check(effects.plays.size() == before, "impact cannot displace higher priority player weapons");
            f.bank.playNativeCue(5, 1f, 1f);
            check(effects.liveCount() == 4 && effects.plays.size() == before + 1,
                "equal priority new weapon replaces one voice within budget");
            f.bank.playFeedback(4);
            check(controls.playingFile("ui_failure.wav") != null
                    && controls.playingFile("ui_focus.wav") == null,
                "failure preempts quiet focus while retaining activation");
            check(loops.live.keySet().equals(loopStreams), "combat/control pressure never steals an ambient stream");
            check(effects.platformSteals == 0 && controls.platformSteals == 0 && loops.platformSteals == 0,
                "bank manages voice replacement before platform exhaustion");
            check(controls.peakConcurrent <= 2 && effects.peakConcurrent <= 4 && loops.peakConcurrent <= 7,
                "peak physical concurrency respects budgets");

            // Source PCM ceilings are independently enforced by the asset verifier.
            // This check uses actual streams/gains emitted by the production bank.
            double emittedPeak = 0.;
            for (SoundPool pool : SoundPool.instances) {
                double pcmPeak = Math.pow(10., (pool == controls ? -12. : pool == effects ? -9. : -18.) / 20.);
                for (SoundPool.Playback playing : pool.live.values())
                    emittedPeak += Math.max(playing.left, playing.right) * pcmPeak;
            }
            near(emittedPeak, BreakwaterAudioPolicy.maximumEffectsPeak(), "observed worst-case mix matches peak budget");
            double music = MusicVolumePolicy.gain(100, false);
            near(music, .50, "independent soundtrack maximum");
            check(emittedPeak < .50 && emittedPeak + music < 1.,
                "all thirteen voices plus maximum music retain digital peak headroom");
        }
    }

    private static void lifecycleStops() {
        try (Fixture f = new Fixture()) {
            f.start(true);
            int opens = f.context.getAssets().totalOpens();
            f.bank.playNativeCue(41, 1f, 1f); f.bank.playNativeCue(1, 1f, 1f); f.bank.playFeedback(2);
            Handler.drain(); check(SoundPool.totalLive() == 3, "fixture has ambient, combat and control voices");
            f.bank.pauseForLifecycle();
            check(SoundPool.totalLive() == 0 && Handler.pendingCount() == 0, "background stops every voice and timer");
            check(!f.bank.playNativeCue(1, 1f, 1f) && !f.bank.playFeedback(2), "background rejects new playback");
            Handler.advanceBy(2_000L); f.bank.resumeForLifecycle(); Handler.drain();
            check(SoundPool.totalLive() == 0, "resume never resurrects gunshots or stale environmental leases");
            check(f.context.getAssets().totalOpens() == opens, "healthy resume reuses decoded clips");
            f.bank.playNativeCue(41, 1f, 1f); Handler.drain();
            check(SoundPool.totalLive() == 1, "fresh heartbeat resumes environment");
            f.bank.setSuspended(true);
            check(SoundPool.totalLive() == 0 && Handler.pendingCount() == 0, "Quick Menu stops all loop state");
            f.bank.setSuspended(false); Handler.drain();
            check(SoundPool.totalLive() == 0, "unsuspend requires fresh native event");
            check(f.bank.playNativeCue(1, 1f, 1f), "fresh event can play after unsuspend");
            f.bank.close(); f.bank.close();
            check(SoundPool.totalLive() == 0, "close is immediate and idempotent");
        }
    }

    private static void staleLoadCallbacks() {
        try (Fixture f = new Fixture()) {
            f.start(false);
            List<SoundPool> old = new ArrayList<>(SoundPool.instances);
            for (SoundPool pool : old) pool.completeAll(0);
            f.bank.setSessionActive(false); f.bank.setSessionActive(true);
            Handler.drain();
            check(SoundPool.instances.size() == 6, "restart creates an isolated pool generation");
            check(!f.bank.playNativeCue(1, 1f, 1f), "old decode callback cannot mark replacement sample ready");
            SoundPool effects = SoundPool.current(EFFECT);
            complete(effects, "weapon_machine.wav", 0);
            check(f.bank.playNativeCue(1, 1f, 1f), "new generation's own completion enables playback");
            int played = SoundPool.totalPlays();
            SoundPool.completeAllLoads(); f.bank.close(); Handler.drain(); Handler.advanceBy(1_000L);
            for (SoundPool pool : old) pool.complete(pool.loads.get(0), 0);
            Handler.drain();
            check(SoundPool.totalPlays() == played && SoundPool.totalLive() == 0,
                "callbacks queued before/after close cannot recreate voices");
            check(!f.bank.playFeedback(2), "closed bank refuses new feedback");
        }
    }

    private static void missingAssetRetry() {
        try (Fixture f = new Fixture()) {
            AssetManager assets = f.context.getAssets();
            String path = ROOT + "weapon_machine.wav";
            assets.missing.add(path); f.start(true);
            check(assets.opensFor(path) == 1, "missing asset attempted initially");
            for (int i = 0; i < 20; i++) {
                check(!f.bank.playNativeCue(1, 1f, 1f), "missing critical cue reports unavailable");
                f.bank.resumeForLifecycle();
            }
            check(assets.opensFor(path) == 1,
                "repeated active lifecycle sync does not bypass three-second failed-load backoff");
            Handler.advanceBy(2_999L); f.bank.playNativeCue(1, 1f, 1f);
            check(assets.opensFor(path) == 1, "failed asset is not retried early");
            Handler.advanceBy(1L); f.bank.playNativeCue(1, 1f, 1f);
            check(assets.opensFor(path) == 2, "failed asset receives one bounded retry");
            assets.missing.remove(path); Handler.advanceBy(3_000L);
            check(!f.bank.playNativeCue(1, 1f, 1f), "repaired asset still waits for async decode");
            complete(SoundPool.current(EFFECT), "weapon_machine.wav", 0);
            check(f.bank.playNativeCue(1, 1f, 1f), "asset recovers after successful retry");
            check(SoundPool.instances.size() == 3, "sample retries reuse existing pools");
            check(loadedFiles(SoundPool.current(EFFECT), "weapon_heavy.wav") == 1,
                "healthy clips are not reloaded during retry");
        }
    }

    private static void decoderRetry() {
        try (Fixture f = new Fixture()) {
            f.start(false);
            SoundPool effects = SoundPool.current(EFFECT);
            SoundPool.Sample failed = effects.lastSample("weapon_heavy.wav");
            effects.complete(failed, 1); SoundPool.completeAllLoads(); Handler.drain();
            check(effects.unloads.contains(failed.id), "decoder failure unloads rejected sample");
            check(!f.bank.playNativeCue(2, 1f, 1f), "failed decode reports unavailable");
            Handler.advanceBy(2_999L); f.bank.playNativeCue(2, 1f, 1f);
            check(loadedFiles(effects, "weapon_heavy.wav") == 1, "decoder retries are delayed");
            Handler.advanceBy(1L); f.bank.playNativeCue(2, 1f, 1f);
            check(loadedFiles(effects, "weapon_heavy.wav") == 2, "decoder receives one new sample id after delay");
            complete(effects, "weapon_heavy.wav", 1);
            Handler.advanceBy(3_000L); f.bank.playNativeCue(2, 1f, 1f);
            complete(effects, "weapon_heavy.wav", 0);
            check(f.bank.playNativeCue(2, 1f, 1f), "later successful decode recovers without a new session");
        }
    }

    private static void newSessionResetsBackoff() {
        try (Fixture f = new Fixture()) {
            AssetManager assets = f.context.getAssets();
            String path = ROOT + "weapon_machine.wav";
            assets.missing.add(path); f.start(false);
            check(assets.opensFor(path) == 1, "first session records failed load");
            f.bank.setSessionActive(false);
            assets.missing.clear();
            f.bank.setSessionActive(true);
            check(assets.opensFor(path) == 2, "new session can prepare immediately after old session failure");
            SoundPool.completeAllLoads(); Handler.drain();
            check(f.bank.playNativeCue(1, 1f, 1f), "new session does not inherit prior failure backoff");
        }
    }

    private static void platformPreparationFailures() {
        try (Fixture f = new Fixture()) {
            SoundPool.rejectBuildLimits.add(CONTROL);
            SoundPool.rejectLoadFiles.add(ROOT + "weapon_heavy.wav");
            f.context.getAssets().lengths.put(ROOT + "weapon_machine.wav", 44L);
            f.start(true);
            check(SoundPool.instances.size() == 2 && SoundPool.buildAttempts == 3
                    && SoundPool.current(LOOP) != null,
                "control/build and combat/load failures do not prevent environment group");
            check(!f.bank.playNativeCue(28, 1f, 1f) && !f.bank.playNativeCue(1, 1f, 1f)
                    && !f.bank.playNativeCue(2, 1f, 1f), "pool failure, invalid WAV size and load rejection are bounded failures");
            Handler.advanceBy(2_999L); f.bank.playNativeCue(28, 1f, 1f);
            check(SoundPool.buildAttempts == 3, "pool creation is not retried per cue");
            SoundPool.rejectBuildLimits.clear(); SoundPool.rejectLoadFiles.clear(); f.context.getAssets().lengths.clear();
            Handler.advanceBy(1L); f.bank.playNativeCue(28, 1f, 1f);
            SoundPool.completeAllLoads(); Handler.drain();
            check(SoundPool.buildAttempts == 5 && SoundPool.instances.size() == 4,
                "only missing/failed groups are rebuilt after the retry window");
            check(f.bank.playNativeCue(28, 1f, 1f) && f.bank.playNativeCue(1, 1f, 1f)
                    && f.bank.playNativeCue(2, 1f, 1f), "all preparation fault types recover");
        }
    }

    private static void rejectedPlaybackRetry() {
        try (Fixture f = new Fixture()) {
            f.start(true);
            SoundPool loops = SoundPool.current(LOOP), effects = SoundPool.current(EFFECT);
            f.bank.playFeedback(2);
            loops.rejectPlayback = true;
            f.bank.playNativeCue(41, 1f, 1f); Handler.drain();
            check(loops.playAttempts == 1 && loops.released, "initial loop rejection discards failing pool");
            check(SoundPool.current(CONTROL).liveCount() == 1, "loop recovery does not stop unrelated controls");
            int builds = SoundPool.buildAttempts;
            for (int i = 0; i < 29; i++) {
                Handler.advanceBy(100L); f.bank.playNativeCue(41, 1f, 1f); Handler.drain();
                f.bank.resumeForLifecycle();
            }
            check(SoundPool.buildAttempts == builds, "heartbeats/state sync do not rebuild pool before retry deadline");
            Handler.advanceBy(100L); f.bank.playNativeCue(41, 1f, 1f); Handler.drain();
            check(SoundPool.buildAttempts == builds + 1, "one replacement pool is created after three seconds");
            SoundPool replacement = SoundPool.current(LOOP);
            check(replacement != loops && replacement.liveCount() == 0, "replacement waits for its own decode");
            loops.complete(loops.lastSample("surf_loop.wav"), 0); Handler.drain();
            check(replacement.plays.isEmpty(), "old pool completion cannot mark replacement samples ready");
            SoundPool.completeAllLoads(); Handler.drain(); Handler.advanceBy(50L);
            check(replacement.liveCount() == 1, "fresh active lease can recover after replacement decode");

            effects.rejectPlayback = true;
            check(!f.bank.playNativeCue(1, 1f, 1f) && effects.released,
                "one-shot rejection reports failure and invalidates its pool");
            int attempts = effects.playAttempts;
            check(!f.bank.playNativeCue(1, 1f, 1f) && effects.playAttempts == attempts,
                "failed event is not disguised as cooldown suppression or replayed on a dead pool");
            Handler.advanceBy(3_000L); f.bank.playNativeCue(1, 1f, 1f);
            SoundPool.completeAllLoads(); Handler.drain();
            SoundPool repaired = SoundPool.current(EFFECT);
            check(repaired != effects && f.bank.playNativeCue(1, 1f, 1f), "new pool restores one-shot playback");
            Handler.advanceBy(60L); repaired.throwPlayback = true;
            check(!f.bank.playNativeCue(1, 1f, 1f) && repaired.released,
                "runtime playback exception is contained by bounded pool recovery");
        }
    }

    private static void expiredRecoveryAndPartialFeedback() {
        try (Fixture f = new Fixture()) {
            f.start(true);
            SoundPool effects = SoundPool.current(EFFECT), loops = SoundPool.current(LOOP);
            effects.rejectPlayback = true;
            check(!f.bank.playFeedback(2 | 8), "partial runtime compound failure is reported");
            SoundPool controls = SoundPool.current(CONTROL);
            check(controls.plays.size() == 1 && controls.playingFile("ui_activate.wav") != null,
                "successful compound component plays once");
            check(!f.bank.playFeedback(2 | 8) && controls.plays.size() == 1,
                "retry preflight cannot replay the already successful component");
            loops.rejectPlayback = true;
            f.bank.playNativeCue(41, 1f, 1f); Handler.drain();
            Handler.advanceBy(3_000L); f.bank.resumeForLifecycle();
            SoundPool.completeAllLoads(); Handler.drain(); Handler.advanceBy(150L);
            check(SoundPool.current(LOOP).liveCount() == 0,
                "loop pool recovery cannot resurrect a lease from the failed pool");
            check(SoundPool.current(EFFECT).plays.isEmpty(), "recovery never replays failed one-shot automatically");
            f.bank.playNativeCue(41, 1f, 1f); Handler.drain();
            check(SoundPool.current(LOOP).liveCount() == 1, "only fresh loop heartbeat restores environment");
        }
    }

    private static void shellControlsAndDeadlines() {
        BreakwaterAudioPolicy p = new BreakwaterAudioPolicy();
        p.setSessionActive(true); p.setLifecycleActive(true);
        check(p.canPlayOneShot(1, 10_000L, false) && p.canPlayOneShot(1, 10_000L, false),
            "checking playback eligibility does not commit a deadline");
        p.recordOneShot(1, 10_000L);
        check(!p.canPlayOneShot(1, 10_010L, false), "successful/intentional suppression records deadline");
        try (Fixture f = new Fixture()) {
            f.start(true); f.bank.setSuspended(true);
            check(!f.bank.playFeedback(2) && !f.bank.playNativeCue(1, 1f, 1f),
                "game playback stays suspended in Quick Menu");
            check(f.bank.playShellFeedback(1 | 2 | 4), "Quick Menu control failure uses shell-only entry point");
            SoundPool controls = SoundPool.current(CONTROL);
            check(controls.plays.size() == 1 && controls.plays.get(0).file.endsWith("/ui_failure.wav"),
                "menu feedback keeps failure precedence and normalized category");
            near(controls.plays.get(0).left, .20, "menu feedback gain remains bounded");
            check(f.bank.playShellFeedback(4) && controls.plays.size() == 1,
                "same-frame shell control repeat consumes no extra voice");
            f.bank.pauseForLifecycle();
            check(!f.bank.playShellFeedback(2) && SoundPool.totalLive() == 0,
                "lifecycle/focus gate stops shell-only control playback too");
        }
    }

    private static void actualUiBankIntegration() {
        Context context = new Context();
        try (UiFeedback ui = new UiFeedback(context)) {
            ui.setNativeModuleSession("breakwater-battery");
            ui.setAudioFocusAllowed(true); ui.setNativeAudioSuspended(false); Handler.drain();
            ui.nativeSoundCue(1); Handler.drain();
            check(NativeCombatSound.played.isEmpty() && ToneGenerator.tones.isEmpty(), "initial unavailable Breakwater cue never leaks legacy sound");
            SoundPool.completeAllLoads(); Handler.drain();
            ui.nativeSoundCue(1); ui.nativeSpatialSoundCue(41, 1f, 1f); Handler.drain(); Handler.advanceBy(100L);
            check(SoundPool.totalLive() == 2, "actual UI reaches actual recorded weapon and environment");
            int played = SoundPool.totalPlays();
            ui.nativeSoundCue(2); ui.setNativeAudioSuspended(true); Handler.drain();
            check(SoundPool.totalPlays() == played && SoundPool.totalLive() == 0,
                "Quick Menu cancels queued shot and stops actual pools");
            ui.nativeSoundCue(2); ui.nativeFeedback(8); ui.activation(); Handler.drain();
            check(NativeCombatSound.played.isEmpty() && ToneGenerator.tones.isEmpty()
                    && SoundPool.current(CONTROL).playingFile("ui_activate.wav") != null,
                "suspended native traffic is silent while shell activation uses normalized controls");
            ui.setNativeAudioSuspended(false); Handler.drain();
            check(SoundPool.current(LOOP).liveCount() == 0 && SoundPool.current(EFFECT).liveCount() == 0,
                "fresh pointer alone does not revive old game voices");
            ui.nativeSoundCue(3); ui.setAudioFocusAllowed(false); Handler.drain();
            check(SoundPool.totalPlays() == played + 1 && NativeCombatSound.played.isEmpty()
                    && SoundPool.totalLive() == 0,
                "focus loss cancels queued game event and stops menu control voices");
            ui.setAudioFocusAllowed(true); Handler.drain(); ui.nativeSoundCue(4); Handler.drain();
            check(SoundPool.totalLive() == 1, "new cue works after focus gain");
            ui.nativeSoundCue(5); ui.pauseForLifecycle(); ui.resumeForLifecycle(); Handler.drain();
            check(SoundPool.totalLive() == 0, "background/resume stops bank and discards old event");
            ui.setNativeAudioSuspended(false); Handler.drain(); ui.nativeSoundCue(5);
            ui.setNativeModuleSession(null); Handler.drain();
            check(SoundPool.totalLive() == 0 && SoundPool.instances.stream().allMatch(pool -> pool.released),
                "session exit releases actual pools and cancels pending cue");
        }
        Handler.drain();
    }

    private static void focusLossWhileMenuSuspended() {
        try (UiFeedback ui = new UiFeedback(new Context())) {
            ui.setNativeModuleSession("breakwater-battery");
            ui.setAudioFocusAllowed(true); ui.setNativeAudioSuspended(false); Handler.drain();
            SoundPool.completeAllLoads(); Handler.drain();
            ui.setNativeAudioSuspended(true); Handler.drain();
            ui.activation(); Handler.drain();
            check(SoundPool.current(CONTROL).liveCount() == 1, "menu owns one normalized control voice");
            ui.setAudioFocusAllowed(false); Handler.drain();
            check(SoundPool.totalLive() == 0, "focus loss stops shell voice even when game was already suspended");
            int played = SoundPool.totalPlays();
            ui.activation(); ui.failure(); Handler.drain();
            check(SoundPool.totalPlays() == played && ToneGenerator.tones.isEmpty(),
                "no normalized or legacy control starts without focus");
            ui.setAudioFocusAllowed(true); Handler.drain(); ui.activation(); Handler.drain();
            check(SoundPool.current(CONTROL).liveCount() == 1, "focus gain restores menu controls without gameplay lease");
        }
        Handler.drain();
    }

    public static void main(String[] args) {
        scenario("asynchronous preparation and cue ownership", BreakwaterAudioBackendHarness::asyncPreparation);
        scenario("atomic feedback and failure precedence", BreakwaterAudioBackendHarness::feedbackAtomicity);
        scenario("environment leases and spatial fades", BreakwaterAudioBackendHarness::loopLeasesAndFades);
        scenario("asynchronous loop readiness", BreakwaterAudioBackendHarness::loopsWaitingForDecode);
        scenario("voice priority, pool isolation and music headroom", BreakwaterAudioBackendHarness::prioritiesAndHeadroom);
        scenario("lifecycle, pause and close", BreakwaterAudioBackendHarness::lifecycleStops);
        scenario("stale decode callbacks after restart and close", BreakwaterAudioBackendHarness::staleLoadCallbacks);
        scenario("missing asset retry and repeated lifecycle sync", BreakwaterAudioBackendHarness::missingAssetRetry);
        scenario("fresh session resets prior load backoff", BreakwaterAudioBackendHarness::newSessionResetsBackoff);
        scenario("decoder rejection and bounded recovery", BreakwaterAudioBackendHarness::decoderRetry);
        scenario("pool, asset and platform load failures", BreakwaterAudioBackendHarness::platformPreparationFailures);
        scenario("bounded playback rejection and exception recovery", BreakwaterAudioBackendHarness::rejectedPlaybackRetry);
        scenario("expired pool recovery and partial compound feedback", BreakwaterAudioBackendHarness::expiredRecoveryAndPartialFeedback);
        scenario("normalized shell controls and deadline commits", BreakwaterAudioBackendHarness::shellControlsAndDeadlines);
        scenario("focus loss while Quick Menu is already suspended", BreakwaterAudioBackendHarness::focusLossWhileMenuSuspended);
        scenario("actual UiFeedback and bank lifecycle integration", BreakwaterAudioBackendHarness::actualUiBankIntegration);
        System.out.println("Breakwater actual backend harness: " + scenarios + " scenarios, " + checks + " checks passed");
    }
}
