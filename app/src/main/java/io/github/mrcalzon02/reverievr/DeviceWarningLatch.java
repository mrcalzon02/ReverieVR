package io.github.mrcalzon02.reverievr;

/**
 * Small monotonic-time latch for honest, non-flickering battery/thermal
 * warnings. Escalation is immediate; a lower severity must remain stable
 * for 10 seconds before presentation changes.
 */
final class DeviceWarningLatch {
    static final long CLEAR_HOLD_NANOS = 10_000_000_000L;
    private int visible = DeviceWarningPolicy.NONE;
    private long lowerSinceNanos;

    int update(int observed, long nowNanos) {
        if (observed < DeviceWarningPolicy.NONE
            || observed > DeviceWarningPolicy.THERMAL_CRITICAL
            || nowNanos <= 0L) {
            return visible;
        }
        if (observed >= visible) {
            visible = observed;
            lowerSinceNanos = 0L;
            return visible;
        }
        if (lowerSinceNanos == 0L || nowNanos < lowerSinceNanos) {
            lowerSinceNanos = nowNanos;
            return visible;
        }
        if (nowNanos - lowerSinceNanos >= CLEAR_HOLD_NANOS) {
            visible = observed;
            lowerSinceNanos = 0L;
        }
        return visible;
    }

    void reset() {
        visible = DeviceWarningPolicy.NONE;
        lowerSinceNanos = 0L;
    }
}
