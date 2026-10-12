package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.*;
import org.junit.Test;

public final class HomeKeyboardTwinTest {
    @Test public void physicalKeyLightsMatchingDigitalKeyAndReleases() {
        HomeKeyboardTwin twin = new HomeKeyboardTwin();
        long time = 1_000_000_000L;
        twin.setConnected(true);
        twin.physicalEdge(12, VirtualKey.Q, true, time);
        assertTrue(twin.illuminated(10, time + 30_000_000L));
        assertTrue(twin.held(VirtualKey.Q));
        assertFalse(twin.illuminated(11, time + 30_000_000L));
        twin.physicalEdge(12, VirtualKey.Q, false, time + 40_000_000L);
        assertFalse(twin.held(VirtualKey.Q));
        assertTrue(twin.illuminated(10, time + 90_000_000L));
        assertFalse(twin.illuminated(10, time + 240_000_000L));
    }

    @Test public void digitalKeysLightIdenticallyToPhysicalOnes() {
        HomeKeyboardTwin twin = new HomeKeyboardTwin();
        twin.virtualPress(48, 1_000_000_000L);
        assertTrue(twin.illuminated(48, 1_050_000_000L));
        assertFalse(twin.illuminated(48, 1_170_000_000L));
        assertEquals(48, HomeKeyboardTwin.keyIndex(VirtualKey.ENTER));
        assertEquals(46, HomeKeyboardTwin.keyIndex(VirtualKey.BACKSPACE));
        assertEquals(47, HomeKeyboardTwin.keyIndex(VirtualKey.DELETE));
        assertEquals(40, HomeKeyboardTwin.keyIndex(VirtualKey.CAPS_LOCK));
        assertEquals(30, HomeKeyboardTwin.keyIndex(VirtualKey.SHIFT));
        assertEquals(29, HomeKeyboardTwin.keyIndex(VirtualKey.SEMICOLON));
        assertEquals(-1, HomeKeyboardTwin.keyIndex(VirtualKey.CTRL));
    }

    @Test public void multiplePhysicalDevicesDoNotDropHeldTwinKey() {
        HomeKeyboardTwin twin = new HomeKeyboardTwin();
        long t = 2_000_000_000L;
        twin.setConnected(true);
        twin.physicalEdge(11, VirtualKey.A, true, t);
        twin.physicalEdge(12, VirtualKey.A, true, t);
        twin.physicalEdge(11, VirtualKey.A, false, t + 10);
        assertTrue(twin.held(VirtualKey.A));
        twin.deviceRemoved(12);
        assertFalse(twin.held(VirtualKey.A));
        assertFalse(twin.illuminated(20, t + 200_000_000L));
    }

    @Test public void noHardwareStillAllowsVirtualTapFeedback() {
        HomeKeyboardTwin twin = new HomeKeyboardTwin();
        long t = 5_000_000_000L;
        twin.setConnected(false);
        twin.virtualPress(10, t);
        twin.setConnected(false); // next GL frame
        assertTrue(twin.illuminated(10, t + 1));
        assertFalse(twin.illuminated(10,
            t + HomeKeyboardTwin.TAP_VISIBLE_NANOS + 1));
    }

    @Test public void disabledOrDisconnectedHardwareClearsHeldKeys() {
        HomeKeyboardTwin twin = new HomeKeyboardTwin();
        long t = 2_000_000_000L;
        twin.setConnected(true);
        twin.physicalEdge(1, VirtualKey.SHIFT, true, t);
        assertTrue(twin.held(VirtualKey.SHIFT));
        twin.setConnected(false);
        assertFalse(twin.connected());
        assertFalse(twin.held(VirtualKey.SHIFT));
        assertFalse(twin.illuminated(30, t + 10));
        twin.physicalEdge(1, VirtualKey.SHIFT, true, t);
        assertFalse(twin.held(VirtualKey.SHIFT));
    }

    @Test public void changesToLightStateRequestRedrawAfterTap() {
        HomeKeyboardTwin twin = new HomeKeyboardTwin();
        long t = 3_000_000_000L;
        twin.setConnected(true);
        assertFalse(twin.updateVisualState(t));
        twin.virtualPress(35, t);
        assertTrue(twin.updateVisualState(t + 1));
        assertFalse(twin.updateVisualState(t + 2));
        assertTrue(twin.updateVisualState(t + HomeKeyboardTwin.TAP_VISIBLE_NANOS + 1));
        assertFalse(twin.updateVisualState(t + HomeKeyboardTwin.TAP_VISIBLE_NANOS + 2));
    }

    @Test public void physicalShiftChangesDigitalOutputWithoutSticking() {
        VrKeyboardEditor editor = new VrKeyboardEditor();
        editor.setPhysicalShift(true);
        assertEquals("Q", editor.label(1, 0));
        assertEquals("!", editor.label(0, 0));
        assertEquals(":", editor.label(2, 9));
        assertTrue(editor.press(1, 0));
        assertEquals("Q", editor.value());
        editor.setPhysicalShift(false);
        assertEquals("q", editor.label(1, 0));
        assertEquals("1", editor.label(0, 0));
        assertTrue(editor.press(0, 0));
        assertEquals("Q1", editor.value());
    }

    @Test public void externalCapsAndDigitalCapsUseOneEditorState() {
        VrKeyboardEditor editor = new VrKeyboardEditor();
        assertTrue(editor.applyExternal(VrKeyboardEditor.EXTERNAL_CAPS_TOGGLE, null));
        assertEquals("Q", editor.label(1, 0));
        assertTrue(editor.press(4, 0));
        assertEquals("q", editor.label(1, 0));
    }
}
