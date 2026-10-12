package io.github.mrcalzon02.reverievr;

import java.util.EnumSet;
import java.util.HashMap;
import java.util.Map;

/**
 * Physical HID <-> Home desk keyboard visual state. Driven solely on the
 * Cardboard render thread, without allocating in a stereo draw callback.
 * Does not inject keys into Android or a physical keyboard device.
 */
final class HomeKeyboardTwin {
    static final int KEY_COUNT = 51;
    static final long TAP_VISIBLE_NANOS = 160_000_000L;

    private final Map<Integer, EnumSet<VirtualKey>> heldByDevice =
        new HashMap<>();
    private final long[] litUntil = new long[KEY_COUNT];
    private final boolean[] lastIlluminated = new boolean[KEY_COUNT];
    private boolean connected;

    void setConnected(boolean attached) {
        // Only clear on a real disconnect, not on every frame. Virtual
        // key tap feedback also works when no hardware keyboard is paired.
        if (connected && !attached) releaseAll();
        connected = attached;
    }

    boolean connected() {
        return connected;
    }

    void physicalEdge(int deviceId, VirtualKey key, boolean down,
                      long nowNanos) {
        if (!connected || key == null) return;
        int slot = keyIndex(key);
        if (down) {
            EnumSet<VirtualKey> device = heldByDevice.get(deviceId);
            if (device == null) {
                device = EnumSet.noneOf(VirtualKey.class);
                heldByDevice.put(deviceId, device);
            }
            device.add(key);
        } else {
            EnumSet<VirtualKey> device = heldByDevice.get(deviceId);
            if (device != null) {
                device.remove(key);
                if (device.isEmpty()) heldByDevice.remove(deviceId);
            }
        }
        if (slot >= 0) litUntil[slot] = nowNanos + TAP_VISIBLE_NANOS;
    }

    void virtualPress(int index, long nowNanos) {
        if (index >= 0 && index < KEY_COUNT) {
            litUntil[index] = nowNanos + TAP_VISIBLE_NANOS;
        }
    }

    void deviceRemoved(int deviceId) {
        heldByDevice.remove(deviceId);
    }

    void releaseAll() {
        heldByDevice.clear();
        java.util.Arrays.fill(litUntil, 0L);
    }

    boolean held(VirtualKey key) {
        for (EnumSet<VirtualKey> device : heldByDevice.values()) {
            if (device.contains(key)) return true;
        }
        return false;
    }

    boolean illuminated(int index, long nowNanos) {
        if (index < 0 || index >= KEY_COUNT) return false;
        if (nowNanos < litUntil[index]) return true;
        for (EnumSet<VirtualKey> device : heldByDevice.values()) {
            for (VirtualKey key : device) {
                if (keyIndex(key) == index) return true;
            }
        }
        return false;
    }

    boolean updateVisualState(long nowNanos) {
        boolean changed = false;
        for (int index = 0; index < KEY_COUNT; index++) {
            boolean visible = illuminated(index, nowNanos);
            if (lastIlluminated[index] != visible) {
                lastIlluminated[index] = visible;
                changed = true;
            }
        }
        return changed;
    }

    /** Mapping is intentionally by logical key, never by typed character:
     * physical shift/caps or user locale must not change the keycap touched. */
    static int keyIndex(VirtualKey key) {
        if (key == null) return -1;
        switch (key) {
            case DIGIT_1: return 0;
            case DIGIT_2: return 1;
            case DIGIT_3: return 2;
            case DIGIT_4: return 3;
            case DIGIT_5: return 4;
            case DIGIT_6: return 5;
            case DIGIT_7: return 6;
            case DIGIT_8: return 7;
            case DIGIT_9: return 8;
            case DIGIT_0: return 9;
            case Q: return 10;
            case W: return 11;
            case E: return 12;
            case R: return 13;
            case T: return 14;
            case Y: return 15;
            case U: return 16;
            case I: return 17;
            case O: return 18;
            case P: return 19;
            case A: return 20;
            case S: return 21;
            case D: return 22;
            case F: return 23;
            case G: return 24;
            case H: return 25;
            case J: return 26;
            case K: return 27;
            case L: return 28;
            case SEMICOLON: return 29;
            case SHIFT: return 30;
            case Z: return 31;
            case X: return 32;
            case C: return 33;
            case V: return 34;
            case B: return 35;
            case N: return 36;
            case M: return 37;
            case COMMA: return 38;
            case PERIOD: return 39;
            case CAPS_LOCK: return 40;
            case SPACE: return 42;
            case TAB: return 43;
            case LEFT: return 44;
            case RIGHT: return 45;
            case BACKSPACE: return 46;
            case DELETE: return 47;
            case ENTER: return 48;
            default: return -1;
        }
    }
}
