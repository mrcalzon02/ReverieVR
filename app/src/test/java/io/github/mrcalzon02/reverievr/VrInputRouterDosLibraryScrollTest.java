package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class VrInputRouterDosLibraryScrollTest {
    private static final class Listener implements VrInputRouter.Listener {
        int navigationActions;
        float swipeDistance;
        int endedCount;
        boolean libraryVisible = true;
        boolean dosMenuVisible;
        int up;
        int down;

        @Override public void onInputAction(VrInputAction action, String source) {
            if (action == VrInputAction.NAV_UP
                || action == VrInputAction.NAV_DOWN) navigationActions++;
            if (action == VrInputAction.NAV_UP) up++;
            if (action == VrInputAction.NAV_DOWN) down++;
        }

        @Override public boolean usesDosShellMenuNavigation() {
            return dosMenuVisible;
        }

        @Override public boolean usesDosLibraryTouchScroll() {
            return libraryVisible;
        }

        @Override public void onDosLibraryTouchScroll(
            float deltaY, boolean ended) {
            if (ended) endedCount++;
            else swipeDistance += deltaY;
        }
    }

    private static ControllerSnapshot touch(boolean touching, int y, long time) {
        return new ControllerSnapshot(
            time, 0, 0,
            0f, 0f, 0f, 1f,
            0f, 0f, 0f,
            0f, 0f, 0f,
            touching, 128, y,
            false, false, false, false, false, 0);
    }

    @Test public void swipingScrollableLibraryDoesNotFirePageNavigation() {
        Listener listener = new Listener();
        VrInputRouter router = new VrInputRouter(listener);
        router.onControllerSnapshot(touch(true, 210, 1000000000L), "BLE");
        router.onControllerSnapshot(touch(true, 170, 1010000000L), "BLE");
        router.onControllerSnapshot(touch(true, 125, 1020000000L), "BLE");
        router.onControllerSnapshot(touch(false, 125, 1030000000L), "BLE");
        assertEquals(-85f, listener.swipeDistance, 0.001f);
        assertEquals(1, listener.endedCount);
        assertEquals(0, listener.navigationActions);
    }

    @Test public void dosOverlayPadDirectionsMoveWithoutClickAndRepeatAtHeldCadence() {
        Listener listener = new Listener();
        listener.libraryVisible = false;
        listener.dosMenuVisible = true;
        VrInputRouter router = new VrInputRouter(listener);
        router.onControllerSnapshot(touch(true, 15, 1000000000L), "BLE");
        assertEquals(1, listener.up);
        router.onControllerSnapshot(touch(true, 15, 1100000000L), "BLE");
        assertEquals(1, listener.up);
        router.onControllerSnapshot(touch(true, 15, 1370000000L), "BLE");
        assertEquals(2, listener.up);
        router.onControllerSnapshot(touch(false, 15, 1390000000L), "BLE");
        assertEquals(2, listener.navigationActions); // no extra lift swipe
        router.onControllerSnapshot(touch(true, 245, 1500000000L), "BLE");
        assertEquals(1, listener.down);
        router.onControllerSnapshot(touch(true, 128, 1510000000L), "BLE");
        router.onControllerSnapshot(touch(true, 245, 1520000000L), "BLE");
        assertEquals(2, listener.down); // neutral re-arms without click
    }

    @Test public void otherScreensKeepNormalTouchpadSwipeNavigation() {
        Listener listener = new Listener();
        listener.libraryVisible = false;
        VrInputRouter router = new VrInputRouter(listener);
        router.onControllerSnapshot(touch(true, 200, 1000000000L), "BLE");
        router.onControllerSnapshot(touch(true, 120, 1010000000L), "BLE");
        router.onControllerSnapshot(touch(false, 120, 1020000000L), "BLE");
        assertEquals(0, listener.endedCount);
        assertTrue(listener.navigationActions > 0);
    }
}
