package io.github.mrcalzon02.reverievr;

/**
 * One paging contract shared by the DOS shelf's drawing, hit testing and
 * selection. The library's size is based on real installed modules, not a
 * fixed number of built-in games.
 */
final class DosLibraryPaging {
    static final int PAGE_SIZE = 3;

    private DosLibraryPaging() {}

    static int pageCount(int moduleCount) {
        if (moduleCount <= 0) return 1;
        // Avoid the integer overflow in (count + PAGE_SIZE - 1).
        return 1 + (moduleCount - 1) / PAGE_SIZE;
    }

    static int clampPage(int page, int moduleCount) {
        return Math.max(0, Math.min(page, pageCount(moduleCount) - 1));
    }

    static int moduleIndex(int page, int slot, int moduleCount) {
        if (moduleCount <= 0 || slot < 0 || slot >= PAGE_SIZE
            || page < 0 || page >= pageCount(moduleCount)) {
            return -1;
        }
        int index = page * PAGE_SIZE + slot;
        return index < moduleCount ? index : -1;
    }

    static int nextPage(int page, int moduleCount) {
        return (clampPage(page, moduleCount) + 1) % pageCount(moduleCount);
    }
}
