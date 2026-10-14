package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class ControlEventHistoryTest {
    @Test public void onlyRecentControlsRemainAndDuplicatesAreDebounced() {
        ControlEventHistory h = new ControlEventHistory();
        h.record("SELECT", 1000000000L);
        h.record("SELECT", 1050000000L);
        h.record("DOUBLE TAP", 2000000000L);
        String[] labels = new String[ControlEventHistory.CAPACITY];
        long[] ages = new long[labels.length];
        assertEquals(2, h.copyRecent(3000000000L, labels, ages));
        assertEquals("SELECT", labels[0]);
        assertEquals("DOUBLE TAP", labels[1]);
        assertFalse(h.isDirty());
        assertFalse(h.hasRecent(17000000000L));
    }
}
