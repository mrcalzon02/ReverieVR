package io.github.mrcalzon02.reverievr;

final class VirtualInputBus {
    static final int MAX_JOYSTICK_BUTTONS = 16;

    private final boolean[] keys =
        new boolean[VirtualKey.values().length];
    private final boolean[] mouseButtons = new boolean[3];
    private final boolean[] joystickButtons =
        new boolean[MAX_JOYSTICK_BUTTONS];

    private float mouseRelativeX;
    private float mouseRelativeY;
    private float mouseAbsoluteX = 0.5f;
    private float mouseAbsoluteY = 0.5f;
    private float joystickAxisX;
    private float joystickAxisY;
    private long generation;

    synchronized void applyDigital(
        VirtualOutput output,
        boolean down
    ) {
        if (output == null) {
            return;
        }

        switch (output.kind) {
            case KEY:
                if (output.code >= 0 && output.code < keys.length) {
                    keys[output.code] = down;
                    generation++;
                }
                break;

            case MOUSE_BUTTON:
                if (output.code >= 0
                    && output.code < mouseButtons.length) {
                    mouseButtons[output.code] = down;
                    generation++;
                }
                break;

            case JOYSTICK_BUTTON:
                if (output.code >= 0
                    && output.code < joystickButtons.length) {
                    joystickButtons[output.code] = down;
                    generation++;
                }
                break;

            default:
                break;
        }
    }

    synchronized void applyAnalog(
        VirtualOutput output,
        float value
    ) {
        if (output == null) {
            return;
        }

        switch (output.kind) {
            case MOUSE_REL_X:
                mouseRelativeX += value;
                generation++;
                break;

            case MOUSE_REL_Y:
                mouseRelativeY += value;
                generation++;
                break;

            case MOUSE_ABS_X:
                mouseAbsoluteX = clamp01((value + 1.0f) * 0.5f);
                generation++;
                break;

            case MOUSE_ABS_Y:
                mouseAbsoluteY = clamp01((value + 1.0f) * 0.5f);
                generation++;
                break;

            case JOYSTICK_AXIS_X:
                joystickAxisX = clampAxis(value);
                generation++;
                break;

            case JOYSTICK_AXIS_Y:
                joystickAxisY = clampAxis(value);
                generation++;
                break;

            default:
                break;
        }
    }

    synchronized boolean isKeyDown(VirtualKey key) {
        return key != null && keys[key.ordinal()];
    }

    synchronized boolean isMouseButtonDown(int button) {
        return button >= 0
            && button < mouseButtons.length
            && mouseButtons[button];
    }

    synchronized boolean isJoystickButtonDown(int button) {
        return button >= 0
            && button < joystickButtons.length
            && joystickButtons[button];
    }

    synchronized float consumeMouseRelativeX() {
        float value = mouseRelativeX;
        mouseRelativeX = 0.0f;
        return value;
    }

    synchronized float consumeMouseRelativeY() {
        float value = mouseRelativeY;
        mouseRelativeY = 0.0f;
        return value;
    }

    synchronized float getMouseAbsoluteX() {
        return mouseAbsoluteX;
    }

    synchronized float getMouseAbsoluteY() {
        return mouseAbsoluteY;
    }

    synchronized float getJoystickAxisX() {
        return joystickAxisX;
    }

    synchronized float getJoystickAxisY() {
        return joystickAxisY;
    }

    synchronized long getGeneration() {
        return generation;
    }

    synchronized void releaseAll() {
        for (int index = 0; index < keys.length; index++) {
            keys[index] = false;
        }
        for (int index = 0; index < mouseButtons.length; index++) {
            mouseButtons[index] = false;
        }
        for (int index = 0; index < joystickButtons.length; index++) {
            joystickButtons[index] = false;
        }

        mouseRelativeX = 0.0f;
        mouseRelativeY = 0.0f;
        joystickAxisX = 0.0f;
        joystickAxisY = 0.0f;
        generation++;
    }

    private static float clamp01(float value) {
        return Math.max(0.0f, Math.min(1.0f, value));
    }

    private static float clampAxis(float value) {
        return Math.max(-1.0f, Math.min(1.0f, value));
    }
}
