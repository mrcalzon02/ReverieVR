package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import org.junit.Test;

public final class DosLibraryPagingTest {
    @Test
    public void multipleBundlesUseAllPagesAndNeverAccessEmptySlots() {
        for (int count : new int[]{0, 1, 2, 3, 4, 7, 9, 16, 51, 100}) {
            int pages = DosLibraryPaging.pageCount(count);
            int seen = 0;
            for (int page = 0; page < pages; page++) {
                for (int slot = 0; slot < DosLibraryPaging.PAGE_SIZE; slot++) {
                    int index = DosLibraryPaging.moduleIndex(page, slot, count);
                    if (index < 0) continue;
                    assertEquals(count + " slot order", seen++, index);
                }
            }
            assertEquals(count, seen);
            assertEquals(0, DosLibraryPaging.nextPage(pages - 1, count));
        }
    }

    @Test
    public void clampsStaleSelectionAndHandlesExtremeNumbers() {
        assertEquals(1, DosLibraryPaging.pageCount(0));
        assertEquals(1, DosLibraryPaging.pageCount(-4));
        assertEquals(3, DosLibraryPaging.pageCount(7));
        assertEquals(0, DosLibraryPaging.clampPage(-8, 7));
        assertEquals(2, DosLibraryPaging.clampPage(100, 7));
        assertEquals(-1, DosLibraryPaging.moduleIndex(2, 1, 7));
        assertEquals(-1, DosLibraryPaging.moduleIndex(0, -1, 7));
        assertEquals(-1, DosLibraryPaging.moduleIndex(0, 3, 7));
        assertEquals(715827883,
            DosLibraryPaging.pageCount(Integer.MAX_VALUE));
        assertEquals(Integer.MAX_VALUE - 1,
            DosLibraryPaging.moduleIndex(715827882, 0, Integer.MAX_VALUE));
    }
}
