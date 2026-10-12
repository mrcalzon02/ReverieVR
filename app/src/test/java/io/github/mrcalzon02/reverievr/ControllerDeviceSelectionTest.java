package io.github.mrcalzon02.reverievr;

import org.junit.Test;
import static org.junit.Assert.*;

public class ControllerDeviceSelectionTest {
    @Test public void selectsSortedDistinctDevicesAndCyclesExplicitly() {
        String[] d = ControllerDeviceSelection.selectable(
            new String[]{"gamepad-b", "gamepad-a", "gamepad-c"});
        assertArrayEquals(new String[]{"gamepad-a","gamepad-b","gamepad-c"}, d);
        assertEquals("gamepad-b", ControllerDeviceSelection.retainOrFirst(d, "gamepad-b"));
        assertEquals("gamepad-a", ControllerDeviceSelection.retainOrFirst(d, "missing"));
        assertEquals("gamepad-c", ControllerDeviceSelection.next(d, "gamepad-b"));
        assertEquals("gamepad-a", ControllerDeviceSelection.next(d, "gamepad-c"));
    }

    @Test public void neverGrantsTwoIdenticalOrAnonymousDevicesPrimaryControl() {
        String[] d = ControllerDeviceSelection.selectable(
            new String[]{"repeat", "repeat", "unique", "", null, "  "});
        assertArrayEquals(new String[]{"unique"}, d);
        assertFalse(ControllerDeviceSelection.accepts("repeat", "repeat", d));
        assertTrue(ControllerDeviceSelection.accepts("unique", "unique", d));
        assertFalse(ControllerDeviceSelection.accepts("unique", "repeat", d));
    }

    @Test public void disconnectedSelectedDeviceDoesNotImplicitlySwap() {
        String[] online = ControllerDeviceSelection.selectable(
            new String[]{"one", "two"});
        assertTrue(ControllerDeviceSelection.accepts("two", "two", online));
        String[] afterUnplug = ControllerDeviceSelection.selectable(
            new String[]{"one"});
        assertFalse(ControllerDeviceSelection.accepts("two", "one", afterUnplug));
        assertFalse(ControllerDeviceSelection.accepts("two", "two", afterUnplug));
    }

    @Test public void emptyDeviceListCannotClaimAnyController() {
        String[] empty = ControllerDeviceSelection.selectable(null);
        assertEquals("", ControllerDeviceSelection.next(empty, "old"));
        assertEquals("", ControllerDeviceSelection.retainOrFirst(empty, "old"));
        assertFalse(ControllerDeviceSelection.accepts("old", "old", empty));
    }
}
