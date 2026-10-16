package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public final class HomeMainMenuLayoutTest {
    private static final int[] ACTIONS = {
        HomeMainMenuLayout.CONTROLLER,
        HomeMainMenuLayout.KEYBOARD,
        HomeMainMenuLayout.MODEL,
        HomeMainMenuLayout.IMPORT,
        HomeMainMenuLayout.RESUME
    };

    @Test public void centerActionsAreVisibleAndNeverOverlap() {
        for (int i = 0; i < ACTIONS.length; i++) {
            int[] rect = HomeMainMenuLayout.centerRect(ACTIONS[i]);
            assertTrue(rect[0] >= 266 && rect[2] <= 744);
            assertTrue(rect[1] >= 180 && rect[3] <= 610);
            assertTrue("Gaze targets should not be tiny", rect[2]-rect[0] >= 350);
            assertTrue("Touch targets should be tall enough", rect[3]-rect[1] >= 60);
            assertTrue(HomeMainMenuLayout.contains(rect,
                (rect[0]+rect[2])/2, (rect[1]+rect[3])/2));
            assertFalse(HomeMainMenuLayout.contains(rect, rect[2]+1, rect[3]+1));
            for (int j = 0; j < i; j++) {
                assertFalse("Centered action targets must be distinct",
                    HomeMainMenuLayout.intersects(rect,
                        HomeMainMenuLayout.centerRect(ACTIONS[j])));
            }
        }
    }

    @Test public void resumeIsBelowEssentialsAndKeepsKeyboardClear() {
        int[] controller = HomeMainMenuLayout.centerRect(HomeMainMenuLayout.CONTROLLER);
        int[] keyboard = HomeMainMenuLayout.centerRect(HomeMainMenuLayout.KEYBOARD);
        int[] model = HomeMainMenuLayout.centerRect(HomeMainMenuLayout.MODEL);
        int[] importer = HomeMainMenuLayout.centerRect(HomeMainMenuLayout.IMPORT);
        int[] resume = HomeMainMenuLayout.centerRect(HomeMainMenuLayout.RESUME);
        assertTrue(controller[3] < keyboard[1]);
        assertTrue(keyboard[3] < model[1]);
        assertTrue(model[3] < importer[1]);
        assertTrue(importer[3] < resume[1]);
        assertTrue("Leave room for one contextual status row", resume[3] < 620);
    }

    @Test public void exitToPhoneStillUsesIndependentRightRailIdentity() {
        assertEquals(26, HomeMainMenuLayout.EXIT_TO_PHONE);
        for (int action : ACTIONS) assertNotEquals(action, HomeMainMenuLayout.EXIT_TO_PHONE);
        try {
            HomeMainMenuLayout.centerRect(HomeMainMenuLayout.EXIT_TO_PHONE);
            fail("Exit to Phone is not a center action");
        } catch (IllegalArgumentException expected) {
            // Right rail owns this control; do not silently move it to center.
        }
    }
}
