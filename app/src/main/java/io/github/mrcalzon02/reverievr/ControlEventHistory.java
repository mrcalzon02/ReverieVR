package io.github.mrcalzon02.reverievr;

/** Bounded low-cost history of actual normalized input transitions. */
final class ControlEventHistory {
    static final int CAPACITY = 6;
    static final long TTL_NANOS = 12000000000L;
    private final String[] events = new String[CAPACITY];
    private final long[] times = new long[CAPACITY];
    private int size;
    private long revision;
    private long renderedRevision;

    synchronized void record(String label, long when) {
        if (label == null || label.isEmpty() || when <= 0L) return;
        String trimmed = label.length() <= 31 ? label : label.substring(0, 31);
        if (size > 0 && trimmed.equals(events[size - 1])
            && when - times[size - 1] < 120000000L) return;
        if (size == CAPACITY) {
            System.arraycopy(events, 1, events, 0, CAPACITY - 1);
            System.arraycopy(times, 1, times, 0, CAPACITY - 1);
            size--;
        }
        events[size] = trimmed;
        times[size] = when;
        size++;
        revision++;
    }

    synchronized boolean isDirty() {
        return revision != renderedRevision;
    }

    synchronized boolean hasRecent(long now) {
        for (int i = 0; i < size; i++)
            if (now >= times[i] && now - times[i] < TTL_NANOS) return true;
        return false;
    }

    synchronized int copyRecent(long now, String[] labels, long[] ages) {
        int count = 0;
        for (int i = 0; i < size; i++) {
            long age = now - times[i];
            if (age < 0L || age >= TTL_NANOS) continue;
            labels[count] = events[i];
            ages[count] = age;
            count++;
        }
        renderedRevision = revision;
        return count;
    }
}
