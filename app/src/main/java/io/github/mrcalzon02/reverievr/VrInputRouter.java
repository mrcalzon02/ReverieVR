package io.github.mrcalzon02.reverievr;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

final class VrInputRouter {
    interface Listener {
        void onInputAction(VrInputAction action, String source);

        /** A hosted game owns the B face button; Android Back and Menu
         * remain shell-owned recovery keys. */
        default boolean shouldDispatchGamepadShellAction(
            int keyCode, VrInputAction action) {
            return true;
        }

        default boolean usesDaydreamDosGameControls() {
            return false;
        }

        /** Capture finger movement for a scrollable VR DOS library only. */
        default boolean usesDosLibraryTouchScroll() {
            return false;
        }

        /** An opened DOS shell menu owns pad Up/Down rather than the guest. */
        default boolean usesDosShellMenuNavigation() {
            return false;
        }

        default void onDosLibraryTouchScroll(float deltaY, boolean ended) {
            // Existing clients without this shelf need no scroll callback.
        }

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

        default void onBindingRelative(BindingInput input, float delta,
                                       String source) { }

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
    private boolean libraryScrollTouch;
    private int dosMenuDirection;
    private long nextDosMenuRepeatNanos;
    private int touchStartX;
    private int touchStartY;
    private int touchLastX;
    private int touchLastY;

    private int horizontalAxisState;
    private int verticalAxisState;
    private final DaydreamGestureRecognizer gestures =
        new DaydreamGestureRecognizer();

    VrInputRouter(Listener listener) {
        this.listener = listener;
    }

    void setBindingListener(BindingListener bindingListener) {
        this.bindingListener = bindingListener;
    }

    /** Clear stale button and axis edges when changing primary hardware.
     * Guest-held keys are released separately by InputBindingEngine. */
    void resetControllerEdges() {
        previousClick = false;
        previousMenu = false;
        previousHome = false;
        previousVolumeUp = false;
        previousVolumeDown = false;
        previousTouching = false;
        libraryScrollTouch = false;
        dosMenuDirection = 0;
        nextDosMenuRepeatNanos = 0L;
        touchStartX = touchStartY = touchLastX = touchLastY = 0;
        horizontalAxisState = verticalAxisState = 0;
        gestures.reset((input, down) ->
            emitBindingDigital(input, down, "Daydream gesture reset"));
    }

    void onControllerSnapshot(
        ControllerSnapshot snapshot,
        String source
    ) {
        if (snapshot == null) {
            return;
        }

        // Only the original Daydream provider has this particular click/
        // acceleration gesture contract. Never silently assign synthesized
        // Daydream gestures to a Steam or generic controller.
        if (source != null && source.toLowerCase(
                java.util.Locale.ROOT).contains("daydream")) {
            gestures.accept(snapshot.receivedAtNanos, snapshot.touchpadPressed,
                snapshot.accelXG, snapshot.accelYG, snapshot.accelZG,
                (input, down) -> emitBindingDigital(input, down, source));
        } else {
            gestures.reset((input, down) ->
                emitBindingDigital(input, down, "Daydream gesture reset"));
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
        final boolean dosGame = listener != null
            && listener.usesDaydreamDosGameControls();
        final boolean dosShellMenu = listener != null
            && listener.usesDosShellMenuNavigation();
        emitBindingDigitalEdge(
            BindingInput.DAYDREAM_APP_BUTTON,
            snapshot.menuPressed,
            previousMenu,
            source
        );
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
        // App/minus becomes held DOS mouse button 2, not Shell Menu.
        if (!dosGame) {
            emitPressedEdge(snapshot.menuPressed, previousMenu,
                VrInputAction.MENU, source);
        }
        // Home/zero owns the universal Quick Menu during DOS play.
        emitPressedEdge(snapshot.homePressed, previousHome,
            daydreamHomeAction(dosGame), source);
        /*
         * The physical controller's volume rocker is a pair of hosted-input
         * buttons in ReverieVR. Do not also emit Android/system volume actions;
         * shell volume lives in interface menus instead.
         */

        if (snapshot.touching) {
            if (!previousTouching) {
                touchStartX = snapshot.touchX;
                touchStartY = snapshot.touchY;
                libraryScrollTouch = !dosGame && listener != null
                    && listener.usesDosLibraryTouchScroll();
            }

            // Relative finger motion drives DOSBox Pure's real mouse device.
            // Absolute pad coordinates remain available for arrow/axis profiles.
            if (previousTouching) {
                if (libraryScrollTouch) {
                    if (listener != null && !snapshot.touchpadPressed) {
                        listener.onDosLibraryTouchScroll(
                            snapshot.touchY - touchLastY, false);
                    }
                } else {
                    emitBindingRelative(BindingInput.TOUCHPAD_REL_X,
                        snapshot.touchX - touchLastX, source);
                    emitBindingRelative(BindingInput.TOUCHPAD_REL_Y,
                        snapshot.touchY - touchLastY, source);
                }
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

            // A library scroll never becomes an extra NAV action on lift.
            // DOS guest touchpad directions remain held keyboard keys.
            if (libraryScrollTouch) {
                if (listener != null) {
                    listener.onDosLibraryTouchScroll(0f, true);
                }
            } else if (!dosShellMenu && !dosGame
                && Math.abs(deltaX) >= TOUCH_SWIPE_THRESHOLD
                && Math.abs(deltaX) > Math.abs(deltaY)) {
                emit(
                    deltaX > 0
                        ? VrInputAction.NAV_RIGHT
                        : VrInputAction.NAV_LEFT,
                    source
                );
            } else if (!dosShellMenu && !dosGame
                && Math.abs(deltaY) >= TOUCH_SWIPE_THRESHOLD) {
                emit(
                    deltaY > 0
                        ? VrInputAction.NAV_DOWN
                        : VrInputAction.NAV_UP,
                    source
                );
            }
        }

        routeDosShellMenuTouch(snapshot, dosShellMenu, source);

        previousClick = snapshot.touchpadPressed;
        previousMenu = snapshot.menuPressed;
        previousHome = snapshot.homePressed;
        previousVolumeUp = snapshot.volumeUpPressed;
        previousVolumeDown = snapshot.volumeDownPressed;
        previousTouching = snapshot.touching;
        if (!snapshot.touching) libraryScrollTouch = false;
    }

    private void routeDosShellMenuTouch(
        ControllerSnapshot snapshot, boolean menuActive, String source) {
        if (!menuActive || !snapshot.touching || snapshot.touchpadPressed) {
            dosMenuDirection = 0;
            nextDosMenuRepeatNanos = 0L;
            return;
        }
        final float y = centerTouch(snapshot.touchY);
        final int direction = y <= -0.55f ? -1 : y >= 0.55f ? 1 : 0;
        if (direction == 0) {
            dosMenuDirection = 0;
            return;
        }
        final long now = snapshot.receivedAtNanos > 0L
            ? snapshot.receivedAtNanos : System.nanoTime();
        if (direction != dosMenuDirection
            || now >= nextDosMenuRepeatNanos) {
            emit(direction < 0 ? VrInputAction.NAV_UP
                : VrInputAction.NAV_DOWN, source);
            nextDosMenuRepeatNanos = now
                + (direction != dosMenuDirection
                    ? 350000000L : 170000000L);
        }
        dosMenuDirection = direction;
    }

    static VrInputAction daydreamHomeAction(boolean dosGame) {
        return dosGame ? VrInputAction.MENU : VrInputAction.RECENTER;
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
        BindingInput physical =
            mapPhysicalGamepadKey(event.getKeyCode());

        if (action == null && binding == null && physical == null) {
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

        if (physical != null) {
            if (event.getAction() == KeyEvent.ACTION_DOWN
                && event.getRepeatCount() == 0) {
                emitBindingDigital(physical, true, source);
            } else if (event.getAction() == KeyEvent.ACTION_UP) {
                emitBindingDigital(physical, false, source);
            }
        }

        if (action != null
            && event.getAction() == KeyEvent.ACTION_DOWN
            && event.getRepeatCount() == 0
            && (listener == null
                || listener.shouldDispatchGamepadShellAction(
                    event.getKeyCode(), action))) {
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
        // Android drivers may expose analog triggers via either axis name.
        // Keep them separate from the face-button shell action policy.
        float leftTrigger = Math.max(
            event.getAxisValue(MotionEvent.AXIS_LTRIGGER),
            event.getAxisValue(MotionEvent.AXIS_BRAKE));
        float rightTrigger = Math.max(
            event.getAxisValue(MotionEvent.AXIS_RTRIGGER),
            event.getAxisValue(MotionEvent.AXIS_GAS));
        emitBindingAxis(
            BindingInput.GAMEPAD_TRIGGER_LEFT, clampAxis(leftTrigger), sourceName);
        emitBindingAxis(
            BindingInput.GAMEPAD_TRIGGER_RIGHT, clampAxis(rightTrigger), sourceName);

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

    private void emitBindingRelative(BindingInput input, float delta,
                                     String source) {
        BindingListener target = bindingListener;
        if (target != null && Float.isFinite(delta) && delta != 0.0f)
            target.onBindingRelative(input, delta, safeSource(source));
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
            default:
                return null;
        }
    }

    /** Preserve individual Android key identities for hosted remapping.
     * Shell navigation mapping stays independent of these game-facing edges. */
    static BindingInput mapPhysicalGamepadKey(int keyCode) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A: return BindingInput.GAMEPAD_BUTTON_A;
            case KeyEvent.KEYCODE_BUTTON_B: return BindingInput.GAMEPAD_BUTTON_B;
            case KeyEvent.KEYCODE_BUTTON_X: return BindingInput.GAMEPAD_BUTTON_X;
            case KeyEvent.KEYCODE_BUTTON_Y: return BindingInput.GAMEPAD_BUTTON_Y;
            case KeyEvent.KEYCODE_BUTTON_L1: return BindingInput.GAMEPAD_BUTTON_L1;
            case KeyEvent.KEYCODE_BUTTON_R1: return BindingInput.GAMEPAD_BUTTON_R1;
            case KeyEvent.KEYCODE_BUTTON_L2: return BindingInput.GAMEPAD_BUTTON_L2;
            case KeyEvent.KEYCODE_BUTTON_R2: return BindingInput.GAMEPAD_BUTTON_R2;
            case KeyEvent.KEYCODE_BUTTON_THUMBL:
                return BindingInput.GAMEPAD_BUTTON_THUMBL;
            case KeyEvent.KEYCODE_BUTTON_THUMBR:
                return BindingInput.GAMEPAD_BUTTON_THUMBR;
            case KeyEvent.KEYCODE_BUTTON_SELECT:
                return BindingInput.GAMEPAD_BUTTON_SELECT;
            case KeyEvent.KEYCODE_BUTTON_START:
                return BindingInput.GAMEPAD_BUTTON_START;
            case KeyEvent.KEYCODE_BUTTON_MODE:
                return BindingInput.GAMEPAD_BUTTON_MODE;
            case KeyEvent.KEYCODE_DPAD_LEFT: return BindingInput.GAMEPAD_DPAD_LEFT;
            case KeyEvent.KEYCODE_DPAD_RIGHT: return BindingInput.GAMEPAD_DPAD_RIGHT;
            case KeyEvent.KEYCODE_DPAD_UP: return BindingInput.GAMEPAD_DPAD_UP;
            case KeyEvent.KEYCODE_DPAD_DOWN: return BindingInput.GAMEPAD_DPAD_DOWN;
            default: return null;
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
