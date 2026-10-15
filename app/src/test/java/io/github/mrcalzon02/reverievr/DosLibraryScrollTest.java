package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import org.junit.Test;

public final class DosLibraryScrollTest {
    @Test
    public void allGamesRemainReachableWithoutPages() {
        for (int count : new int[]{0, 1, 3, 5, 6, 7, 23, 51, 100, 301}) {
            int first = 0;
            for (int i = 0; i < DosLibraryScroll.maxFirst(count); i++)
                first = DosLibraryScroll.move(first, 1, count);
            assertEquals(DosLibraryScroll.maxFirst(count), first);
            int last = -1;
            for (int row = 0; row < DosLibraryScroll.VISIBLE_ROWS; row++) {
                int index = DosLibraryScroll.indexAt(first, row, count);
                if (index < 0) continue;
                last = index;
            }
            assertEquals(count - 1, last);
            assertEquals(first, DosLibraryScroll.move(first, 10, count));
            assertEquals(0, DosLibraryScroll.move(first, -1000, count));
        }
    }

    @Test
    public void libraryRefreshKeepsVisibleAnchorWithoutHidingNewTopImports() {
        String[] oldIds = {"latest", "new", "Ken", "Jetpack",
            "Squarez", "DOOM", "a", "b", "c"};
        String[] nextIds = {"fresh", "latest", "new", "Ken", "Jetpack",
            "Squarez", "DOOM", "a", "b", "c"};
        assertEquals(3, DosLibraryScroll.retainAnchor(oldIds, 2, nextIds));
        assertEquals(0, DosLibraryScroll.retainAnchor(oldIds, 0, nextIds));
        assertEquals(2, DosLibraryScroll.retainAnchor(oldIds, 2,
            new String[]{"fresh", "latest", "other", "Jetpack",
                "Squarez", "DOOM", "a", "b", "c"}));
        assertEquals(0, DosLibraryScroll.retainAnchor(oldIds, 8,
            new String[]{"DOOM"}));
    }

    @Test
    public void rowsAndExtremeScrollValuesStayBounded() {
        assertEquals(0, DosLibraryScroll.maxFirst(5));
        assertEquals(2, DosLibraryScroll.maxFirst(7));
        assertEquals(6, DosLibraryScroll.indexAt(2, 4, 7));
        assertEquals(-1, DosLibraryScroll.indexAt(2, 5, 7));
        assertEquals(-1, DosLibraryScroll.indexAt(0, 3, 2));
        assertEquals(-1, DosLibraryScroll.indexAt(-1, 0, 10));
        assertEquals(Integer.MAX_VALUE - 5,
            DosLibraryScroll.maxFirst(Integer.MAX_VALUE));
        assertEquals(Integer.MAX_VALUE - 1,
            DosLibraryScroll.indexAt(Integer.MAX_VALUE - 5, 4,
                Integer.MAX_VALUE));
        assertEquals(Integer.MAX_VALUE - 5,
            DosLibraryScroll.move(Integer.MAX_VALUE - 5,
                Integer.MAX_VALUE, Integer.MAX_VALUE));
        assertEquals(0, DosLibraryScroll.move(0,
            Integer.MIN_VALUE, Integer.MAX_VALUE));
    }
}
