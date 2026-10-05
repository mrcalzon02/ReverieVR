package io.github.mrcalzon02.reverievr;

import java.util.Objects;

final class VirtualOutput {
    static final int MOUSE_LEFT = 0;
    static final int MOUSE_RIGHT = 1;
    static final int MOUSE_MIDDLE = 2;

    final VirtualOutputKind kind;
    final int code;

    private VirtualOutput(VirtualOutputKind kind, int code) {
        this.kind = Objects.requireNonNull(kind, "kind");
        this.code = code;
    }

    static VirtualOutput key(VirtualKey key) {
        return new VirtualOutput(
            VirtualOutputKind.KEY,
            Objects.requireNonNull(key, "key").ordinal()
        );
    }

    static VirtualOutput mouseButton(int button) {
        if (button < MOUSE_LEFT || button > MOUSE_MIDDLE) {
            throw new IllegalArgumentException("Unsupported mouse button: " + button);
        }
        return new VirtualOutput(VirtualOutputKind.MOUSE_BUTTON, button);
    }

    static VirtualOutput mouseRelativeX() {
        return new VirtualOutput(VirtualOutputKind.MOUSE_REL_X, 0);
    }

    static VirtualOutput mouseRelativeY() {
        return new VirtualOutput(VirtualOutputKind.MOUSE_REL_Y, 0);
    }

    static VirtualOutput mouseAbsoluteX() {
        return new VirtualOutput(VirtualOutputKind.MOUSE_ABS_X, 0);
    }

    static VirtualOutput mouseAbsoluteY() {
        return new VirtualOutput(VirtualOutputKind.MOUSE_ABS_Y, 0);
    }

    static VirtualOutput joystickAxisX() {
        return new VirtualOutput(VirtualOutputKind.JOYSTICK_AXIS_X, 0);
    }

    static VirtualOutput joystickAxisY() {
        return new VirtualOutput(VirtualOutputKind.JOYSTICK_AXIS_Y, 0);
    }

    static VirtualOutput joystickButton(int button) {
        if (button < 0 || button >= VirtualInputBus.MAX_JOYSTICK_BUTTONS) {
            throw new IllegalArgumentException("Unsupported joystick button: " + button);
        }
        return new VirtualOutput(VirtualOutputKind.JOYSTICK_BUTTON, button);
    }

    VirtualKey key() {
        if (kind != VirtualOutputKind.KEY
            || code < 0
            || code >= VirtualKey.values().length) {
            return null;
        }
        return VirtualKey.values()[code];
    }

    @Override
    public boolean equals(Object other) {
        if (this == other) {
            return true;
        }
        if (!(other instanceof VirtualOutput)) {
            return false;
        }
        VirtualOutput that = (VirtualOutput) other;
        return kind == that.kind && code == that.code;
    }

    @Override
    public int hashCode() {
        return (kind.ordinal() * 31) + code;
    }

    @Override
    public String toString() {
        if (kind == VirtualOutputKind.KEY) {
            VirtualKey key = key();
            return key == null ? "KEY(" + code + ")" : "KEY " + key.name();
        }
        return kind.name() + (code == 0 ? "" : " " + code);
    }
}
