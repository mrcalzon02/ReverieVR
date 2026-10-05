package io.github.mrcalzon02.reverievr.controller;

import android.content.Context;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.os.Handler;
import android.os.Looper;
import android.view.MotionEvent;
import android.view.SoundEffectConstants;
import android.view.View;
import android.widget.Button;

final class ControllerUiFeedback implements AutoCloseable {
    private static final long PRESS_IN_MILLIS = 55L;
    private static final long PRESS_OUT_MILLIS = 90L;
    private static final long FAILURE_FLASH_MILLIS = 360L;

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final AudioManager audioManager;
    private ToneGenerator toneGenerator;

    ControllerUiFeedback(Context context) {
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

    void prepareMomentary(Button button) {
        if (button != null) {
            button.setSoundEffectsEnabled(false);
        }
    }

    void animatePressState(View view, MotionEvent event) {
        if (view == null
            || event == null
            || !view.isEnabled()) {
            return;
        }

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                view.animate().cancel();
                view.animate()
                    .scaleX(0.975f)
                    .scaleY(0.975f)
                    .alpha(0.86f)
                    .setDuration(PRESS_IN_MILLIS)
                    .start();
                break;

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                view.animate().cancel();
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

    void activation() {
        if (toneGenerator != null) {
            try {
                toneGenerator.startTone(
                    ToneGenerator.TONE_PROP_ACK,
                    55
                );
                return;
            } catch (RuntimeException ignored) {
                // Fall back to platform UI feedback.
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
        mainHandler.post(
            () -> {
                if (toneGenerator != null) {
                    try {
                        toneGenerator.startTone(
                            ToneGenerator.TONE_PROP_NACK,
                            140
                        );
                    } catch (RuntimeException ignored) {
                        playSystemEffect(
                            SoundEffectConstants.NAVIGATION_DOWN,
                            0.70f
                        );
                    }
                } else {
                    playSystemEffect(
                        SoundEffectConstants.NAVIGATION_DOWN,
                        0.70f
                    );
                }

                if (source != null) {
                    source.setActivated(true);
                    mainHandler.postDelayed(
                        () -> source.setActivated(false),
                        FAILURE_FLASH_MILLIS
                    );
                }
            }
        );
    }

    private void playSystemEffect(
        int effect,
        float volume
    ) {
        if (audioManager == null) {
            return;
        }

        try {
            audioManager.playSoundEffect(effect, volume);
        } catch (RuntimeException ignored) {
            // Feedback failure cannot break controller operation.
        }
    }

    @Override
    public void close() {
        mainHandler.removeCallbacksAndMessages(null);
        if (toneGenerator != null) {
            toneGenerator.release();
            toneGenerator = null;
        }
    }
}
