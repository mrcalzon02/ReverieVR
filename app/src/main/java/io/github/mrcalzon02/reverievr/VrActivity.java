package io.github.mrcalzon02.reverievr;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.graphics.SurfaceTexture;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.hardware.input.InputManager;
import android.media.AudioManager;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.provider.Settings;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;

import com.google.cardboard.proto.CardboardDevice;
import com.google.cardboard.sdk.CardboardView;
import com.google.cardboard.sdk.QrCode;
import com.google.cardboard.sdk.deviceparams.DeviceParamsUtils;

import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

public final class VrActivity extends Activity
    implements ControllerManager.Listener,
        VrShellRenderer.Host,
        LocalVideoPlayer.Listener,
        InputManager.InputDeviceListener,
        VrInputRouter.Listener,
        VrInputRouter.BindingListener,
        StandardHidInputRouter.Listener,
        SensorEventListener,
        DosSession.Listener {

    static final String EXTRA_STARTUP_ERROR =
        "io.github.mrcalzon02.reverievr.VR_STARTUP_ERROR";
    static final String EXTRA_REQUEST_MEDIA_PICKER =
        "io.github.mrcalzon02.reverievr.REQUEST_MEDIA_PICKER";
    static final String EXTRA_REQUEST_MODEL_PICKER =
        "io.github.mrcalzon02.reverievr.REQUEST_MODEL_PICKER";
    static final String EXTRA_REQUEST_DOS_PICKER =
        "io.github.mrcalzon02.reverievr.REQUEST_DOS_PICKER";
    static final String EXTRA_REQUEST_UPDATE_CHECK =
        "io.github.mrcalzon02.reverievr.REQUEST_UPDATE_CHECK";
    static final String EXTRA_REQUEST_BLUETOOTH_SETTINGS =
        "io.github.mrcalzon02.reverievr.REQUEST_BLUETOOTH_SETTINGS";
    static final String EXTRA_REQUEST_CONTROLLER_PAIRING =
        "io.github.mrcalzon02.reverievr.REQUEST_CONTROLLER_PAIRING";

    private static final float SAFE_VIEWER_FALLBACK_IPD_METERS = 0.060f;
    private static final int PHONE_CAPTURE_CONSENT_REQUEST = 1207;
    private final PhoneScreenCaptureSession phoneCapture =
        new PhoneScreenCaptureSession();

    private CardboardView cardboardView;
    private VrShellRenderer renderer;
    private ControllerManager controllerManager;
    private ReveriePreferences preferences;
    private UiFeedback uiFeedback;
    private ShellMenuMusic menuMusic;
    private LocalVideoPlayer videoPlayer;
    private InputManager inputManager;
    private VrInputRouter inputRouter;
    private InputBindingManager inputBindingManager;
    private InputBindingEngine inputBindingEngine;
    private VirtualInputBus virtualInputBus;
    private StandardHidInputRouter standardHidInputRouter;
    private DosModuleRepository dosModuleRepository;
    private BundledDosContentInstaller bundledDosContentInstaller;
    private DosSession dosSession;
    private NativeModuleRuntime nativeModuleRuntime;
    private SensorManager sensorManager;
    private Sensor linearAccelerationSensor;
    private boolean headMotionSensorRegistered;
    private boolean cardboardRendererBound;
    private volatile boolean startupFailed;

    private final AtomicInteger batteryTemperatureTenthsC =
        new AtomicInteger(
            PerformanceEnvironmentSnapshot.BATTERY_TEMPERATURE_UNAVAILABLE
        );
    private final AtomicInteger thermalStatus =
        new AtomicInteger(
            PerformanceEnvironmentSnapshot.THERMAL_STATUS_UNAVAILABLE
        );
    private final BroadcastReceiver batteryTemperatureReceiver =
        new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                updateBatteryStatus(intent);
            }
        };
    private boolean batteryTemperatureReceiverRegistered;
    private Api29ThermalMonitor api29ThermalMonitor;
    private boolean inputDeviceListenerRegistered;
    private volatile String activeDosModuleId = "";
    private volatile String controllerConnectionMessage =
        "Controller is not connected.";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        try {
            VrStartupGuard.setPhase(
                this,
                "activity-window"
            );
            requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        enterImmersiveMode();

        VrStartupGuard.setPhase(
            this,
            "runtime-dependencies"
        );

        ReverieApplication application =
            (ReverieApplication) getApplication();
        controllerManager =
            application.getControllerManager();
        inputBindingManager =
            application.getInputBindingManager();
        inputBindingEngine =
            inputBindingManager.getEngine();
        virtualInputBus =
            inputBindingManager.getBus();
        standardHidInputRouter =
            new StandardHidInputRouter(this);
        preferences = new ReveriePreferences(this);
        uiFeedback = new UiFeedback(this);
        menuMusic = new ShellMenuMusic(this);
        videoPlayer = new LocalVideoPlayer(this, this);
        dosModuleRepository =
            new DosModuleRepository(this);
        bundledDosContentInstaller =
            new BundledDosContentInstaller(
                this,
                dosModuleRepository
            );
        dosSession =
            new DosSession(
                this,
                virtualInputBus,
                this
            );
        nativeModuleRuntime =
            new NativeModuleRuntime(
                new java.io.File(
                    getFilesDir(),
                    "native-modules"
                ),
                virtualInputBus,
                preferences.getLoggingMode()
                    == LoggingMode.DEVELOPMENT
            );
        inputManager =
            (InputManager) getSystemService(Context.INPUT_SERVICE);
        sensorManager =
            (SensorManager) getSystemService(
                Context.SENSOR_SERVICE
            );
        if (sensorManager != null) {
            linearAccelerationSensor =
                sensorManager.getDefaultSensor(
                    Sensor.TYPE_LINEAR_ACCELERATION
                );
        }
        inputRouter = new VrInputRouter(this);
        inputRouter.setBindingListener(this);

        VrStartupGuard.setPhase(
            this,
            "cardboard-jni-and-view"
        );
        CardboardView.setUseCardboardGlSurfaceView(true);
        cardboardView = new CardboardView(this);
        cardboardView.setStereoRenderMode(true);

        float viewerIpd = readViewerInterLensDistance();

        renderer =
            new VrShellRenderer(
                this,
                preferences,
                viewerIpd,
                dosSession,
                nativeModuleRuntime,
                this
            );
        renderer.setPhoneBattery(readPhoneBattery());
        refreshDosModuleStatus();

        VrStartupGuard.setPhase(
            this,
            "cardboard-renderer-bind"
        );
        cardboardView.setRenderer(renderer);
        cardboardRendererBound = true;
        cardboardView.setOnBackButtonClick(this::finish);
        cardboardView.setOnSettingsButtonClick(
            () -> inputRouter.submitAction(
                VrInputAction.MENU,
                "Cardboard system control"
            )
        );
        cardboardView.setOnTriggerEvent(
            () -> inputRouter.submitAction(
                VrInputAction.SELECT,
                "Cardboard trigger"
            )
        );

        VrStartupGuard.setPhase(
            this,
            "waiting-first-frame"
        );
        setContentView(cardboardView);
        controllerManager.addListener(this);
        refreshInputSourceStatus();

        ReverieLog.milestone(
            "VR_STARTUP",
            "VR activity created; waiting for first rendered frame."
        );
        } catch (RuntimeException | LinkageError failure) {
            failVrStartup(
                "activity-initialization",
                failure
            );
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (startupFailed) {
            return;
        }

        registerInputDeviceListener();
        registerHeadMotionSensor();
        registerPerformanceEnvironmentMonitoring();
        enterImmersiveMode();

        if (cardboardView != null) {
            cardboardView.onResume();
        }
        if (uiFeedback != null) {
            uiFeedback.resumeForLifecycle();
        }
        if (menuMusic != null) {
            menuMusic.resumeForLifecycle();
        }
        if (renderer != null) {
            renderer.setPhoneBattery(readPhoneBattery());
        }
        if (videoPlayer != null) {
            videoPlayer.resumeForLifecycle();
        }
        if (dosSession != null) {
            dosSession.resumeForLifecycle();
        }
        if (nativeModuleRuntime != null) {
            nativeModuleRuntime.resumeForLifecycle();
        }

        refreshInputSourceStatus();
        refreshDosModuleStatus();
    }

    @Override
    protected void onPause() {
        unregisterInputDeviceListener();
        unregisterHeadMotionSensor();

        if (inputBindingEngine != null) {
            inputBindingEngine.releaseAll();
        }
        if (standardHidInputRouter != null) {
            standardHidInputRouter.reset();
        }
        if (uiFeedback != null) {
            uiFeedback.pauseForLifecycle();
        }
        if (menuMusic != null) {
            menuMusic.pauseForLifecycle();
        }
        if (videoPlayer != null) {
            videoPlayer.pauseForLifecycle();
        }
        if (dosSession != null) {
            dosSession.pauseForLifecycle();
        }
        if (nativeModuleRuntime != null) {
            nativeModuleRuntime.pauseForLifecycle();
        }
        if (cardboardView != null
            && cardboardRendererBound) {
            cardboardView.onPause();
        }
        unregisterPerformanceEnvironmentMonitoring();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        phoneCapture.close();
        unregisterInputDeviceListener();
        unregisterHeadMotionSensor();

        if (controllerManager != null) {
            controllerManager.removeListener(this);
        }
        if (videoPlayer != null) {
            videoPlayer.release();
        }
        if (dosSession != null) {
            dosSession.close();
        }
        if (inputBindingManager != null) {
            inputBindingManager.endHostedProfile();
        }
        if (cardboardView != null
            && cardboardRendererBound) {
            cardboardView.onDestroy();
        }
        if (nativeModuleRuntime != null) {
            nativeModuleRuntime.close();
        }
        if (menuMusic != null) {
            menuMusic.close();
        }
        if (uiFeedback != null) {
            uiFeedback.close();
        }
        super.onDestroy();
    }

    @Override
    public void onSensorChanged(
        SensorEvent event
    ) {
        if (event == null
            || event.sensor == null
            || event.sensor.getType()
                != Sensor.TYPE_LINEAR_ACCELERATION
            || event.values == null
            || event.values.length < 3
            || renderer == null) {
            return;
        }

        renderer.setHeadLinearAcceleration(
            event.values[0],
            event.values[1],
            event.values[2],
            event.timestamp
        );
    }

    @Override
    public void onAccuracyChanged(
        Sensor sensor,
        int accuracy
    ) {
        // Linear-acceleration accuracy changes do not alter the bounded
        // translation contract; samples continue through drift correction.
    }

    @Override
    public void onVrKeyboardTextCommitted(String text) {
        final String safeText = text == null ? "" : text;
        runOnUiThread(() -> {
            ClipboardManager clipboard =
                (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
            if (clipboard != null) {
                clipboard.setPrimaryClip(
                    ClipData.newPlainText("ReverieVR keyboard", safeText)
                );
                ReverieLog.milestone("VR_KEYBOARD",
                    "Keyboard text copied to Android clipboard.");
            }
        });
    }

    @Override
    public void onVrFirstFrameRendered() {
        if (startupFailed) {
            return;
        }

        runOnUiThread(() -> {
            if (startupFailed) {
                return;
            }

            VrStartupGuard.markFirstFrame(this);
            ReverieLog.milestone(
                "VR_STARTUP",
                "First VR frame rendered successfully."
            );
        });
    }

    @Override
    public void onVrRendererFailure(
        String phase,
        Throwable throwable
    ) {
        String safePhase =
            phase == null
                || phase.trim().isEmpty()
                ? "renderer"
                : "renderer-" + phase.trim();

        runOnUiThread(
            () -> failVrStartup(
                safePhase,
                throwable
            )
        );
    }

    private void failVrStartup(
        String phase,
        Throwable throwable
    ) {
        if (startupFailed) {
            return;
        }
        startupFailed = true;

        String safePhase =
            phase == null
                || phase.trim().isEmpty()
                ? "unknown"
                : phase.trim();

        VrStartupGuard.recordFailure(
            this,
            safePhase,
            throwable
        );
        ReverieLog.error(
            "VR_STARTUP",
            "VR startup failed during "
                + safePhase
                + ".",
            throwable
        );

        String detail =
            throwable == null
                ? ""
                : throwable.getClass().getSimpleName()
                    + (
                        throwable.getMessage() == null
                            || throwable.getMessage()
                                .trim()
                                .isEmpty()
                            ? ""
                            : ": "
                                + throwable.getMessage()
                                    .trim()
                    );

        String message =
            "VR startup failed during "
                + safePhase
                + "."
                + (
                    detail.isEmpty()
                        ? ""
                        : "\n\n" + detail
                );

        Intent result = new Intent();
        result.putExtra(
            EXTRA_STARTUP_ERROR,
            message
        );
        setResult(
            Activity.RESULT_CANCELED,
            result
        );
        finish();
    }

    private static boolean isHeadsetBlockedVolumeKey(int keyCode) {
        return keyCode == KeyEvent.KEYCODE_VOLUME_UP
            || keyCode == KeyEvent.KEYCODE_VOLUME_DOWN
            || keyCode == KeyEvent.KEYCODE_VOLUME_MUTE;
    }

    private boolean blockedControllerInput(android.view.InputEvent event) {
        if (event == null) return false;
        String selected = preferences.getActiveController();
        boolean steam = AndroidGamepadSupport.isOriginalSteamController(event.getDevice());
        boolean scrcpy = AndroidGamepadSupport.isScrcpyHid(event.getDevice())
            || ReveriePreferences.CONTROLLER_SCRCPY.equals(selected)
                && AndroidGamepadSupport.isScrcpyCompatibleGamepad(event.getDevice());
        if (steam) return ReveriePreferences.CONTROLLER_DAYDREAM.equals(selected)
            || ReveriePreferences.CONTROLLER_PHONE.equals(selected)
            || ReveriePreferences.CONTROLLER_SCRCPY.equals(selected)
            || ReveriePreferences.CONTROLLER_AUTO.equals(selected) && controllerManager.isReady();
        if (scrcpy) return ReveriePreferences.CONTROLLER_STEAM.equals(selected)
            || ReveriePreferences.CONTROLLER_DAYDREAM.equals(selected)
            || ReveriePreferences.CONTROLLER_PHONE.equals(selected)
            || ReveriePreferences.CONTROLLER_AUTO.equals(selected)
                && (controllerManager.isReady()
                    || AndroidGamepadSupport.firstConnectedSteamController() != null
                    || (AndroidGamepadSupport.firstConnectedGamepadName() != null
                        && !ScrcpyHidIdentity.matchesName(
                            AndroidGamepadSupport.firstConnectedGamepadName())));
        return (ReveriePreferences.CONTROLLER_STEAM.equals(selected)
            || ReveriePreferences.CONTROLLER_SCRCPY.equals(selected))
            && AndroidGamepadSupport.isGamepadEvent(event);
    }

    /** Respect the persistent external HID preference without blocking a
     * gamepad, touchscreen recovery or host-reserved Menu/Back navigation. */
    private boolean consumeDisabledExternalHid(android.view.InputEvent event,
                                               boolean recovery) {
        if (event == null || preferences.isExternalHidEnabled()) return false;
        int sources = event.getSource();
        boolean gamepad = AndroidGamepadSupport.isGamepadEvent(event);
        boolean keyboard = (sources & android.view.InputDevice.SOURCE_KEYBOARD)
            == android.view.InputDevice.SOURCE_KEYBOARD;
        boolean mouse = (sources & android.view.InputDevice.SOURCE_MOUSE)
            == android.view.InputDevice.SOURCE_MOUSE
            || (sources & android.view.InputDevice.SOURCE_MOUSE_RELATIVE)
            == android.view.InputDevice.SOURCE_MOUSE_RELATIVE;
        android.view.InputDevice device = event.getDevice();
        // Do not block Android's own virtual keyboard or touchscreen dispatch.
        if (device != null && device.isVirtual()) return false;
        return ExternalHidInputPolicy.shouldConsume(false, keyboard, mouse,
            gamepad, recovery);
    }

    @Override
    public void onExternalHidInputPolicyChanged(boolean enabled) {
        if (!enabled) runOnUiThread(() -> {
            // Clear any held HID keys/mouse buttons before the next event;
            // StandardHidInputRouter emits explicit reset releases.
            if (standardHidInputRouter != null) standardHidInputRouter.reset();
        });
    }

    private boolean selectedShellMouse(String source) {
        String selected = preferences.getActiveController();
        if (SteamControllerIdentity.matchesAndroidHid(0, source))
            return ReveriePreferences.CONTROLLER_STEAM.equals(selected)
                || ReveriePreferences.CONTROLLER_AUTO.equals(selected)
                    && !controllerManager.isReady();
        if (!ScrcpyHidIdentity.matchesName(source)) return false;
        return ReveriePreferences.CONTROLLER_SCRCPY.equals(selected)
            || ReveriePreferences.CONTROLLER_AUTO.equals(selected)
                && !controllerManager.isReady()
                && AndroidGamepadSupport.firstConnectedSteamController() == null
                && (AndroidGamepadSupport.firstConnectedGamepadName() == null
                    || ScrcpyHidIdentity.matchesName(
                        AndroidGamepadSupport.firstConnectedGamepadName()));
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (blockedControllerInput(event)) return true;
        // The phone is enclosed by the viewer in VR. Clamp pressure can hold
        // these keys down; consume both presses and releases before routing.
        if (event != null && isHeadsetBlockedVolumeKey(event.getKeyCode())) {
            return true;
        }

        if (event != null
            && event.getKeyCode() == KeyEvent.KEYCODE_MENU
            && event.getAction() == KeyEvent.ACTION_DOWN
            && event.getRepeatCount() == 0
            && inputRouter != null) {
            inputRouter.submitAction(
                VrInputAction.MENU,
                "Keyboard Menu key"
            );
            return true;
        }

        if (routeQuickMenuKeyboardEvent(event)) {
            return true;
        }
        if (consumeDisabledExternalHid(event,
                event != null && (event.getKeyCode() == KeyEvent.KEYCODE_MENU
                    || event.getKeyCode() == KeyEvent.KEYCODE_BACK
                    || event.getKeyCode() == KeyEvent.KEYCODE_ESCAPE))) {
            return true;
        }

        if (routeExternalHomeKeyboardInput(event)) {
            return true;
        }

        if (inputRouter != null && inputRouter.onKeyEvent(event)) {
            return true;
        }
        if (standardHidInputRouter != null
            && standardHidInputRouter.onKeyboardEvent(event)) {
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    /**
     * Physical Bluetooth/USB keyboards and scrcpy UHID type into the same
     * desk editor. Android's virtual keyboard, Steam HID compatibility keys
     * and gamepad action keys remain owned by their existing input paths.
     * The external-input toggle is enforced before we reach this method.
     */
    private boolean routeExternalHomeKeyboardInput(KeyEvent event) {
        if (event == null || renderer == null) return false;
        android.view.InputDevice device = event.getDevice();
        boolean physicalKeyboard = device != null && !device.isVirtual();
        boolean sourceKeyboard =
            (event.getSource() & android.view.InputDevice.SOURCE_KEYBOARD)
                == android.view.InputDevice.SOURCE_KEYBOARD;
        boolean controllerDevice = AndroidGamepadSupport.isGamepadEvent(event)
            || AndroidGamepadSupport.isOriginalSteamController(device);
        if (!ExternalHidInputPolicy.shouldEditHomeKeyboard(
                preferences.isExternalHidEnabled(),
                renderer.acceptsPhysicalKeyboardInput(),
                sourceKeyboard, physicalKeyboard, controllerDevice,
                event.isCtrlPressed() || event.isAltPressed()
                    || event.isMetaPressed())) return false;

        int action = VrKeyboardEditor.EXTERNAL_TEXT;
        String characters = null;
        switch (event.getKeyCode()) {
            case KeyEvent.KEYCODE_DEL:
                action = VrKeyboardEditor.EXTERNAL_BACKSPACE;
                break;
            case KeyEvent.KEYCODE_FORWARD_DEL:
                action = VrKeyboardEditor.EXTERNAL_DELETE;
                break;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                action = VrKeyboardEditor.EXTERNAL_LEFT;
                break;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                action = VrKeyboardEditor.EXTERNAL_RIGHT;
                break;
            case KeyEvent.KEYCODE_TAB:
                action = VrKeyboardEditor.EXTERNAL_TAB;
                break;
            case KeyEvent.KEYCODE_ENTER:
            case KeyEvent.KEYCODE_NUMPAD_ENTER:
                action = VrKeyboardEditor.EXTERNAL_ENTER;
                break;
            default:
                int codePoint = event.getUnicodeChar();
                if (codePoint < 32 || !Character.isValidCodePoint(codePoint))
                    return false;
                characters = new String(Character.toChars(codePoint));
                break;
        }
        if (event.getAction() == KeyEvent.ACTION_UP) return true;
        if (event.getAction() != KeyEvent.ACTION_DOWN) return false;
        return renderer.queuePhysicalKeyboardEdit(action, characters);
    }

    private boolean routeQuickMenuKeyboardEvent(
        KeyEvent event
    ) {
        if (event == null
            || renderer == null
            || !renderer.isOrientationMenuVisible()
            || inputRouter == null) {
            return false;
        }

        int action = event.getAction();
        if (action == KeyEvent.ACTION_UP) {
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_LEFT:
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                case KeyEvent.KEYCODE_ENTER:
                case KeyEvent.KEYCODE_NUMPAD_ENTER:
                case KeyEvent.KEYCODE_SPACE:
                case KeyEvent.KEYCODE_ESCAPE:
                case KeyEvent.KEYCODE_BACK:
                    return true;

                default:
                    return false;
            }
        }

        if (action != KeyEvent.ACTION_DOWN
            || event.getRepeatCount() != 0) {
            return false;
        }

        switch (event.getKeyCode()) {
            case KeyEvent.KEYCODE_DPAD_LEFT:
                return renderer
                    .requestQuickMenuKeyboardNavigation(
                        VrInputAction.NAV_LEFT
                    );

            case KeyEvent.KEYCODE_DPAD_RIGHT:
                return renderer
                    .requestQuickMenuKeyboardNavigation(
                        VrInputAction.NAV_RIGHT
                    );

            case KeyEvent.KEYCODE_DPAD_UP:
                return renderer
                    .requestQuickMenuKeyboardNavigation(
                        VrInputAction.NAV_UP
                    );

            case KeyEvent.KEYCODE_DPAD_DOWN:
                return renderer
                    .requestQuickMenuKeyboardNavigation(
                        VrInputAction.NAV_DOWN
                    );

            case KeyEvent.KEYCODE_ENTER:
            case KeyEvent.KEYCODE_NUMPAD_ENTER:
            case KeyEvent.KEYCODE_SPACE:
                inputRouter.submitAction(
                    VrInputAction.SELECT,
                    "Keyboard quick menu"
                );
                return true;

            case KeyEvent.KEYCODE_ESCAPE:
            case KeyEvent.KEYCODE_BACK:
                inputRouter.submitAction(
                    VrInputAction.BACK,
                    "Keyboard quick menu"
                );
                return true;

            default:
                return false;
        }
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {
        if (blockedControllerInput(event)) return true;
        if (consumeDisabledExternalHid(event, false)) return true;
        if (inputRouter != null
            && inputRouter.onGenericMotionEvent(event)) {
            return true;
        }

        if (routeStandardMouseEvent(event)) {
            return true;
        }

        return super.dispatchGenericMotionEvent(event);
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (blockedControllerInput(event)) return true;
        // The display is physically inaccessible inside the headset. Consume
        // touchscreen events before Cardboard can interpret clamp-nub contacts
        // as trigger or navigation input. External mouse input remains routed.
        if (event != null
            && (event.getSource() & android.view.InputDevice.SOURCE_TOUCHSCREEN)
                == android.view.InputDevice.SOURCE_TOUCHSCREEN) {
            return true;
        }

        if (consumeDisabledExternalHid(event, false)) return true;
        if (event != null) {
            int source = event.getSource();
            boolean mouse =
                (source & android.view.InputDevice.SOURCE_MOUSE)
                    == android.view.InputDevice.SOURCE_MOUSE
                || (source & android.view.InputDevice.SOURCE_MOUSE_RELATIVE)
                    == android.view.InputDevice.SOURCE_MOUSE_RELATIVE;

            if (mouse && routeStandardMouseEvent(event)) {
                return true;
            }
        }

        return super.dispatchTouchEvent(event);
    }

    private boolean routeStandardMouseEvent(MotionEvent event) {
        int width = cardboardView == null
            ? 0
            : cardboardView.getWidth();
        int height = cardboardView == null
            ? 0
            : cardboardView.getHeight();

        return standardHidInputRouter != null
            && standardHidInputRouter.onMouseEvent(
                event,
                width,
                height
            );
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            enterImmersiveMode();
        }
    }

    @Override
    public void onConnectionStateChanged(
        ControllerProvider.ConnectionState state,
        String message
    ) {
        controllerConnectionMessage =
            message == null ? "" : message;

        if (state != ControllerProvider.ConnectionState.READY
            && inputBindingEngine != null) {
            inputBindingEngine.releaseAll();
        }

        refreshInputSourceStatus();
    }

    @Override
    public void onBatteryChanged(int percentage, int millivolts) {
        refreshInputSourceStatus();
    }

    @Override
    public void onControllerStateChanged(
        ControllerSnapshot snapshot
    ) {
        if (inputRouter == null || snapshot == null
                || ReveriePreferences.CONTROLLER_STEAM.equals(
                    preferences.getActiveController())) return;

        if (renderer != null) {
            renderer.setControllerPose(snapshot);
        }

        inputRouter.onControllerSnapshot(
            snapshot,
            controllerManager.getActiveProviderDisplayName()
        );
    }

    @Override
    public void onPointerAxis(
        float horizontal,
        float vertical,
        String source
    ) {
        if (renderer != null) {
            renderer.setVirtualPointerAxes(
                horizontal,
                vertical
            );
        }
    }

    @Override
    public void onInputAction(
        VrInputAction action,
        String source
    ) {
        if (renderer == null || action == null) {
            return;
        }

        renderer.noteInputAction(action, source);

        switch (action) {
            case SELECT:
                if (renderer.handlesSelectAsShellAction()) {
                    if (renderer.canActivateSelect()) {
                        uiFeedback.activation();
                    } else {
                        uiFeedback.failure();
                    }
                }
                renderer.requestSelect();
                break;

            case BACK:
                if (!renderer.consumeBackDuringInputTraining()) {
                    renderer.requestBack();
                }
                break;

            case MENU:
                // Release held HID guest keys before taking shell focus.
                if (!renderer.isOrientationMenuVisible()
                    && standardHidInputRouter != null)
                    standardHidInputRouter.reset();
                uiFeedback.activation();
                renderer.toggleOrientationMenu();
                break;

            case RECENTER:
                renderer.requestRecenter();
                break;

            case NAV_LEFT:
                if (!renderer.requestPointerNavigation(
                        VrInputAction.NAV_LEFT
                    )) {
                    renderer.requestVideoSeek(-10000);
                }
                break;

            case NAV_RIGHT:
                if (!renderer.requestPointerNavigation(
                        VrInputAction.NAV_RIGHT
                    )) {
                    renderer.requestVideoSeek(10000);
                }
                break;

            case VOLUME_UP:
                adjustMediaVolume(AudioManager.ADJUST_RAISE);
                break;

            case VOLUME_DOWN:
                adjustMediaVolume(AudioManager.ADJUST_LOWER);
                break;

            case NAV_UP:
                renderer.requestPointerNavigation(
                    VrInputAction.NAV_UP
                );
                break;

            case NAV_DOWN:
                renderer.requestPointerNavigation(
                    VrInputAction.NAV_DOWN
                );
                break;

            default:
                break;
        }
    }

    @Override
    public void onBindingDigital(
        BindingInput input,
        boolean down,
        String source
    ) {
        if (isGuestInputSuppressed()) {
            return;
        }
        if (inputBindingEngine != null) {
            inputBindingEngine.submitDigital(
                input,
                down
            );
        }
    }

    @Override
    public void onBindingAxis(
        BindingInput input,
        float value,
        String source
    ) {
        if (isGuestInputSuppressed()) {
            return;
        }
        if (inputBindingEngine != null) {
            inputBindingEngine.submitAxis(
                input,
                value
            );
        }
    }

    @Override
    public void onHeadBindingDelta(
        float yawDeltaRadians,
        float pitchDeltaRadians
    ) {
        if (inputBindingEngine == null
            || isGuestInputSuppressed()) {
            return;
        }

        inputBindingEngine.submitRelative(
            BindingInput.HEAD_YAW_DELTA,
            yawDeltaRadians
        );
        inputBindingEngine.submitRelative(
            BindingInput.HEAD_PITCH_DELTA,
            pitchDeltaRadians
        );
    }

    @Override
    public void onVirtualKey(
        VirtualKey key,
        boolean down,
        String source
    ) {
        if (isGuestInputSuppressed()
            && !(StandardHidInputRouter.RESET_SOURCE.equals(source) && !down)) {
            return;
        }
        if (virtualInputBus != null && key != null) {
            virtualInputBus.applyDigital(
                VirtualOutput.key(key),
                down
            );
        }
    }

    @Override
    public void onVirtualMouseButton(
        int button,
        boolean down,
        String source
    ) {
        if (down && button == VirtualOutput.MOUSE_LEFT && selectedShellMouse(source)
                && renderer != null && renderer.handlesSelectAsShellAction())
            inputRouter.submitAction(VrInputAction.SELECT, source);
        if (isGuestInputSuppressed()
            && !(StandardHidInputRouter.RESET_SOURCE.equals(source) && !down)) {
            return;
        }
        if (virtualInputBus != null) {
            virtualInputBus.applyDigital(
                VirtualOutput.mouseButton(button),
                down
            );
        }
    }

    @Override
    public void onVirtualMouseRelative(
        float deltaX,
        float deltaY,
        String source
    ) {
        if (selectedShellMouse(source) && renderer != null)
            renderer.nudgeExternalPointer(deltaX, deltaY);
        if (virtualInputBus == null
            || isGuestInputSuppressed()) {
            return;
        }

        virtualInputBus.applyAnalog(
            VirtualOutput.mouseRelativeX(),
            deltaX
        );
        virtualInputBus.applyAnalog(
            VirtualOutput.mouseRelativeY(),
            deltaY
        );
    }

    @Override
    public void onVirtualMouseAbsolute(
        float normalizedX,
        float normalizedY,
        String source
    ) {
        if (virtualInputBus == null
            || isGuestInputSuppressed()) {
            return;
        }

        virtualInputBus.applyAnalog(
            VirtualOutput.mouseAbsoluteX(),
            (normalizedX * 2.0f) - 1.0f
        );
        virtualInputBus.applyAnalog(
            VirtualOutput.mouseAbsoluteY(),
            (normalizedY * 2.0f) - 1.0f
        );
    }

    @Override
    public void onVirtualMouseWheel(
        float delta,
        String source
    ) {
        if (isGuestInputSuppressed()) {
            return;
        }
        if (virtualInputBus != null) {
            virtualInputBus.applyMouseWheel(delta);
        }
    }

    private boolean isGuestInputSuppressed() {
        return renderer != null
            && renderer.isHostedInputSuppressed();
    }

    private void adjustMediaVolume(int direction) {
        AudioManager audio =
            (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        if (audio != null) {
            audio.adjustStreamVolume(
                AudioManager.STREAM_MUSIC,
                direction,
                AudioManager.FLAG_SHOW_UI
            );
        }
    }

    private void registerInputDeviceListener() {
        if (inputManager != null && !inputDeviceListenerRegistered) {
            inputManager.registerInputDeviceListener(this, null);
            inputDeviceListenerRegistered = true;
        }
    }

    private void unregisterInputDeviceListener() {
        if (inputManager != null && inputDeviceListenerRegistered) {
            inputManager.unregisterInputDeviceListener(this);
            inputDeviceListenerRegistered = false;
        }
    }

    @Override
    public void onInputDeviceAdded(int deviceId) {
        refreshInputSourceStatus();
    }

    @Override
    public void onInputDeviceRemoved(int deviceId) {
        if (inputBindingEngine != null) {
            inputBindingEngine.releaseAll();
        }
        if (standardHidInputRouter != null) {
            standardHidInputRouter.reset();
        }
        refreshInputSourceStatus();
    }

    @Override
    public void onInputDeviceChanged(int deviceId) {
        refreshInputSourceStatus();
    }

    private void refreshInputSourceStatus() {
        if (renderer == null || controllerManager == null) return;
        String selected = preferences.getActiveController();
        boolean provider = controllerManager.isReady() && (
            ReveriePreferences.CONTROLLER_AUTO.equals(selected)
                || ReveriePreferences.CONTROLLER_DAYDREAM.equals(selected)
                    && "daydream_ble".equals(controllerManager.getActiveProviderId())
                || ReveriePreferences.CONTROLLER_PHONE.equals(selected)
                    && "daydream_phone_emulator".equals(controllerManager.getActiveProviderId()));
        String gamepad = AndroidGamepadSupport.firstConnectedGamepadName();
        android.view.InputDevice sd = AndroidGamepadSupport.firstConnectedSteamController();
        String steam = sd == null ? null : sd.getName();
        android.view.InputDevice scrcpyDevice =
            ReveriePreferences.CONTROLLER_SCRCPY.equals(selected)
                ? AndroidGamepadSupport.firstConnectedScrcpySelectableInput()
                : AndroidGamepadSupport.firstConnectedScrcpyHid();
        String scrcpy = scrcpyDevice == null ? null : scrcpyDevice.getName();
        boolean steamPrimary = steam != null && (
            ReveriePreferences.CONTROLLER_STEAM.equals(selected)
                || ReveriePreferences.CONTROLLER_AUTO.equals(selected)
                    && !provider && (gamepad == null || steam.equals(gamepad)));
        boolean scrcpyPrimary = scrcpy != null && (
            ReveriePreferences.CONTROLLER_SCRCPY.equals(selected)
                || ReveriePreferences.CONTROLLER_AUTO.equals(selected)
                    && !provider && !steamPrimary
                    && (gamepad == null || ScrcpyHidIdentity.matchesName(gamepad)));
        String connectedVrController = steamPrimary ? steam
            : scrcpyPrimary ? scrcpy
            : ReveriePreferences.CONTROLLER_AUTO.equals(selected) && !provider
                ? gamepad : null;
        renderer.setGamepadPointerAvailable(!provider && connectedVrController != null);
        renderer.setSteamControllerPrimary(steamPrimary);
        controllerManager.refreshSteamBattery(steamPrimary);

        if (provider) {
            String providerName =
                controllerManager.getActiveProviderDisplayName();
            renderer.setControllerState(
                true,
                providerName == null || providerName.trim().isEmpty()
                    ? controllerConnectionMessage
                    : providerName + " ready"
            );
            renderer.setControllerBattery(
                controllerManager.getBatteryPercentage()
            );
            return;
        }

        if (connectedVrController != null) {
            renderer.setControllerState(
                true,
                "Android controller ready: " + connectedVrController
            );
            renderer.setControllerBattery(steamPrimary
                ? controllerManager.getSteamBatteryPercentage() : -1);
        } else {
            renderer.setControllerState(
                false,
                ReveriePreferences.CONTROLLER_STEAM.equals(selected)
                    ? "Selected Steam Controller disconnected"
                    : ReveriePreferences.CONTROLLER_SCRCPY.equals(selected)
                        ? "Selected scrcpy HID disconnected" : controllerConnectionMessage
            );
            renderer.setControllerBattery(-1);
        }
    }

    @Override
    public void onExitToPhoneRequested() {
        runOnUiThread(this::finish);
    }

    @Override
    public void onPhoneCaptureConsentRequested() {
        runOnUiThread(() -> {
            if (phoneCapture.isActive()) return;
            Intent consent = PhoneScreenCaptureSession.consentIntent(this);
            if (consent == null) {
                Toast.makeText(this,
                    "Android screen capture unavailable", Toast.LENGTH_LONG).show();
                return;
            }
            try {
                startActivityForResult(consent, PHONE_CAPTURE_CONSENT_REQUEST);
            } catch (RuntimeException failure) {
                ReverieLog.error("PHONE_CAPTURE",
                    "Could not launch capture consent", failure);
                Toast.makeText(this,
                    "Could not request screen capture permission",
                    Toast.LENGTH_LONG).show();
            }
        });
    }

    @Override
    public void onPhoneCaptureDismissed() {
        runOnUiThread(() -> phoneCapture.close());
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != PHONE_CAPTURE_CONSENT_REQUEST) return;
        if (resultCode != RESULT_OK) {
            ReverieLog.milestone("PHONE_CAPTURE", "Screen capture denied");
            return;
        }
        boolean active = phoneCapture.start(this, resultCode, data);
        ReverieLog.milestone("PHONE_CAPTURE", active
            ? "Consent accepted; capture frames acquired (VR upload pending)"
            : "Consent returned but capture session could not start");
        if (!active) Toast.makeText(this,
            "Phone screen capture could not start", Toast.LENGTH_LONG).show();
    }

    @Override
    public void onModelImportRequested() {
        returnToPhonePicker(EXTRA_REQUEST_MODEL_PICKER);
    }

    @Override
    public void onMediaSelectionRequested() {
        returnToPhonePicker(
            EXTRA_REQUEST_MEDIA_PICKER
        );
    }

    @Override
    public void onDosImportRequested() {
        returnToPhonePicker(
            EXTRA_REQUEST_DOS_PICKER
        );
    }

    @Override
    public void onUpdateCheckRequested() {
        returnToPhonePicker(
            EXTRA_REQUEST_UPDATE_CHECK
        );
    }

    @Override
    public void onBluetoothSettingsRequested() {
        returnToPhonePicker(
            EXTRA_REQUEST_BLUETOOTH_SETTINGS
        );
    }

    @Override
    public void onControllerPairingRequested() {
        returnToPhonePicker(
            EXTRA_REQUEST_CONTROLLER_PAIRING
        );
    }

    private void returnToPhonePicker(
        String extra
    ) {
        runOnUiThread(() -> {
            Intent result = new Intent();
            result.putExtra(extra, true);
            setResult(
                Activity.RESULT_OK,
                result
            );
            finish();
        });
    }

    @Override
    public void onUiFocusChanged() {
        runOnUiThread(() -> uiFeedback.focus());
    }

    @Override
    public void onUiActionRejected() {
        runOnUiThread(() -> uiFeedback.failure());
    }

    @Override
    public void onNativeModuleFeedbackRequested(
        int feedbackFlags
    ) {
        if (uiFeedback == null) {
            return;
        }

        final int safeFlags =
            feedbackFlags
                & NativeModuleRuntime.FEEDBACK_ALL;
        if (safeFlags == 0) {
            return;
        }

        runOnUiThread(() -> {
            if ((safeFlags
                    & NativeModuleRuntime.FEEDBACK_FOCUS)
                != 0) {
                uiFeedback.focus();
            }
            if ((safeFlags
                    & NativeModuleRuntime.FEEDBACK_ACTIVATION)
                != 0) {
                uiFeedback.activation();
            }
            if ((safeFlags
                    & NativeModuleRuntime.FEEDBACK_FAILURE)
                != 0) {
                uiFeedback.failure();
            }
            if ((safeFlags & NativeModuleRuntime.FEEDBACK_WEAPON_FIRE) != 0) {
                uiFeedback.weaponFire();
            }
            if ((safeFlags & NativeModuleRuntime.FEEDBACK_RELOAD_START) != 0) {
                uiFeedback.reloadStart();
            }
            if ((safeFlags & NativeModuleRuntime.FEEDBACK_RELOAD_READY) != 0) {
                uiFeedback.reloadReady();
            }
        });
    }

    @Override
    public void onControllerSpringRecenterRequested() {
        runOnUiThread(() -> uiFeedback.springRecenter());
    }

    @Override
    public void onSetupCompleted() {
        // The renderer persists the setup profile before this callback.
    }

    @Override
    public PerformanceEnvironmentSnapshot
        getPerformanceEnvironmentSnapshot() {
        return new PerformanceEnvironmentSnapshot(
            batteryTemperatureTenthsC.get(),
            thermalStatus.get()
        );
    }

    @Override
    public void onVideoSurfaceTextureReady(
        SurfaceTexture surfaceTexture
    ) {
        runOnUiThread(() -> {
            if (videoPlayer != null) {
                videoPlayer.attachSurfaceTexture(surfaceTexture);
            }
        });
    }

    private void setShellMenuMusicActive(
        boolean active
    ) {
        runOnUiThread(() -> {
            if (menuMusic != null) {
                menuMusic.setShellActive(active);
            }
        });
    }

    @Override
    public boolean onNativeModulePlaybackRequested(
        String moduleId
    ) {
        if (nativeModuleRuntime == null
            || inputBindingManager == null) {
            return false;
        }

        final String hostedProfileId =
            BuiltInBindingProfiles.ID_NATIVE_STANDARD;

        inputBindingManager.beginHostedProfile(
            hostedProfileId
        );

        if (!nativeModuleRuntime.start(moduleId)) {
            inputBindingManager.endHostedProfile();
            String message =
                nativeModuleRuntime.getLastError();
            runOnUiThread(() ->
                Toast.makeText(
                    this,
                    message.isEmpty()
                        ? "Could not start the native module."
                        : message,
                    Toast.LENGTH_LONG
                ).show()
            );
            return false;
        }

        setShellMenuMusicActive(false);
        ReverieLog.milestone(
            "NATIVE_MODULE",
            "Stage B launched module="
                + moduleId
                + " profile="
                + hostedProfileId
        );
        return true;
    }

    @Override
    public void onNativeModuleStopRequested() {
        if (nativeModuleRuntime != null) {
            nativeModuleRuntime.stop();
        }
        if (inputBindingManager != null) {
            inputBindingManager.endHostedProfile();
        }
        setShellMenuMusicActive(true);
    }

    @Override
    public boolean onDosPlaybackRequested(
        String moduleId
    ) {
        if (dosModuleRepository == null
            || dosSession == null
            || inputBindingManager == null) {
            return false;
        }

        DosGameModule module =
            dosModuleRepository.findById(moduleId);

        if (module != null
            && BundledDosContentInstaller.DOOM_MODULE_ID.equals(
                module.id
            )
            && bundledDosContentInstaller != null) {
            try {
                module =
                    bundledDosContentInstaller
                        .prepareForLaunch(module);
            } catch (java.io.IOException exception) {
                ReverieLog.error(
                    "BUNDLED_CONTENT",
                    "Could not prepare bundled DOOM for launch.",
                    exception
                );
                module = null;
            }
        }

        if (module == null || !module.isContentPresent()) {
            runOnUiThread(() ->
                Toast.makeText(
                    this,
                    "That DOS module is no longer available.",
                    Toast.LENGTH_LONG
                ).show()
            );
            refreshDosModuleStatus();
            return false;
        }

        BindingProfile hostedProfile =
            dosModuleRepository.resolveBindingProfile(module);
        inputBindingManager.beginHostedProfile(
            hostedProfile
        );
        activeDosModuleId = module.id;

        if (!dosSession.start(module)) {
            activeDosModuleId = "";
            inputBindingManager.endHostedProfile();
            String message = dosSession.getLastError();
            runOnUiThread(() ->
                Toast.makeText(
                    this,
                    message.isEmpty()
                        ? "Could not start the DOS module."
                        : message,
                    Toast.LENGTH_LONG
                ).show()
            );
            return false;
        }

        setShellMenuMusicActive(false);
        ReverieLog.milestone(
            "DOS_SESSION",
            "Stage B launched module="
                + module.id
                + " profile="
                + module.bindingProfileId
        );
        return true;
    }

    @Override
    public void onDosOverlayPauseRequested() {
        if (inputBindingEngine != null) {
            inputBindingEngine.releaseAll();
        }
        if (standardHidInputRouter != null) {
            standardHidInputRouter.reset();
        }
        if (dosSession != null) {
            dosSession.pauseForOverlay();
        }
    }

    @Override
    public void onDosOverlayResumeRequested() {
        if (dosSession != null) {
            dosSession.resumeFromOverlay();
        }
    }

    @Override
    public void onVolumeAdjustRequested(int direction) {
        if (direction < 0) {
            adjustMediaVolume(AudioManager.ADJUST_LOWER);
        } else if (direction > 0) {
            adjustMediaVolume(AudioManager.ADJUST_RAISE);
        }
    }

    @Override
    public void onBrightnessAdjustRequested(int direction) {
        if (direction == 0) {
            return;
        }

        runOnUiThread(() -> {
            WindowManager.LayoutParams attributes =
                getWindow().getAttributes();
            float current = attributes.screenBrightness;
            if (current < 0.0f) {
                int systemBrightness =
                    Settings.System.getInt(
                        getContentResolver(),
                        Settings.System.SCREEN_BRIGHTNESS,
                        128
                    );
                current =
                    Math.max(
                        0.0f,
                        Math.min(
                            1.0f,
                            systemBrightness / 255.0f
                        )
                    );
            }

            float delta =
                direction < 0
                    ? -0.10f
                    : 0.10f;
            float next =
                Math.max(
                    0.05f,
                    Math.min(
                        1.0f,
                        current + delta
                    )
                );
            attributes.screenBrightness = next;
            getWindow().setAttributes(attributes);
            ReverieLog.milestone(
                "VR_BRIGHTNESS",
                "windowBrightnessPercent="
                    + Math.round(next * 100.0f)
            );
        });
    }

    @Override
    public String getActiveBindingProfileName() {
        if (inputBindingManager == null) {
            return "No binding profile";
        }
        BindingProfile profile =
            inputBindingManager.getProfile();
        return profile == null
            ? "No binding profile"
            : profile.displayName;
    }

    @Override
    public String getActiveBindingTuningSummary() {
        if (inputBindingManager == null) {
            return "No active profile";
        }

        BindingProfile profile =
            inputBindingManager.getProfile();
        String summary =
            BindingProfileTuner.describeAnalog(profile);
        boolean custom =
            dosModuleRepository != null
                && dosModuleRepository.hasCustomBindingProfile(
                    activeDosModuleId
                );
        return summary
            + (custom ? " • CUSTOM" : " • BUILT-IN");
    }

    @Override
    public void onDosBindingProfileCycleRequested(
        int direction
    ) {
        if (direction == 0
            || dosModuleRepository == null
            || inputBindingManager == null
            || !inputBindingManager.isHostedProfileActive()) {
            return;
        }

        DosGameModule module =
            activeDosModule();
        if (module == null) {
            return;
        }

        List<BindingProfile> profiles =
            BuiltInBindingProfiles.dosProfiles();
        if (profiles.isEmpty()) {
            return;
        }

        int current = 0;
        for (int index = 0; index < profiles.size(); index++) {
            if (profiles.get(index).id.equals(
                module.bindingProfileId
            )) {
                current = index;
                break;
            }
        }

        int step = direction < 0 ? -1 : 1;
        int next =
            (current + step + profiles.size())
                % profiles.size();
        BindingProfile selected = profiles.get(next);

        if (!dosModuleRepository.updateBindingProfileId(
            module.id,
            selected.id
        )) {
            return;
        }

        inputBindingManager.setHostedProfile(selected);
        ReverieLog.milestone(
            "DOS_BINDING",
            "Module="
                + module.id
                + " selected profile="
                + selected.id
        );
    }

    @Override
    public void onDosBindingSensitivityAdjustRequested(
        int direction
    ) {
        if (direction == 0) {
            return;
        }
        tuneActiveDosProfile(
            direction < 0 ? 0.9f : 1.1f,
            0.0f,
            "sensitivity"
        );
    }

    @Override
    public void onDosBindingDeadzoneAdjustRequested(
        int direction
    ) {
        if (direction == 0) {
            return;
        }
        tuneActiveDosProfile(
            1.0f,
            direction < 0 ? -0.02f : 0.02f,
            "deadzone"
        );
    }

    @Override
    public void onDosBindingResetRequested() {
        if (dosModuleRepository == null
            || inputBindingManager == null
            || !inputBindingManager.isHostedProfileActive()) {
            return;
        }

        DosGameModule module = activeDosModule();
        if (module == null) {
            return;
        }

        dosModuleRepository.clearCustomBindingProfile(
            module.id
        );
        BindingProfile selected =
            BuiltInBindingProfiles.byId(
                module.bindingProfileId
            );
        inputBindingManager.setHostedProfile(selected);
        ReverieLog.milestone(
            "DOS_BINDING",
            "Module="
                + module.id
                + " reset profile="
                + selected.id
        );
    }

    private void tuneActiveDosProfile(
        float scaleFactor,
        float deadzoneDelta,
        String label
    ) {
        if (dosModuleRepository == null
            || inputBindingManager == null
            || !inputBindingManager.isHostedProfileActive()) {
            return;
        }

        DosGameModule module = activeDosModule();
        BindingProfile current =
            inputBindingManager.getProfile();
        if (module == null || current == null) {
            return;
        }

        BindingProfile tuned = current;
        if (scaleFactor != 1.0f) {
            tuned =
                BindingProfileTuner.adjustAnalogScale(
                    tuned,
                    scaleFactor
                );
        }
        if (deadzoneDelta != 0.0f) {
            tuned =
                BindingProfileTuner.adjustAnalogDeadzone(
                    tuned,
                    deadzoneDelta
                );
        }
        if (tuned == current) {
            return;
        }

        if (!dosModuleRepository.saveCustomBindingProfile(
            module.id,
            tuned
        )) {
            return;
        }

        inputBindingManager.setHostedProfile(tuned);
        ReverieLog.milestone(
            "DOS_BINDING",
            "Module="
                + module.id
                + " tuned "
                + label
                + " "
                + BindingProfileTuner.describeAnalog(tuned)
        );
    }

    private DosGameModule activeDosModule() {
        if (dosModuleRepository == null
            || activeDosModuleId == null
            || activeDosModuleId.trim().isEmpty()) {
            return null;
        }
        return dosModuleRepository.findById(
            activeDosModuleId
        );
    }

    @Override
    public void onDosStopRequested() {
        activeDosModuleId = "";
        if (dosSession != null) {
            dosSession.stop();
        }
        if (inputBindingManager != null) {
            inputBindingManager.endHostedProfile();
        }
        setShellMenuMusicActive(true);
        refreshDosModuleStatus();
    }

    @Override
    public void onDosSessionEnded(
        String message,
        boolean error
    ) {
        runOnUiThread(() -> {
            activeDosModuleId = "";
            if (menuMusic != null) {
                menuMusic.setShellActive(true);
            }
            if (inputBindingManager != null) {
                inputBindingManager.endHostedProfile();
            }
            if (renderer != null) {
                renderer.requestDosExit();
            }
            refreshDosModuleStatus();

            if (error) {
                uiFeedback.failure();
                Toast.makeText(
                    this,
                    message == null || message.trim().isEmpty()
                        ? "DOS session ended with an error."
                        : message,
                    Toast.LENGTH_LONG
                ).show();
            }
        });
    }

    private void refreshDosModuleStatus() {
        if (renderer == null
            || dosModuleRepository == null) {
            return;
        }

        java.util.List<DosGameModule> modules =
            dosModuleRepository.list();

        renderer.setDosModules(
            DosNativeRuntime.isAvailable(),
            modules
        );
    }

    @Override
    public void onVideoPlaybackRequested() {
        runOnUiThread(() -> {
            if (videoPlayer != null && preferences != null) {
                if (menuMusic != null) {
                    menuMusic.setShellActive(false);
                }
                videoPlayer.play(preferences.getSelectedVideoUri());
            }
        });
    }

    @Override
    public void onVideoTogglePauseRequested() {
        runOnUiThread(() -> {
            if (videoPlayer != null) {
                videoPlayer.togglePause();
            }
        });
    }

    @Override
    public void onVideoSeekRequested(int deltaMillis) {
        runOnUiThread(() -> {
            if (videoPlayer != null) {
                videoPlayer.seekRelative(deltaMillis);
            }
        });
    }

    @Override
    public void onVideoStopRequested() {
        runOnUiThread(() -> {
            if (videoPlayer != null) {
                videoPlayer.stop();
            }
            if (menuMusic != null) {
                menuMusic.setShellActive(true);
            }
        });
    }

    @Override
    public void onVideoPrepared(float aspectRatio) {
        if (renderer != null) {
            renderer.setVideoAspectRatio(aspectRatio);
        }
    }

    @Override
    public void onVideoStateChanged(String message) {
        // Playback state is intentionally quiet inside the headset for now.
    }

    @Override
    public void onVideoError(String message) {
        runOnUiThread(() -> {
            if (menuMusic != null) {
                menuMusic.setShellActive(true);
            }
            uiFeedback.failure();
            Toast.makeText(
                this,
                message,
                Toast.LENGTH_LONG
            ).show();
            if (renderer != null) {
                renderer.requestVideoExit();
            }
        });
    }

    private float readViewerInterLensDistance() {
        try {
            byte[] savedParams = QrCode.getSavedDeviceParams();
            if (savedParams == null) {
                return SAFE_VIEWER_FALLBACK_IPD_METERS;
            }

            CardboardDevice.DeviceParams params =
                DeviceParamsUtils.parseCardboardDeviceParams(
                    savedParams
                );
            if (params == null
                || !params.hasInterLensDistance()) {
                return SAFE_VIEWER_FALLBACK_IPD_METERS;
            }

            float value = params.getInterLensDistance();
            if (value < 0.050f || value > 0.080f) {
                return SAFE_VIEWER_FALLBACK_IPD_METERS;
            }
            return value;
        } catch (RuntimeException exception) {
            return SAFE_VIEWER_FALLBACK_IPD_METERS;
        }
    }

    private void registerHeadMotionSensor() {
        if (headMotionSensorRegistered
            || sensorManager == null
            || linearAccelerationSensor == null) {
            return;
        }

        try {
            headMotionSensorRegistered =
                sensorManager.registerListener(
                    this,
                    linearAccelerationSensor,
                    SensorManager.SENSOR_DELAY_GAME
                );
            if (!headMotionSensorRegistered) {
                ReverieLog.milestone(
                    "HEAD_MOTION",
                    "Linear-acceleration sensor registration was rejected; "
                        + "VR remains rotation-tracked only."
                );
            }
        } catch (RuntimeException exception) {
            headMotionSensorRegistered = false;
            ReverieLog.error(
                "HEAD_MOTION",
                "Linear-acceleration sensor registration failed.",
                exception
            );
        }
    }

    private void unregisterHeadMotionSensor() {
        if (!headMotionSensorRegistered
            || sensorManager == null) {
            return;
        }

        try {
            sensorManager.unregisterListener(
                this,
                linearAccelerationSensor
            );
        } catch (RuntimeException ignored) {
            // Activity teardown owns the listener lifetime.
        }
        headMotionSensorRegistered = false;
    }

    private void registerPerformanceEnvironmentMonitoring() {
        if (!batteryTemperatureReceiverRegistered) {
            try {
                Intent sticky = registerReceiver(
                    batteryTemperatureReceiver,
                    new IntentFilter(Intent.ACTION_BATTERY_CHANGED)
                );
                batteryTemperatureReceiverRegistered = true;
                updateBatteryStatus(sticky);
            } catch (RuntimeException exception) {
                batteryTemperatureTenthsC.set(
                    PerformanceEnvironmentSnapshot
                        .BATTERY_TEMPERATURE_UNAVAILABLE
                );
            }
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            if (api29ThermalMonitor == null) {
                api29ThermalMonitor =
                    new Api29ThermalMonitor(this, thermalStatus);
            }
            api29ThermalMonitor.start();
        } else {
            thermalStatus.set(
                PerformanceEnvironmentSnapshot.THERMAL_STATUS_UNAVAILABLE
            );
        }
    }

    private void unregisterPerformanceEnvironmentMonitoring() {
        if (batteryTemperatureReceiverRegistered) {
            try {
                unregisterReceiver(batteryTemperatureReceiver);
            } catch (RuntimeException ignored) {
                // Receiver state is already being discarded.
            }
            batteryTemperatureReceiverRegistered = false;
        }

        if (api29ThermalMonitor != null) {
            api29ThermalMonitor.stop();
        }
    }

    private void updateBatteryStatus(Intent intent) {
        if (intent == null
            || !Intent.ACTION_BATTERY_CHANGED.equals(intent.getAction())) {
            batteryTemperatureTenthsC.set(
                PerformanceEnvironmentSnapshot
                    .BATTERY_TEMPERATURE_UNAVAILABLE
            );
            return;
        }

        int value = intent.getIntExtra(
            BatteryManager.EXTRA_TEMPERATURE,
            PerformanceEnvironmentSnapshot.BATTERY_TEMPERATURE_UNAVAILABLE
        );
        if (value < -500 || value > 1000) {
            value =
                PerformanceEnvironmentSnapshot
                    .BATTERY_TEMPERATURE_UNAVAILABLE;
        }
        batteryTemperatureTenthsC.set(value);

        // Battery broadcasts update on real level changes. The fallback
        // BatteryManager read at lifecycle entry remains available.
        int percentage = PhoneBatteryPercentage.fromLevelAndScale(
            intent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1),
            intent.getIntExtra(BatteryManager.EXTRA_SCALE, -1)
        );
        if (percentage >= 0 && renderer != null) {
            renderer.setPhoneBattery(percentage);
        }
    }

    @TargetApi(Build.VERSION_CODES.Q)
    private static final class Api29ThermalMonitor {
        private final AtomicInteger destination;
        private final PowerManager powerManager;
        private final PowerManager.OnThermalStatusChangedListener listener;
        private boolean started;

        Api29ThermalMonitor(
            Context context,
            AtomicInteger destination
        ) {
            this.destination = destination;
            powerManager =
                (PowerManager) context.getSystemService(
                    Context.POWER_SERVICE
                );
            listener = destination::set;
        }

        void start() {
            if (started) {
                return;
            }
            if (powerManager == null) {
                destination.set(
                    PerformanceEnvironmentSnapshot
                        .THERMAL_STATUS_UNAVAILABLE
                );
                return;
            }

            try {
                destination.set(
                    powerManager.getCurrentThermalStatus()
                );
                powerManager.addThermalStatusListener(listener);
                started = true;
            } catch (RuntimeException exception) {
                destination.set(
                    PerformanceEnvironmentSnapshot
                        .THERMAL_STATUS_UNAVAILABLE
                );
            }
        }

        void stop() {
            if (started && powerManager != null) {
                try {
                    powerManager.removeThermalStatusListener(listener);
                } catch (RuntimeException ignored) {
                    // The Activity is leaving VR; stale callbacks are ignored.
                }
            }
            started = false;
            destination.set(
                PerformanceEnvironmentSnapshot
                    .THERMAL_STATUS_UNAVAILABLE
            );
        }
    }

    private int readPhoneBattery() {
        BatteryManager batteryManager =
            (BatteryManager) getSystemService(
                Context.BATTERY_SERVICE
            );
        if (batteryManager == null) {
            return -1;
        }

        int percentage =
            batteryManager.getIntProperty(
                BatteryManager.BATTERY_PROPERTY_CAPACITY
            );
        return percentage >= 0 && percentage <= 100
            ? percentage
            : -1;
    }

    private void enterImmersiveMode() {
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        );
    }
}
