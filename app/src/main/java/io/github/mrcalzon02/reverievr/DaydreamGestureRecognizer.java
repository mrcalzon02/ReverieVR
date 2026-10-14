package io.github.mrcalzon02.reverievr;

/** Normalizes physical Daydream clicks and acceleration into optional gestures.
 * Existing Select click/Menu/Home events are owned by VrInputRouter and remain
 * immediate: this class adds distinct guest-binding events, never replaces them.
 */
final class DaydreamGestureRecognizer {
    interface Sink { void onGesture(BindingInput input, boolean down); }

    static final long LONG_PRESS_NANOS = 650000000L;
    static final long DOUBLE_TAP_NANOS = 350000000L;
    static final long MAX_TAP_NANOS = 300000000L;
    static final long SHAKE_COOLDOWN_NANOS = 1100000000L;
    private boolean pressed;
    private boolean held;
    private long pressedSince;
    private long pendingTap;
    private long lastShakeNanos;
    private boolean shakeArmed = true;

    void accept(long now, boolean click, float ax, float ay, float az,
                Sink sink) {
        if (sink == null || now <= 0L) return;
        if (pendingTap > 0L && now - pendingTap >= DOUBLE_TAP_NANOS) {
            sink.onGesture(BindingInput.DAYDREAM_TAP, true);
            sink.onGesture(BindingInput.DAYDREAM_TAP, false);
            pendingTap = 0L;
        }
        if (click && !pressed) {
            pressedSince = now;
            held = false;
        }
        if (click && pressed && !held
                && pressedSince > 0L
                && now - pressedSince >= LONG_PRESS_NANOS) {
            pendingTap = 0L;
            held = true;
            sink.onGesture(BindingInput.DAYDREAM_PRESS_HOLD, true);
        }
        if (!click && pressed) {
            if (held) {
                sink.onGesture(BindingInput.DAYDREAM_PRESS_HOLD, false);
            } else if (pressedSince > 0L
                    && now - pressedSince <= MAX_TAP_NANOS) {
                if (pendingTap > 0L
                        && now - pendingTap < DOUBLE_TAP_NANOS) {
                    sink.onGesture(BindingInput.DAYDREAM_DOUBLE_TAP, true);
                    sink.onGesture(BindingInput.DAYDREAM_DOUBLE_TAP, false);
                    pendingTap = 0L;
                } else {
                    pendingTap = now;
                }
            }
            pressedSince = 0L;
            held = false;
        }
        pressed = click;

        // Raw packets report acceleration including gravity. A 2.5g
        // magnitude requires a deliberate impulse; release under 1.5g.
        if (Float.isFinite(ax) && Float.isFinite(ay) && Float.isFinite(az)) {
            float magnitudeSquared = ax * ax + ay * ay + az * az;
            if (magnitudeSquared < 2.25f) shakeArmed = true;
            if (shakeArmed && magnitudeSquared > 6.25f
                && (lastShakeNanos == 0L
                    || now - lastShakeNanos > SHAKE_COOLDOWN_NANOS)) {
                shakeArmed = false;
                lastShakeNanos = now;
                sink.onGesture(BindingInput.DAYDREAM_SHAKE, true);
                sink.onGesture(BindingInput.DAYDREAM_SHAKE, false);
            }
        }
    }

    void reset(Sink sink) {
        if (held && sink != null)
            sink.onGesture(BindingInput.DAYDREAM_PRESS_HOLD, false);
        pressed = false;
        held = false;
        pressedSince = 0L;
        pendingTap = 0L;
        shakeArmed = true;
    }
}
