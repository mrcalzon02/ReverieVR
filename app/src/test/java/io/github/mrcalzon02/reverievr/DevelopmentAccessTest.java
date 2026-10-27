package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class DevelopmentAccessTest {
    @Test public void playIsFailClosedForAllPreferenceStates() {
        for (boolean developer : new boolean[]{false, true}) {
            for (boolean option : new boolean[]{false, true}) {
                assertFalse(DevelopmentAccess.permitted(true, developer, option));
            }
        }
    }

    @Test public void sideloadRequiresMasterAndSpecificOption() {
        assertFalse(DevelopmentAccess.permitted(false, false, false));
        assertFalse(DevelopmentAccess.permitted(false, false, true));
        assertFalse(DevelopmentAccess.permitted(false, true, false));
        assertTrue(DevelopmentAccess.permitted(false, true, true));
    }
}
