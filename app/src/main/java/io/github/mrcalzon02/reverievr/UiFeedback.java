package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.os.Handler;
import android.os.Looper;
import android.view.MotionEvent;
import android.view.SoundEffectConstants;
import android.view.View;
import android.widget.Button;

final class UiFeedback implements AutoCloseable {
    private static final long PRESS_IN_MILLIS = 55L;
    private static final long PRESS_OUT_MILLIS = 90L;
    private static final long FAILURE_FLASH_MILLIS = 360L;
    private static final long FOCUS_SOUND_MIN_INTERVAL_MILLIS = 90L;

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final AudioManager audioManager;
    private final Context audioContext;
    private final ControllerSpringSound controllerSpringSound =
        new ControllerSpringSound();
    private final NativeCombatSound combatSound = new NativeCombatSound();
    private final RedLedgerSound redLedgerSound;
    private BreakwaterAudio breakwaterAudio;
    private volatile boolean lifecycleActive = true;
    private volatile boolean nativeSessionActive;
    private volatile boolean breakwaterSessionActive;
    private volatile boolean nativeAudioSuspended = true;
    private volatile boolean audioFocusAllowed = true;
    private volatile boolean closed;
    private volatile long audioGeneration;
    private long nativeSessionGeneration;
    private long appliedNativeSessionGeneration = -1L;
    private ToneGenerator toneGenerator;
    private long lastFocusSoundMillis;

    UiFeedback(Context context) {
        Context application = context == null ? null : context.getApplicationContext();
        audioContext = application == null ? context : application;
        redLedgerSound = new RedLedgerSound(context);
        audioManager =
            context == null
                ? null
                : (AudioManager) context.getSystemService(
                    Context.AUDIO_SERVICE
                );

        try {
            toneGenerator =
                new ToneGenerator(
                    AudioManager.STREAM_MUSIC,
                    55
                );
        } catch (RuntimeException ignored) {
            toneGenerator = null;
        }
    }

    void bind(Button button, Runnable action) {
        if (button == null || action == null) {
            return;
        }

        button.setSoundEffectsEnabled(false);
        button.setOnTouchListener(
            (view, event) -> {
                animatePressState(view, event);
                return false;
            }
        );
        button.setOnClickListener(
            view -> {
                activation();
                try {
                    action.run();
                } catch (RuntimeException exception) {
                    failure(view);
                    throw exception;
                }
            }
        );
    }

    /** Publish the session boundary before posting any native audio work. */
    synchronized void setNativeModuleSession(String moduleId) {
        if (closed) return;
        nativeSessionActive = moduleId != null && !moduleId.isEmpty();
        breakwaterSessionActive = "breakwater-battery".equals(moduleId);
        audioFocusAllowed = !breakwaterSessionActive;
        // A new guest waits for a fresh validated interaction/listener frame.
        nativeAudioSuspended = true;
        nativeSessionGeneration++;
        audioGeneration++;
        mainHandler.post(this::syncNativeAudioState);
    }

    /** Safe to call each renderer frame: only transitions post to the UI thread. */
    synchronized void setNativeAudioSuspended(boolean suspended) {
        if (closed || !nativeSessionActive || nativeAudioSuspended == suspended)
            return;
        nativeAudioSuspended = suspended;
        audioGeneration++;
        mainHandler.post(this::syncNativeAudioState);
    }

    synchronized void setAudioFocusAllowed(boolean allowed) {
        if (closed || audioFocusAllowed == allowed) return;
        audioFocusAllowed = allowed;
        audioGeneration++;
        mainHandler.post(this::syncNativeAudioState);
    }

    private void syncNativeAudioState() {
        final boolean breakwater;
        final boolean suspended;
        final boolean active;
        final boolean focusAllowed;
        final long sessionGeneration;
        synchronized (this) {
            if (closed) return;
            breakwater = breakwaterSessionActive;
            suspended = nativeAudioSuspended;
            active = lifecycleActive;
            focusAllowed = audioFocusAllowed;
            sessionGeneration = nativeSessionGeneration;
        }
        if (appliedNativeSessionGeneration != sessionGeneration) {
            // Even a stop/start collapsed into one UI turn resets old voices.
            controllerSpringSound.stop();
            combatSound.stop();
            redLedgerSound.stop();
            stopTone();
            if (breakwaterAudio != null) breakwaterAudio.setSessionActive(false);
            appliedNativeSessionGeneration = sessionGeneration;
        } else if (suspended || !active || !focusAllowed) {
            combatSound.stop();
            redLedgerSound.stop();
            stopTone();
        }
        if (breakwater && breakwaterAudio == null)
            breakwaterAudio = new BreakwaterAudio(audioContext);
        if (breakwaterAudio != null) {
            breakwaterAudio.setSessionActive(breakwater);
            breakwaterAudio.setSuspended(suspended || !focusAllowed);
            if (active && focusAllowed) breakwaterAudio.resumeForLifecycle();
            else breakwaterAudio.pauseForLifecycle();
        }
    }

    /** One queue hop; obsolete session/pause callbacks cannot revive voices. */
    private synchronized void postAudio(boolean nativeOnly, Runnable action) {
        if (closed || !lifecycleActive || !audioFocusAllowed
            || (nativeOnly && (!nativeSessionActive || nativeAudioSuspended)))
            return;
        final long generation = audioGeneration;
        mainHandler.post(() -> {
            if (closed || !lifecycleActive || !audioFocusAllowed
                || generation != audioGeneration
                || (nativeOnly && (!nativeSessionActive || nativeAudioSuspended)))
                return;
            action.run();
        });
    }

    private boolean playBreakwaterFeedback(int flags) {
        if (!breakwaterSessionActive) return false;
        // Breakwater owns its normalized control voice budget even in Quick Menu.
        // Unready/recovering recordings stay quiet instead of using louder legacy tones.
        if (breakwaterAudio != null) breakwaterAudio.playShellFeedback(flags);
        return true;
    }

    void focus() {
        postAudio(false, this::playFocusNow);
    }

    private void playFocusNow() {
        long now = android.os.SystemClock.uptimeMillis();
        if (now - lastFocusSoundMillis < FOCUS_SOUND_MIN_INTERVAL_MILLIS) return;
        lastFocusSoundMillis = now;
        if (!playBreakwaterFeedback(NativeModuleRuntime.FEEDBACK_FOCUS))
            playSystemEffect(SoundEffectConstants.NAVIGATION_UP, 0.18f);
    }

    void activation() {
        postAudio(false, this::playActivationNow);
    }

    void springRecenter() {
        postAudio(false, () -> {
            if (!playBreakwaterFeedback(NativeModuleRuntime.FEEDBACK_ACTIVATION))
                controllerSpringSound.play();
        });
    }

    void nativeFeedback(int feedbackFlags) {
        final int flags = feedbackFlags & NativeModuleRuntime.FEEDBACK_ALL;
        if (flags == 0) return;
        postAudio(true, () -> {
            // Preserve the complete native result so failure takes precedence
            // over activation/focus instead of creating overlapping UI tones.
            if (breakwaterSessionActive) {
                if (breakwaterAudio != null) breakwaterAudio.playFeedback(flags);
                return;
            }
            if ((flags & NativeModuleRuntime.FEEDBACK_FAILURE) != 0) {
                playFailureNow();
                return;
            }
            if ((flags & NativeModuleRuntime.FEEDBACK_ACTIVATION) != 0)
                playActivationNow();
            else if ((flags & NativeModuleRuntime.FEEDBACK_FOCUS) != 0)
                playFocusNow();
            if ((flags & NativeModuleRuntime.FEEDBACK_WEAPON_FIRE) != 0)
                combatSound.play(NativeCombatWaveform.FIRE);
            if ((flags & NativeModuleRuntime.FEEDBACK_RELOAD_START) != 0)
                combatSound.play(NativeCombatWaveform.RELOAD);
            if ((flags & NativeModuleRuntime.FEEDBACK_RELOAD_READY) != 0)
                combatSound.play(NativeCombatWaveform.READY);
            if ((flags & NativeModuleRuntime.FEEDBACK_ARTILLERY) != 0)
                combatSound.play(NativeCombatWaveform.ARTILLERY);
            if ((flags & NativeModuleRuntime.FEEDBACK_AIRSTRIKE) != 0)
                combatSound.play(NativeCombatWaveform.AIRSTRIKE);
        });
    }

    void weaponFire() { nativeFeedback(NativeModuleRuntime.FEEDBACK_WEAPON_FIRE); }
    void reloadStart() { nativeFeedback(NativeModuleRuntime.FEEDBACK_RELOAD_START); }
    void reloadReady() { nativeFeedback(NativeModuleRuntime.FEEDBACK_RELOAD_READY); }
    void artilleryStrike() { nativeFeedback(NativeModuleRuntime.FEEDBACK_ARTILLERY); }
    void airstrike() { nativeFeedback(NativeModuleRuntime.FEEDBACK_AIRSTRIKE); }

    void nativeSoundCue(int cueId) {
        if (cueId < 1 || cueId > BreakwaterAudioPolicy.LAST_NATIVE_CUE) return;
        postAudio(true, () -> {
            if (breakwaterSessionActive) {
                if (breakwaterAudio != null) breakwaterAudio.playNativeCue(cueId, 1f, 1f);
            } else if (cueId >= RedLedgerSound.FIRST_CUE && cueId <= RedLedgerSound.LAST_CUE) {
                redLedgerSound.playCue(cueId);
            } else if (cueId <= 21) {
                combatSound.playNativeCue(cueId);
            }
        });
    }

    void nativeSpatialSoundCue(int cueId, float left, float right) {
        if (!BreakwaterAudioPolicy.isSpatialCue(cueId)
            || !Float.isFinite(left) || !Float.isFinite(right)
            || left < 0f || left > 1f || right < 0f || right > 1f) return;
        postAudio(true, () -> {
            if (breakwaterSessionActive) {
                if (breakwaterAudio != null) breakwaterAudio.playNativeCue(cueId, left, right);
            } else if (cueId <= 21) {
                combatSound.playNativeCueSpatial(cueId, left, right);
            }
        });
    }

    synchronized void pauseForLifecycle() {
        lifecycleActive = false;
        nativeAudioSuspended = true;
        audioGeneration++;
        controllerSpringSound.stop();
        combatSound.stop();
        redLedgerSound.stop();
        stopTone();
        if (breakwaterAudio != null) breakwaterAudio.pauseForLifecycle();
    }

    synchronized void resumeForLifecycle() {
        if (closed) return;
        lifecycleActive = true;
        nativeAudioSuspended = true;
        audioGeneration++;
        combatSound.resumeForLifecycle();
        syncNativeAudioState();
    }

    private void stopTone() {
        if (toneGenerator != null) {
            try { toneGenerator.stopTone(); }
            catch (RuntimeException ignored) { }
        }
    }

    private void playActivationNow() {
        if (playBreakwaterFeedback(NativeModuleRuntime.FEEDBACK_ACTIVATION)) return;
        if (toneGenerator != null) {
            try {
                toneGenerator.startTone(
                    ToneGenerator.TONE_PROP_ACK,
                    55
                );
                return;
            } catch (RuntimeException ignored) {
                // Fall back to the platform sound-effect service.
            }
        }

        playSystemEffect(
            SoundEffectConstants.CLICK,
            0.45f
        );
    }

    void failure() {
        failure(null);
    }

    void failure(View source) {
        postAudio(false, () -> {
            playFailureNow();
            if (source != null) {
                source.setActivated(true);
                mainHandler.postDelayed(() -> source.setActivated(false),
                    FAILURE_FLASH_MILLIS);
            }
        });
    }

    private void playFailureNow() {
        if (playBreakwaterFeedback(NativeModuleRuntime.FEEDBACK_FAILURE)) return;
        if (toneGenerator != null) {
            try {
                toneGenerator.startTone(ToneGenerator.TONE_PROP_NACK, 140);
                return;
            } catch (RuntimeException ignored) { }
        }
        playSystemEffect(SoundEffectConstants.NAVIGATION_DOWN, 0.70f);
    }

    private void animatePressState(
        View view,
        MotionEvent event
    ) {
        if (view == null
            || event == null
            || !view.isEnabled()) {
            return;
        }

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                view.animate()
                    .cancel();
                view.animate()
                    .scaleX(0.975f)
                    .scaleY(0.975f)
                    .alpha(0.86f)
                    .setDuration(PRESS_IN_MILLIS)
                    .start();
                break;

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                view.animate()
                    .cancel();
                view.animate()
                    .scaleX(1.0f)
                    .scaleY(1.0f)
                    .alpha(1.0f)
                    .setDuration(PRESS_OUT_MILLIS)
                    .start();
                break;

            default:
                break;
        }
    }

    private void playSystemEffect(
        int effect,
        float volume
    ) {
        if (audioManager == null) {
            return;
        }

        try {
            audioManager.playSoundEffect(
                effect,
                volume
            );
        } catch (RuntimeException ignored) {
            // Feedback failure must never break the requested action.
        }
    }

    @Override
    public synchronized void close() {
        if (closed) return;
        closed = true;
        nativeSessionActive = false;
        breakwaterSessionActive = false;
        nativeAudioSuspended = true;
        audioGeneration++;
        mainHandler.removeCallbacksAndMessages(null);
        controllerSpringSound.close();
        combatSound.close();
        redLedgerSound.close();
        if (breakwaterAudio != null) breakwaterAudio.close();
        if (toneGenerator != null) {
            toneGenerator.release();
            toneGenerator = null;
        }
    }
}
