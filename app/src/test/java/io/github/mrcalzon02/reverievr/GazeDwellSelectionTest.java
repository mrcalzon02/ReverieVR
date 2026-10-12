package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class GazeDwellSelectionTest {
    private static boolean focusThrough(GazeDwellSelection dwell, int context,
        int target, long from, long until) {
        boolean fired = false;
        for (long tick = from + 100_000_000L; tick < until;
             tick += 100_000_000L) {
            fired |= dwell.update(true, context, target, tick);
        }
        return fired;
    }

    @Test public void firesOnceOnlyAfterFiveContinuousSeconds() {
        GazeDwellSelection d = new GazeDwellSelection();
        long t = 1_000_000_000L;
        assertFalse(d.update(true, 10, 2, t));
        assertFalse(focusThrough(d, 10, 2, t, t + 5_000_000_000L));
        assertTrue(d.progress() > .97f);
        assertFalse(d.update(true, 10, 2, t + 4_999_999_999L));
        assertTrue(d.update(true, 10, 2, t + 5_000_000_000L));
        assertEquals(1f, d.progress(), 0f);
        assertTrue(d.completed());
        assertFalse(d.update(true, 10, 2, t + 5_100_000_000L));
    }

    @Test public void lookingAwayOrChangingPanelCancelsEntireCountdown() {
        GazeDwellSelection d = new GazeDwellSelection();
        long t = 8_000_000_000L;
        d.update(true, 1, 2, t);
        assertFalse(focusThrough(d, 1, 2, t, t + 3_000_000_000L));
        assertTrue(d.progress() > .5f);
        assertFalse(d.update(true, 1, -1, t + 3_100_000_000L));
        assertEquals(0f, d.progress(), 0f);
        assertFalse(d.update(true, 1, 2, t + 3_200_000_000L));
        assertFalse(focusThrough(d, 1, 2, t + 3_200_000_000L, t + 5_200_000_000L));
        assertFalse(d.update(true, 1, 2, t + 5_200_000_000L));
        assertFalse(d.update(true, 2, 2, t + 6_000_000_000L));
        assertEquals(0f, d.progress(), 0f);
        assertFalse(focusThrough(d, 2, 2, t + 6_000_000_000L, t + 11_000_000_000L));
        assertTrue(d.update(true, 2, 2, t + 11_000_000_000L));
    }

    @Test public void controllerReturnAndClockRegressionReset() {
        GazeDwellSelection d = new GazeDwellSelection();
        long t = 10_000_000_000L;
        d.update(true, 1, 4, t);
        assertFalse(focusThrough(d, 1, 4, t, t + 4_000_000_000L));
        assertFalse(d.update(false, 1, 4, t + 4_000_000_000L));
        assertEquals(0f, d.progress(), 0f);
        d.update(true, 1, 4, t + 5_000_000_000L);
        assertFalse(d.update(true, 1, 4, t + 4_900_000_000L));
        assertEquals(0f, d.progress(), 0f);
        assertFalse(d.update(true, 1, 4, t + 9_000_000_000L));
        assertFalse(focusThrough(d, 1, 4, t + 9_000_000_000L, t + 14_000_000_000L));
        assertTrue(d.update(true, 1, 4, t + 14_000_000_000L));
        assertFalse(d.update(true, 1, 4, t + 20_000_000_000L));
        d.update(true, 1, 5, t + 20_100_000_000L);
        assertFalse(focusThrough(d, 1, 5, t + 20_100_000_000L, t + 25_100_000_000L));
        assertTrue(d.update(true, 1, 5, t + 25_100_000_000L));
    }

    @Test public void pausedRendererCannotInstantlySelectOrReopenRecoveryMenu() {
        GazeDwellSelection d = new GazeDwellSelection();
        long t = 30_000_000_000L;
        d.update(true, 9, 7, t);
        assertFalse(focusThrough(d, 9, 7, t, t + 4_900_000_000L));
        assertFalse(d.update(true, 9, 7, t + 6_000_000_000L));
        assertEquals(0f, d.progress(), 0f);
        assertFalse(d.completed());
        assertFalse(focusThrough(d, 9, 7, t + 6_000_000_000L, t + 11_000_000_000L));
        assertTrue(d.update(true, 9, 7, t + 11_000_000_000L));
        d.reset();
        assertFalse(d.update(true, 15, 0, t));
        assertFalse(focusThrough(d, 15, 0, t, t + 4_900_000_000L));
        assertFalse(d.update(true, 15, 0, t + 6_000_000_000L));
        assertEquals(0f, d.progress(), 0f);
    }
}
