package io.github.mrcalzon02.reverievr;

/**
 * One offset-based scroll contract for the DOS shelf's rendering, hit testing
 * and launch selection. No page tabs, wraparound, or module-count ceiling.
 */
final class DosLibraryScroll {
    static final int VISIBLE_ROWS = 5;
    static final float TOUCH_PIXELS_PER_ROW = 26.0f;

    private DosLibraryScroll() {}

    static int maxFirst(int count) {
        return Math.max(0, count - VISIBLE_ROWS);
    }

    static int clampFirst(int first, int count) {
        return Math.max(0, Math.min(first, maxFirst(count)));
    }

    static int indexAt(int first, int row, int count) {
        if (count <= 0 || row < 0 || row >= VISIBLE_ROWS
            || first < 0 || first > maxFirst(count)) return -1;
        long index = (long) first + row;
        return index >= count ? -1 : (int) index;
    }

    static int move(int first, int rows, int count) {
        long updated = (long) clampFirst(first, count) + rows;
        return (int) Math.max(0L, Math.min(updated, maxFirst(count)));
    }

    // Keep the same topmost visible game when imports refresh the shelf,
    // but let fresh imports appear naturally when already at the top.
    static int retainAnchor(String[] previousIds, int previousFirst,
                            String[] nextIds) {
        int nextCount = nextIds == null ? 0 : nextIds.length;
        int fallback = clampFirst(previousFirst, nextCount);
        if (previousFirst <= 0 || previousIds == null
            || previousFirst >= previousIds.length || nextIds == null) {
            return fallback;
        }
        String anchor = previousIds[previousFirst];
        for (int index = 0; index < nextIds.length; index++) {
            if (anchor != null && anchor.equals(nextIds[index])) {
                return clampFirst(index, nextCount);
            }
        }
        return fallback;
    }

    static int shown(int count) {
        return Math.max(0, Math.min(count, VISIBLE_ROWS));
    }
}
