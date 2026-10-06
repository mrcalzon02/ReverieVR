package io.github.mrcalzon02.reverievr;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

final class VrInputRouter {
    interface Listener {
        void onInputAction(VrInputAction action, String source);

        default void onPointerAxis(
            float horizontal,
            float vertical,
            String source
        ) {
            // Optional: only VR surfaces consume continuous pointer aim.
        }
    }

    interface BindingListener {
        void onBindingDigital(
            BindingInput input,
            boolean down,
            String source
        );

        void onBindingAxis(
            BindingInput input,
            float value,
            String source
        );
    }

    private static final int TOUCH_SWIPE_THRESHOLD = 48;
    private static final float AXIS_THRESHOLD = 0.65f;

    private final Listener listener;
    private BindingListener bindingListener;

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

    void setBindingListener(BindingListener bindingListener) {
        this.bindingListener = bindingListener;
    }

    void onControllerSnapshot(
        ControllerSnapshot snapshot,
        String source
    ) {
        if (snapshot == null) {
            return;
        }

        if (ReverieLog.isDevelopment()) {
            ReverieLog.dev(
                "INPUT_RAW",
                "controller source="
                    + safeSource(source)
                    + " "
                    + snapshot.toDiagnosticString()
            );
        }

        emitBindingDigitalEdge(
            BindingInput.SELECT,
            snapshot.touchpadPressed,
            previousClick,
            source
        );
        // The Daydream App/Menu button is shell-owned. Do not leak it into
        // hosted content as Back before the modal shell menu can intercept it.
        emitBindingDigitalEdge(
            BindingInput.RECENTER,
            snapshot.homePressed,
            previousHome,
            source
        );
        emitBindingDigitalEdge(
            BindingInput.VOLUME_UP,
            snapshot.volumeUpPressed,
            previousVolumeUp,
            source
        );
        emitBindingDigitalEdge(
            BindingInput.VOLUME_DOWN,
            snapshot.volumeDownPressed,
            previousVolumeDown,
            source
        );

        emitPressedEdge(
            snapshot.touchpadPressed,
            previousClick,
            VrInputAction.SELECT,
            source
        );
        emitPressedEdge(
            snapshot.menuPressed,
            previousMenu,
            VrInputAction.MENU,
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

            emitBindingAxis(
                BindingInput.TOUCHPAD_X,
                centerTouch(snapshot.touchX),
                source
            );
            emitBindingAxis(
                BindingInput.TOUCHPAD_Y,
                centerTouch(snapshot.touchY),
                source
            );
        } else if (previousTouching) {
            emitBindingAxis(
                BindingInput.TOUCHPAD_X,
                0.0f,
                source
            );
            emitBindingAxis(
                BindingInput.TOUCHPAD_Y,
                0.0f,
                source
            );

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
        if (event == null
            || !AndroidGamepadSupport.isGamepadEvent(event)) {
            return false;
        }

        VrInputAction action =
            mapGamepadKey(event.getKeyCode());
        BindingInput binding =
            mapGamepadBindingKey(event.getKeyCode());

        if (action == null && binding == null) {
            return false;
        }

        String source = gamepadSource(event);

        if (ReverieLog.isDevelopment()) {
            ReverieLog.dev(
                "INPUT_RAW",
                "gamepad-key source="
                    + source
                    + " action="
                    + event.getAction()
                    + " keyCode="
                    + event.getKeyCode()
                    + " repeat="
                    + event.getRepeatCount()
            );
        }

        if (binding != null) {
            if (event.getAction() == KeyEvent.ACTION_DOWN
                && event.getRepeatCount() == 0) {
                emitBindingDigital(
                    binding,
                    true,
                    source
                );
            } else if (event.getAction() == KeyEvent.ACTION_UP) {
                emitBindingDigital(
                    binding,
                    false,
                    source
                );
            }
        }

        if (action != null
            && event.getAction() == KeyEvent.ACTION_DOWN
            && event.getRepeatCount() == 0) {
            emit(action, source);
        }

        return true;
    }

    boolean onGenericMotionEvent(MotionEvent event) {
        if (event == null
            || !AndroidGamepadSupport.isGamepadEvent(event)) {
            return false;
        }

        int source = event.getSource();
        if ((source & InputDevice.SOURCE_JOYSTICK)
            != InputDevice.SOURCE_JOYSTICK) {
            return false;
        }

        float horizontal =
            event.getAxisValue(MotionEvent.AXIS_HAT_X);
        float vertical =
            event.getAxisValue(MotionEvent.AXIS_HAT_Y);

        if (Math.abs(horizontal) < 0.1f) {
            horizontal =
                event.getAxisValue(MotionEvent.AXIS_X);
        }
        if (Math.abs(vertical) < 0.1f) {
            vertical =
                event.getAxisValue(MotionEvent.AXIS_Y);
        }

        float rightX =
            event.getAxisValue(MotionEvent.AXIS_Z);
        float rightY =
            event.getAxisValue(MotionEvent.AXIS_RZ);

        String sourceName = gamepadSource(event);

        if (ReverieLog.isDevelopment()) {
            ReverieLog.dev(
                "INPUT_RAW",
                "gamepad-axis source="
                    + sourceName
                    + " x="
                    + horizontal
                    + " y="
                    + vertical
                    + " rx="
                    + rightX
                    + " ry="
                    + rightY
            );
        }

        emitBindingAxis(
            BindingInput.GAMEPAD_X,
            horizontal,
            sourceName
        );
        emitBindingAxis(
            BindingInput.GAMEPAD_Y,
            vertical,
            sourceName
        );
        emitBindingAxis(
            BindingInput.GAMEPAD_RX,
            rightX,
            sourceName
        );
        emitBindingAxis(
            BindingInput.GAMEPAD_RY,
            rightY,
            sourceName
        );

        if (listener != null) {
            listener.onPointerAxis(
                clampAxis(rightX),
                clampAxis(rightY),
                sourceName
            );
        }

        int newHorizontal = axisState(horizontal);
        int newVertical = axisState(vertical);

        if (newHorizontal != horizontalAxisState
            && newHorizontal != 0) {
            emit(
                newHorizontal < 0
                    ? VrInputAction.NAV_LEFT
                    : VrInputAction.NAV_RIGHT,
                sourceName
            );
        }

        if (newVertical != verticalAxisState
            && newVertical != 0) {
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

    void submitAction(
        VrInputAction action,
        String source
    ) {
        if (action == null) {
            return;
        }

        BindingInput binding = bindingForAction(action);
        if (binding != null) {
            emitBindingDigital(binding, true, source);
            emitBindingDigital(binding, false, source);
        }
        emit(action, source);
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

    private void emitBindingDigitalEdge(
        BindingInput input,
        boolean current,
        boolean previous,
        String source
    ) {
        if (current != previous) {
            emitBindingDigital(
                input,
                current,
                source
            );
        }
    }

    private void emitBindingDigital(
        BindingInput input,
        boolean down,
        String source
    ) {
        if (ReverieLog.isDevelopment()) {
            ReverieLog.dev(
                "INPUT_NORMALIZED",
                "digital input="
                    + input
                    + " down="
                    + down
                    + " source="
                    + safeSource(source)
            );
        }

        BindingListener target = bindingListener;
        if (target != null) {
            target.onBindingDigital(
                input,
                down,
                safeSource(source)
            );
        }
    }

    private void emitBindingAxis(
        BindingInput input,
        float value,
        String source
    ) {
        if (ReverieLog.isDevelopment()) {
            ReverieLog.dev(
                "INPUT_NORMALIZED",
                "axis input="
                    + input
                    + " value="
                    + value
                    + " source="
                    + safeSource(source)
            );
        }

        BindingListener target = bindingListener;
        if (target != null) {
            target.onBindingAxis(
                input,
                clampAxis(value),
                safeSource(source)
            );
        }
    }

    private void emit(
        VrInputAction action,
        String source
    ) {
        if (ReverieLog.isDevelopment()) {
            ReverieLog.dev(
                "INPUT_ACTION",
                "action="
                    + action
                    + " source="
                    + safeSource(source)
            );
        }

        if (listener != null) {
            listener.onInputAction(
                action,
                safeSource(source)
            );
        }
    }

    private static float centerTouch(int value) {
        float safe =
            Math.max(0.0f, Math.min(255.0f, value));
        return (safe / 127.5f) - 1.0f;
    }

    private static float clampAxis(float value) {
        return Math.max(-1.0f, Math.min(1.0f, value));
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

    private static BindingInput bindingForAction(
        VrInputAction action
    ) {
        switch (action) {
            case SELECT:
                return BindingInput.SELECT;
            case BACK:
                return BindingInput.BACK;
            case RECENTER:
                return BindingInput.RECENTER;
            case NAV_LEFT:
                return BindingInput.NAV_LEFT;
            case NAV_RIGHT:
                return BindingInput.NAV_RIGHT;
            case NAV_UP:
                return BindingInput.NAV_UP;
            case NAV_DOWN:
                return BindingInput.NAV_DOWN;
            case VOLUME_UP:
                return BindingInput.VOLUME_UP;
            case VOLUME_DOWN:
                return BindingInput.VOLUME_DOWN;
            default:
                return null;
        }
    }

    private static BindingInput mapGamepadBindingKey(
        int keyCode
    ) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A:
            case KeyEvent.KEYCODE_DPAD_CENTER:
            case KeyEvent.KEYCODE_ENTER:
                return BindingInput.SELECT;

            case KeyEvent.KEYCODE_BUTTON_B:
            case KeyEvent.KEYCODE_BACK:
                return BindingInput.BACK;

            case KeyEvent.KEYCODE_BUTTON_START:
            case KeyEvent.KEYCODE_BUTTON_MODE:
            case KeyEvent.KEYCODE_MENU:
                return null;

            case KeyEvent.KEYCODE_DPAD_LEFT:
            case KeyEvent.KEYCODE_BUTTON_L1:
                return BindingInput.NAV_LEFT;

            case KeyEvent.KEYCODE_DPAD_RIGHT:
            case KeyEvent.KEYCODE_BUTTON_R1:
                return BindingInput.NAV_RIGHT;

            case KeyEvent.KEYCODE_DPAD_UP:
                return BindingInput.NAV_UP;

            case KeyEvent.KEYCODE_DPAD_DOWN:
                return BindingInput.NAV_DOWN;

            case KeyEvent.KEYCODE_VOLUME_UP:
                return BindingInput.VOLUME_UP;

            case KeyEvent.KEYCODE_VOLUME_DOWN:
                return BindingInput.VOLUME_DOWN;

            default:
                return null;
        }
    }

    private static VrInputAction mapGamepadKey(
        int keyCode
    ) {
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
            case KeyEvent.KEYCODE_MENU:
                return VrInputAction.MENU;

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

    private static String safeSource(String source) {
        return source == null || source.trim().isEmpty()
            ? "controller"
            : source;
    }

    private static String gamepadSource(
        android.view.InputEvent event
    ) {
        InputDevice device =
            event == null ? null : event.getDevice();

        if (device == null) {
            return "Android gamepad";
        }

        String name = device.getName();
        return name == null || name.trim().isEmpty()
            ? "Android gamepad"
            : name;
    }
}
