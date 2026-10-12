package io.github.mrcalzon02.reverievr;

/**
 * Small, allocation-free visual hover hysteresis for the shell's ray targets.
 * The raycast remains authoritative on Select: a stale highlight must never
 * activate a different or occluded control. No smoothing of the physical aim
 * ray or of native/DOS gameplay input is performed.
 */
final class VrUiTargetStabilizer {
    static final long SWITCH_NANOS = 45_000_000L;
    static final long LOSS_GRACE_NANOS = 65_000_000L;
    static final long MAX_FRAME_GAP_NANOS = 250_000_000L;

    private int visible = -1;
    private int candidate = -1;
    private long candidateSince;
    private long lastSeen;
    private long lastFrame = -1L;

    int update(int rawTarget, long nowNanos, boolean selectRequested) {
        if (rawTarget < 0) rawTarget = -1;
        if (nowNanos < 0L || (lastFrame >= 0L
                && (nowNanos < lastFrame
                    || nowNanos - lastFrame > MAX_FRAME_GAP_NANOS))) {
            reset();
        }
        if (nowNanos < 0L) return -1;
        lastFrame = nowNanos;

        // No visual latch may ever turn a current ray miss into an action.
        // Selecting an actual fresh target also bypasses delay entirely.
        if (selectRequested || visible < 0 && rawTarget >= 0) {
            visible = rawTarget;
            candidate = -1;
            candidateSince = nowNanos;
            lastSeen = nowNanos;
            return visible;
        }
        if (rawTarget == visible) {
            candidate = -1;
            lastSeen = nowNanos;
            return visible;
        }
        if (rawTarget < 0) {
            candidate = -1;
            if (visible >= 0 && nowNanos - lastSeen >= LOSS_GRACE_NANOS)
                visible = -1;
            return visible;
        }
        if (candidate != rawTarget) {
            candidate = rawTarget;
            candidateSince = nowNanos;
        } else if (nowNanos - candidateSince >= SWITCH_NANOS) {
            visible = rawTarget;
            lastSeen = nowNanos;
            candidate = -1;
        }
        return visible;
    }

    void reset() {
        visible = -1;
        candidate = -1;
        candidateSince = 0L;
        lastSeen = 0L;
        lastFrame = -1L;
    }
}
