package io.github.mrcalzon02.reverievr;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

final class VrInputRouter {
    interface Listener {
        void onInputAction(VrInputAction action, String source);
    }

    private static final int TOUCH_SWIPE_THRESHOLD = 48;
    private static final float AXIS_THRESHOLD = 0.65f;

    private final Listener listener;

    private boolean previousClick;
    private boolean previousMenu;
    private boolean previousHome;
    private boolean previousVolumeUp;
    private boolean previousVolumeDown;

    private boolean previousTouching;
    private int touchStartX;
    private int touchStartY;
    private int touchLastX;
    private int touchLastY;

    private int horizontalAxisState;
    private int verticalAxisState;

    VrInputRouter(Listener listener) {
        this.listener = listener;
    }

    void onControllerSnapshot(ControllerSnapshot snapshot, String source) {
        if (snapshot == null) {
            return;
        }

        emitPressedEdge(
            snapshot.touchpadPressed,
            previousClick,
            VrInputAction.SELECT,
            source
        );
        emitPressedEdge(
            snapshot.menuPressed,
            previousMenu,
            VrInputAction.BACK,
            source
        );
        emitPressedEdge(
            snapshot.homePressed,
            previousHome,
            VrInputAction.RECENTER,
            source
        );
        emitPressedEdge(
            snapshot.volumeUpPressed,
            previousVolumeUp,
            VrInputAction.VOLUME_UP,
            source
        );
        emitPressedEdge(
            snapshot.volumeDownPressed,
            previousVolumeDown,
            VrInputAction.VOLUME_DOWN,
            source
        );

        if (snapshot.touching) {
            if (!previousTouching) {
                touchStartX = snapshot.touchX;
                touchStartY = snapshot.touchY;
            }
            touchLastX = snapshot.touchX;
            touchLastY = snapshot.touchY;
        } else if (previousTouching) {
            int deltaX = touchLastX - touchStartX;
            int deltaY = touchLastY - touchStartY;

            if (Math.abs(deltaX) >= TOUCH_SWIPE_THRESHOLD
                && Math.abs(deltaX) > Math.abs(deltaY)) {
                emit(
                    deltaX > 0
                        ? VrInputAction.NAV_RIGHT
                        : VrInputAction.NAV_LEFT,
                    source
                );
            } else if (Math.abs(deltaY) >= TOUCH_SWIPE_THRESHOLD) {
                emit(
                    deltaY > 0
                        ? VrInputAction.NAV_DOWN
                        : VrInputAction.NAV_UP,
                    source
                );
            }
        }

        previousClick = snapshot.touchpadPressed;
        previousMenu = snapshot.menuPressed;
        previousHome = snapshot.homePressed;
        previousVolumeUp = snapshot.volumeUpPressed;
        previousVolumeDown = snapshot.volumeDownPressed;
        previousTouching = snapshot.touching;
    }

    boolean onKeyEvent(KeyEvent event) {
        if (event == null || !AndroidGamepadSupport.isGamepadEvent(event)) {
            return false;
        }

        if (event.getAction() != KeyEvent.ACTION_DOWN || event.getRepeatCount() != 0) {
            return isMappedGamepadKey(event.getKeyCode());
        }

        VrInputAction action = mapGamepadKey(event.getKeyCode());
        if (action == null) {
            return false;
        }

        emit(action, gamepadSource(event));
        return true;
    }

    boolean onGenericMotionEvent(MotionEvent event) {
        if (event == null || !AndroidGamepadSupport.isGamepadEvent(event)) {
            return false;
        }

        int source = event.getSource();
        if ((source & InputDevice.SOURCE_JOYSTICK) != InputDevice.SOURCE_JOYSTICK) {
            return false;
        }

        float horizontal = event.getAxisValue(MotionEvent.AXIS_HAT_X);
        float vertical = event.getAxisValue(MotionEvent.AXIS_HAT_Y);

        if (Math.abs(horizontal) < 0.1f) {
            horizontal = event.getAxisValue(MotionEvent.AXIS_X);
        }
        if (Math.abs(vertical) < 0.1f) {
            vertical = event.getAxisValue(MotionEvent.AXIS_Y);
        }

        int newHorizontal = axisState(horizontal);
        int newVertical = axisState(vertical);
        String sourceName = gamepadSource(event);

        if (newHorizontal != horizontalAxisState && newHorizontal != 0) {
            emit(
                newHorizontal < 0
                    ? VrInputAction.NAV_LEFT
                    : VrInputAction.NAV_RIGHT,
                sourceName
            );
        }

        if (newVertical != verticalAxisState && newVertical != 0) {
            emit(
                newVertical < 0
                    ? VrInputAction.NAV_UP
                    : VrInputAction.NAV_DOWN,
                sourceName
            );
        }

        horizontalAxisState = newHorizontal;
        verticalAxisState = newVertical;
        return true;
    }

    void submitAction(VrInputAction action, String source) {
        if (action != null) {
            emit(action, source);
        }
    }

    private void emitPressedEdge(
        boolean current,
        boolean previous,
        VrInputAction action,
        String source
    ) {
        if (current && !previous) {
            emit(action, source);
        }
    }

    private void emit(VrInputAction action, String source) {
        if (listener != null) {
            listener.onInputAction(
                action,
                source == null || source.trim().isEmpty()
                    ? "controller"
                    : source
            );
        }
    }

    private static int axisState(float value) {
        if (value <= -AXIS_THRESHOLD) {
            return -1;
        }
        if (value >= AXIS_THRESHOLD) {
            return 1;
        }
        return 0;
    }

    private static boolean isMappedGamepadKey(int keyCode) {
        return mapGamepadKey(keyCode) != null;
    }

    private static VrInputAction mapGamepadKey(int keyCode) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A:
            case KeyEvent.KEYCODE_DPAD_CENTER:
            case KeyEvent.KEYCODE_ENTER:
                return VrInputAction.SELECT;

            case KeyEvent.KEYCODE_BUTTON_B:
            case KeyEvent.KEYCODE_BACK:
                return VrInputAction.BACK;

            case KeyEvent.KEYCODE_BUTTON_START:
            case KeyEvent.KEYCODE_BUTTON_MODE:
                return VrInputAction.RECENTER;

            case KeyEvent.KEYCODE_DPAD_LEFT:
            case KeyEvent.KEYCODE_BUTTON_L1:
                return VrInputAction.NAV_LEFT;

            case KeyEvent.KEYCODE_DPAD_RIGHT:
            case KeyEvent.KEYCODE_BUTTON_R1:
                return VrInputAction.NAV_RIGHT;

            case KeyEvent.KEYCODE_DPAD_UP:
                return VrInputAction.NAV_UP;

            case KeyEvent.KEYCODE_DPAD_DOWN:
                return VrInputAction.NAV_DOWN;

            case KeyEvent.KEYCODE_VOLUME_UP:
                return VrInputAction.VOLUME_UP;

            case KeyEvent.KEYCODE_VOLUME_DOWN:
                return VrInputAction.VOLUME_DOWN;

            default:
                return null;
        }
    }

    private static String gamepadSource(android.view.InputEvent event) {
        InputDevice device = event == null ? null : event.getDevice();
        if (device == null) {
            return "Android gamepad";
        }

        String name = device.getName();
        return name == null || name.trim().isEmpty()
            ? "Android gamepad"
            : name;
    }
}
