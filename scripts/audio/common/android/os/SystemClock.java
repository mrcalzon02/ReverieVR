package android.os;

/** Monotonic time controlled by the harness; no sleeps or wall-clock races. */
public final class SystemClock {
    private static long now = 10_000L;
    private SystemClock() { }
    public static long uptimeMillis() { return now; }
    public static void setTime(long value) {
        if (value < now) throw new IllegalArgumentException("time moved backwards");
        now = value;
    }
    static void reset() { now = 10_000L; }
}
