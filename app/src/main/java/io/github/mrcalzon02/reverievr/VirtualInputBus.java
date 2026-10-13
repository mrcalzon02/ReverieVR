package io.github.mrcalzon02.reverievr;

final class VirtualInputBus {
    static final int MAX_JOYSTICK_BUTTONS = 16;

    private final boolean[] keys =
        new boolean[VirtualKey.values().length];
    private final boolean[] mouseButtons = new boolean[3];
    private final boolean[] joystickButtons =
        new boolean[MAX_JOYSTICK_BUTTONS];

    // Press/release can happen entirely between two DOS/native poll frames.
    // Retain the unseen rising edge for one capture without changing held
    // state, so a brief click, key tap or joystick activation is not lost.
    private final boolean[] pendingKeyPresses = new boolean[keys.length];
    private final boolean[] pendingMousePresses =
        new boolean[mouseButtons.length];
    private final boolean[] pendingJoystickPresses =
        new boolean[joystickButtons.length];

    private float mouseRelativeX;
    private float mouseRelativeY;
    private float mouseAbsoluteX = 0.5f;
    private float mouseAbsoluteY = 0.5f;
    private float mouseWheel;
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
                    if (down && !keys[output.code]) {
                        pendingKeyPresses[output.code] = true;
                    }
                    keys[output.code] = down;
                    generation++;
                }
                break;

            case MOUSE_BUTTON:
                if (output.code >= 0
                    && output.code < mouseButtons.length) {
                    if (down && !mouseButtons[output.code]) {
                        pendingMousePresses[output.code] = true;
                    }
                    mouseButtons[output.code] = down;
                    generation++;
                }
                break;

            case JOYSTICK_BUTTON:
                if (output.code >= 0
                    && output.code < joystickButtons.length) {
                    if (down && !joystickButtons[output.code]) {
                        pendingJoystickPresses[output.code] = true;
                    }
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

    synchronized void applyMouseWheel(float delta) {
        if (delta == 0.0f) {
            return;
        }
        mouseWheel += delta;
        generation++;
    }

    synchronized void capture(
        VirtualInputSnapshot target
    ) {
        if (target == null) {
            return;
        }

        for (int i = 0; i < keys.length; i++) {
            target.keys[i] = keys[i] || pendingKeyPresses[i];
            pendingKeyPresses[i] = false;
        }
        for (int i = 0; i < mouseButtons.length; i++) {
            target.mouseButtons[i] = mouseButtons[i]
                || pendingMousePresses[i];
            pendingMousePresses[i] = false;
        }
        for (int i = 0; i < joystickButtons.length; i++) {
            target.joystickButtons[i] = joystickButtons[i]
                || pendingJoystickPresses[i];
            pendingJoystickPresses[i] = false;
        }

        target.mouseRelativeX = mouseRelativeX;
        target.mouseRelativeY = mouseRelativeY;
        target.mouseAbsoluteX = mouseAbsoluteX;
        target.mouseAbsoluteY = mouseAbsoluteY;
        target.mouseWheel = mouseWheel;
        target.joystickAxisX = joystickAxisX;
        target.joystickAxisY = joystickAxisY;
        target.generation = generation;

        mouseRelativeX = 0.0f;
        mouseRelativeY = 0.0f;
        mouseWheel = 0.0f;
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

    synchronized float consumeMouseWheel() {
        float value = mouseWheel;
        mouseWheel = 0.0f;
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
            pendingKeyPresses[index] = false;
        }
        for (int index = 0; index < mouseButtons.length; index++) {
            mouseButtons[index] = false;
            pendingMousePresses[index] = false;
        }
        for (int index = 0; index < joystickButtons.length; index++) {
            joystickButtons[index] = false;
            pendingJoystickPresses[index] = false;
        }

        mouseRelativeX = 0.0f;
        mouseRelativeY = 0.0f;
        mouseWheel = 0.0f;
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
