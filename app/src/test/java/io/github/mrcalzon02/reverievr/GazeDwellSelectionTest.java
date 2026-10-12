package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class GazeDwellSelectionTest {
    @Test public void firesOnceOnlyAfterFiveRealSeconds() {
        GazeDwellSelection d=new GazeDwellSelection();
        long t=1_000_000L;
        assertFalse(d.update(true,10,2,t));
        assertFalse(d.update(true,10,2,t+4_999_999_999L));
        assertTrue(d.progress()>.99f);
        assertTrue(d.update(true,10,2,t+5_000_000_000L));
        assertEquals(1f,d.progress(),0f);
        assertTrue(d.completed());
        assertFalse(d.update(true,10,2,t+9_000_000_000L));
    }

    @Test public void lookingAwayOrChangingPanelCancelsEntireCountdown() {
        GazeDwellSelection d=new GazeDwellSelection();
        long t=8_000_000_000L;
        d.update(true,1,2,t);
        d.update(true,1,2,t+3_000_000_000L);
        assertTrue(d.progress()>.5f);
        assertFalse(d.update(true,1,-1,t+3_100_000_000L));
        assertEquals(0f,d.progress(),0f);
        assertFalse(d.update(true,1,2,t+3_200_000_000L));
        assertFalse(d.update(true,1,2,t+5_200_000_000L));
        assertFalse(d.update(true,2,2,t+6_000_000_000L));
        assertEquals(0f,d.progress(),0f);
        assertTrue(d.update(true,2,2,t+11_000_000_000L));
    }

    @Test public void controllerReturnAndClockRegressionReset() {
        GazeDwellSelection d=new GazeDwellSelection();
        long t=10_000_000_000L;
        d.update(true,1,4,t);
        assertFalse(d.update(false,1,4,t+4_000_000_000L));
        assertEquals(0f,d.progress(),0f);
        d.update(true,1,4,t+5_000_000_000L);
        assertFalse(d.update(true,1,4,t+4_900_000_000L));
        assertEquals(0f,d.progress(),0f);
        assertFalse(d.update(true,1,4,t+10_000_000_000L));
        assertTrue(d.update(true,1,4,t+15_000_000_000L));
        assertFalse(d.update(true,1,4,t+20_000_000_000L));
        d.update(true,1,5,t+20_100_000_000L);
        assertTrue(d.update(true,1,5,t+25_100_000_000L));
    }
}
