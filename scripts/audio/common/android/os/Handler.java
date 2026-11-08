package android.os;

import java.util.PriorityQueue;

/** Single UI queue with handler-scoped cancellation and deterministic timers. */
public final class Handler {
    private static final PriorityQueue<Work> WORK = new PriorityQueue<>();
    private static long sequence;
    private static final class Work implements Comparable<Work> {
        final Handler owner;
        final Runnable runnable;
        final long at, order;
        Work(Handler owner, Runnable runnable, long at) {
            this.owner = owner; this.runnable = runnable; this.at = at;
            order = sequence++;
        }
        @Override public int compareTo(Work other) {
            int time = Long.compare(at, other.at);
            return time != 0 ? time : Long.compare(order, other.order);
        }
    }
    public Handler(Looper ignored) { }
    public boolean post(Runnable work) { return postDelayed(work, 0L); }
    public boolean postDelayed(Runnable work, long delay) {
        WORK.add(new Work(this, work, SystemClock.uptimeMillis() + Math.max(0L, delay)));
        return true;
    }
    public void removeCallbacks(Runnable work) {
        WORK.removeIf(item -> item.owner == this && item.runnable == work);
    }
    public void removeCallbacksAndMessages(Object ignored) {
        WORK.removeIf(item -> item.owner == this);
    }
    public static void drain() { advanceTo(SystemClock.uptimeMillis()); }
    public static void advanceBy(long duration) {
        advanceTo(SystemClock.uptimeMillis() + duration);
    }
    public static void advanceTo(long target) {
        if (target < SystemClock.uptimeMillis()) throw new IllegalArgumentException("past target");
        int dispatched = 0;
        while (!WORK.isEmpty() && WORK.peek().at <= target) {
            if (++dispatched > 100_000) throw new AssertionError("unbounded callback loop");
            Work work = WORK.remove();
            SystemClock.setTime(work.at);
            work.runnable.run();
        }
        SystemClock.setTime(target);
    }
    public static int pendingCount() { return WORK.size(); }
    public static void reset() { WORK.clear(); sequence = 0L; SystemClock.reset(); }
}
