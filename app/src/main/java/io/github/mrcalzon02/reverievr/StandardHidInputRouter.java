package io.github.mrcalzon02.reverievr;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

final class StandardHidInputRouter {
    interface Listener {
        void onVirtualKey(
            VirtualKey key,
            boolean down,
            String source
        );

        void onVirtualMouseButton(
            int button,
            boolean down,
            String source
        );

        void onVirtualMouseRelative(
            float deltaX,
            float deltaY,
            String source
        );

        void onVirtualMouseAbsolute(
            float normalizedX,
            float normalizedY,
            String source
        );

        void onVirtualMouseWheel(
            float delta,
            String source
        );
    }

    private final Listener listener;

    private int previousMouseButtons;
    private boolean mousePositionInitialized;
    private float previousMouseX;
    private float previousMouseY;

    StandardHidInputRouter(Listener listener) {
        this.listener = listener;
    }

    boolean onKeyboardEvent(KeyEvent event) {
        if (event == null || isGamepad(event)) {
            return false;
        }

        int source = event.getSource();
        if ((source & InputDevice.SOURCE_KEYBOARD)
            != InputDevice.SOURCE_KEYBOARD) {
            return false;
        }

        VirtualKey key = mapKey(event.getKeyCode());
        if (key == null) {
            return false;
        }

        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            if (event.getRepeatCount() == 0) {
                emitKey(key, true, sourceName(event));
            }
            return true;
        }

        if (event.getAction() == KeyEvent.ACTION_UP) {
            emitKey(key, false, sourceName(event));
            return true;
        }

        return false;
    }

    boolean onMouseEvent(
        MotionEvent event,
        int viewportWidth,
        int viewportHeight
    ) {
        if (event == null) {
            return false;
        }

        int source = event.getSource();
        boolean mouse =
            (source & InputDevice.SOURCE_MOUSE)
                == InputDevice.SOURCE_MOUSE
            || (source & InputDevice.SOURCE_MOUSE_RELATIVE)
                == InputDevice.SOURCE_MOUSE_RELATIVE;
        if (!mouse) {
            return false;
        }

        String sourceName = sourceName(event);
        emitMouseButtons(event.getButtonState(), sourceName);

        float relativeX =
            event.getAxisValue(MotionEvent.AXIS_RELATIVE_X);
        float relativeY =
            event.getAxisValue(MotionEvent.AXIS_RELATIVE_Y);

        float currentX = event.getX();
        float currentY = event.getY();

        if (relativeX == 0.0f
            && relativeY == 0.0f
            && mousePositionInitialized) {
            relativeX = currentX - previousMouseX;
            relativeY = currentY - previousMouseY;
        }

        previousMouseX = currentX;
        previousMouseY = currentY;
        mousePositionInitialized = true;

        if (relativeX != 0.0f || relativeY != 0.0f) {
            Listener target = listener;
            if (target != null) {
                target.onVirtualMouseRelative(
                    relativeX,
                    relativeY,
                    sourceName
                );
            }
        }

        if (viewportWidth > 0 && viewportHeight > 0) {
            float normalizedX =
                clamp01(currentX / viewportWidth);
            float normalizedY =
                clamp01(currentY / viewportHeight);

            Listener target = listener;
            if (target != null) {
                target.onVirtualMouseAbsolute(
                    normalizedX,
                    normalizedY,
                    sourceName
                );
            }
        }

        float wheel =
            event.getAxisValue(MotionEvent.AXIS_VSCROLL);
        if (wheel != 0.0f) {
            Listener target = listener;
            if (target != null) {
                target.onVirtualMouseWheel(
                    wheel,
                    sourceName
                );
            }
        }

        return true;
    }

    void reset() {
        if (previousMouseButtons != 0) {
            emitMouseButtons(0, "mouse disconnected");
        }
        mousePositionInitialized = false;
        previousMouseX = 0.0f;
        previousMouseY = 0.0f;
    }

    private void emitMouseButtons(
        int buttonState,
        String source
    ) {
        emitMouseButtonTransition(
            MotionEvent.BUTTON_PRIMARY,
            VirtualOutput.MOUSE_LEFT,
            buttonState,
            source
        );
        emitMouseButtonTransition(
            MotionEvent.BUTTON_SECONDARY,
            VirtualOutput.MOUSE_RIGHT,
            buttonState,
            source
        );
        emitMouseButtonTransition(
            MotionEvent.BUTTON_TERTIARY,
            VirtualOutput.MOUSE_MIDDLE,
            buttonState,
            source
        );

        previousMouseButtons = buttonState;
    }

    private void emitMouseButtonTransition(
        int androidMask,
        int virtualButton,
        int currentState,
        String source
    ) {
        boolean previous =
            (previousMouseButtons & androidMask) != 0;
        boolean current =
            (currentState & androidMask) != 0;

        if (previous == current) {
            return;
        }

        Listener target = listener;
        if (target != null) {
            target.onVirtualMouseButton(
                virtualButton,
                current,
                source
            );
        }
    }

    private void emitKey(
        VirtualKey key,
        boolean down,
        String source
    ) {
        Listener target = listener;
        if (target != null) {
            target.onVirtualKey(
                key,
                down,
                source
            );
        }
    }

    private static VirtualKey mapKey(int keyCode) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_ESCAPE:
                return VirtualKey.ESCAPE;
            case KeyEvent.KEYCODE_TAB:
                return VirtualKey.TAB;
            case KeyEvent.KEYCODE_ENTER:
            case KeyEvent.KEYCODE_NUMPAD_ENTER:
                return VirtualKey.ENTER;
            case KeyEvent.KEYCODE_SPACE:
                return VirtualKey.SPACE;
            case KeyEvent.KEYCODE_CTRL_LEFT:
            case KeyEvent.KEYCODE_CTRL_RIGHT:
                return VirtualKey.CTRL;
            case KeyEvent.KEYCODE_ALT_LEFT:
            case KeyEvent.KEYCODE_ALT_RIGHT:
                return VirtualKey.ALT;
            case KeyEvent.KEYCODE_SHIFT_LEFT:
            case KeyEvent.KEYCODE_SHIFT_RIGHT:
                return VirtualKey.SHIFT;

            case KeyEvent.KEYCODE_DPAD_UP:
                return VirtualKey.UP;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                return VirtualKey.DOWN;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                return VirtualKey.LEFT;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                return VirtualKey.RIGHT;

            case KeyEvent.KEYCODE_A: return VirtualKey.A;
            case KeyEvent.KEYCODE_B: return VirtualKey.B;
            case KeyEvent.KEYCODE_C: return VirtualKey.C;
            case KeyEvent.KEYCODE_D: return VirtualKey.D;
            case KeyEvent.KEYCODE_E: return VirtualKey.E;
            case KeyEvent.KEYCODE_F: return VirtualKey.F;
            case KeyEvent.KEYCODE_G: return VirtualKey.G;
            case KeyEvent.KEYCODE_H: return VirtualKey.H;
            case KeyEvent.KEYCODE_I: return VirtualKey.I;
            case KeyEvent.KEYCODE_J: return VirtualKey.J;
            case KeyEvent.KEYCODE_K: return VirtualKey.K;
            case KeyEvent.KEYCODE_L: return VirtualKey.L;
            case KeyEvent.KEYCODE_M: return VirtualKey.M;
            case KeyEvent.KEYCODE_N: return VirtualKey.N;
            case KeyEvent.KEYCODE_O: return VirtualKey.O;
            case KeyEvent.KEYCODE_P: return VirtualKey.P;
            case KeyEvent.KEYCODE_Q: return VirtualKey.Q;
            case KeyEvent.KEYCODE_R: return VirtualKey.R;
            case KeyEvent.KEYCODE_S: return VirtualKey.S;
            case KeyEvent.KEYCODE_T: return VirtualKey.T;
            case KeyEvent.KEYCODE_U: return VirtualKey.U;
            case KeyEvent.KEYCODE_V: return VirtualKey.V;
            case KeyEvent.KEYCODE_W: return VirtualKey.W;
            case KeyEvent.KEYCODE_X: return VirtualKey.X;
            case KeyEvent.KEYCODE_Y: return VirtualKey.Y;
            case KeyEvent.KEYCODE_Z: return VirtualKey.Z;

            case KeyEvent.KEYCODE_0: return VirtualKey.DIGIT_0;
            case KeyEvent.KEYCODE_1: return VirtualKey.DIGIT_1;
            case KeyEvent.KEYCODE_2: return VirtualKey.DIGIT_2;
            case KeyEvent.KEYCODE_3: return VirtualKey.DIGIT_3;
            case KeyEvent.KEYCODE_4: return VirtualKey.DIGIT_4;
            case KeyEvent.KEYCODE_5: return VirtualKey.DIGIT_5;
            case KeyEvent.KEYCODE_6: return VirtualKey.DIGIT_6;
            case KeyEvent.KEYCODE_7: return VirtualKey.DIGIT_7;
            case KeyEvent.KEYCODE_8: return VirtualKey.DIGIT_8;
            case KeyEvent.KEYCODE_9: return VirtualKey.DIGIT_9;

            case KeyEvent.KEYCODE_INSERT:
                return VirtualKey.INSERT;
            case KeyEvent.KEYCODE_FORWARD_DEL:
                return VirtualKey.DELETE;
            case KeyEvent.KEYCODE_MOVE_HOME:
                return VirtualKey.HOME;
            case KeyEvent.KEYCODE_MOVE_END:
                return VirtualKey.END;
            case KeyEvent.KEYCODE_PAGE_UP:
                return VirtualKey.PAGE_UP;
            case KeyEvent.KEYCODE_PAGE_DOWN:
                return VirtualKey.PAGE_DOWN;

            case KeyEvent.KEYCODE_F1: return VirtualKey.F1;
            case KeyEvent.KEYCODE_F2: return VirtualKey.F2;
            case KeyEvent.KEYCODE_F3: return VirtualKey.F3;
            case KeyEvent.KEYCODE_F4: return VirtualKey.F4;
            case KeyEvent.KEYCODE_F5: return VirtualKey.F5;
            case KeyEvent.KEYCODE_F6: return VirtualKey.F6;
            case KeyEvent.KEYCODE_F7: return VirtualKey.F7;
            case KeyEvent.KEYCODE_F8: return VirtualKey.F8;
            case KeyEvent.KEYCODE_F9: return VirtualKey.F9;
            case KeyEvent.KEYCODE_F10: return VirtualKey.F10;
            case KeyEvent.KEYCODE_F11: return VirtualKey.F11;
            case KeyEvent.KEYCODE_F12: return VirtualKey.F12;

            default:
                return null;
        }
    }

    private static boolean isGamepad(KeyEvent event) {
        int source = event.getSource();
        return (source & InputDevice.SOURCE_GAMEPAD)
                == InputDevice.SOURCE_GAMEPAD
            || (source & InputDevice.SOURCE_JOYSTICK)
                == InputDevice.SOURCE_JOYSTICK;
    }

    private static String sourceName(
        android.view.InputEvent event
    ) {
        InputDevice device =
            event == null ? null : event.getDevice();

        if (device == null) {
            return "Android HID";
        }

        String name = device.getName();
        return name == null || name.trim().isEmpty()
            ? "Android HID"
            : name;
    }

    private static float clamp01(float value) {
        return Math.max(0.0f, Math.min(1.0f, value));
    }
}
