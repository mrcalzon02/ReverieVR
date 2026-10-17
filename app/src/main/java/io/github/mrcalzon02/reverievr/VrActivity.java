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
    private HomeAmbientAudio homeAmbientAudio;
    private LocalVideoPlayer videoPlayer;
    private final java.util.concurrent.ExecutorService calibratedMediaExecutor =
        java.util.concurrent.Executors.newSingleThreadExecutor();
    private final AtomicInteger calibratedMediaEpoch = new AtomicInteger();
    private final java.util.concurrent.ExecutorService emulatorSourceExecutor =
        java.util.concurrent.Executors.newSingleThreadExecutor();
    private final java.util.concurrent.ConcurrentHashMap<String,String>
        emulatorSourceStatuses = new java.util.concurrent.ConcurrentHashMap<>();
    private EmulatorSourceRepository emulatorSourceRepository;
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
    private volatile long dosControllerMouseLastPoseNanos;
    private float dosControllerMousePreviousYaw;
    private float dosControllerMousePreviousPitch;
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
    private volatile String pendingControllerSource = "";
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
        homeAmbientAudio = new HomeAmbientAudio();
        videoPlayer = new LocalVideoPlayer(this, this);
        dosModuleRepository =
            new DosModuleRepository(this);
        bundledDosContentInstaller =
            new BundledDosContentInstaller(
                this,
                dosModuleRepository
            );
        emulatorSourceRepository =
            new EmulatorSourceRepository(getFilesDir());
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
                getAssets(),
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
        // Cardboard's two stock overlay buttons are touchscreen-only.
        // VR deliberately consumes physical touchscreen input to avoid
        // Daydream headset clamp contacts; retain stereo alignment marker,
        // remove only the inaccessible close/gear buttons.
        cardboardView.post(this::hideCardboardTouchControls);

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
        menuMusic.setLibraryRefresh(() -> {
            if (renderer != null) renderer.requestMusicUiRefresh();
        });
        refreshDosModuleStatus();

        VrStartupGuard.setPhase(
            this,
            "cardboard-renderer-bind"
        );
        cardboardView.setRenderer(renderer);
        cardboardRendererBound = true;
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
        refreshKeyboardTwinStatus();
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
        if (homeAmbientAudio != null) homeAmbientAudio.resumeForLifecycle();
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
        if (renderer != null) renderer.setPhysicalKeyboardAttached(false);
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
        if (homeAmbientAudio != null) homeAmbientAudio.pauseForLifecycle();
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
        calibratedMediaEpoch.incrementAndGet();
        calibratedMediaExecutor.shutdownNow();
        emulatorSourceExecutor.shutdownNow();
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
        if (homeAmbientAudio != null) homeAmbientAudio.close();
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
    public void onDosDeskKeyRequested(VirtualKey key, boolean shift) {
        if (key == null || renderer == null || !renderer.isDosGameControlFocused()
            || isGuestInputSuppressed() || virtualInputBus == null) return;
        if (shift) virtualInputBus.applyDigital(VirtualOutput.key(VirtualKey.SHIFT), true);
        virtualInputBus.applyDigital(VirtualOutput.key(key), true);
        virtualInputBus.applyDigital(VirtualOutput.key(key), false);
        if (shift) virtualInputBus.applyDigital(VirtualOutput.key(VirtualKey.SHIFT), false);
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
        if (event == null || preferences == null) return false;
        boolean steam=AndroidGamepadSupport.isOriginalSteamController(event.getDevice());
        boolean gamepad=AndroidGamepadSupport.isGamepadEvent(event);
        // External keyboards still type into the desk editor independently
        // of the selected gamepad; the Steam HID itself is a controller.
        if(!gamepad && !steam)return false;
        String choice = preferences.getActiveController();
        boolean sourceAllowed = ControllerSourcePolicy.acceptsGamepad(
            choice,
            controllerManager!=null && controllerManager.isReady(),
            steam,
            AndroidGamepadSupport.isScrcpyHid(event.getDevice()),
            AndroidGamepadSupport.isScrcpyCompatibleGamepad(event.getDevice()),
            gamepad,
            AndroidGamepadSupport.firstConnectedSteamController()!=null);
        if (!sourceAllowed) return true;
        // A category is not an individual primary controller. On the
        // Android route, admit only the exact user-picked physical device.
        if (ReveriePreferences.CONTROLLER_ANDROID.equals(choice)) {
            String descriptor=event.getDevice()==null ? null
                : event.getDevice().getDescriptor();
            return !ControllerDeviceSelection.accepts(
                preferences.getSelectedAndroidGamepadDescriptor(),
                descriptor,AndroidGamepadSupport.connectedGamepadDescriptors());
        }
        // AUTO arbitration also selects one Android gamepad, not every
        // gamepad that Android happened to enumerate.
        if (ReveriePreferences.CONTROLLER_AUTO.equals(choice) && gamepad
            && !steam) {
            String descriptor=event.getDevice()==null ? null
                : event.getDevice().getDescriptor();
            return !ControllerDeviceSelection.accepts(
                AndroidGamepadSupport.firstConnectedGamepadDescriptor(),
                descriptor,AndroidGamepadSupport.connectedGamepadDescriptors());
        }
        return false;
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
        boolean controllerInput = ExternalHidInputPolicy.controllerOwnsHid(
            gamepad,
            AndroidGamepadSupport.isOriginalSteamController(device),
            mouse,
            mouse && device != null && selectedShellMouse(device.getName()));
        return ExternalHidInputPolicy.shouldConsume(false, keyboard, mouse,
            controllerInput, recovery);
    }

    @Override
    public void onExternalHidInputPolicyChanged(boolean enabled) {
        refreshKeyboardTwinStatus();
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

        int eventAction = event.getAction();
        if (eventAction != KeyEvent.ACTION_DOWN
            && eventAction != KeyEvent.ACTION_UP) return false;
        boolean down = eventAction == KeyEvent.ACTION_DOWN;
        VirtualKey physicalKey =
            StandardHidInputRouter.mapKey(event.getKeyCode());
        int action = VrKeyboardEditor.EXTERNAL_NO_TEXT;
        String characters = null;
        if (down) {
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_SHIFT_LEFT:
                case KeyEvent.KEYCODE_SHIFT_RIGHT:
                    break; // physical modifier reflected, not sticky
                case KeyEvent.KEYCODE_CAPS_LOCK:
                    if (event.getRepeatCount() == 0)
                        action = VrKeyboardEditor.EXTERNAL_CAPS_TOGGLE;
                    break;
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
                    if (codePoint >= 32
                        && Character.isValidCodePoint(codePoint)) {
                        action = VrKeyboardEditor.EXTERNAL_TEXT;
                        characters = new String(Character.toChars(codePoint));
                    }
                    break;
            }
        }
        // Preserve shell/guest bindings for keys not represented by this
        // editor. Never synthesize Android KeyEvents or echo back into HID.
        if (action == VrKeyboardEditor.EXTERNAL_NO_TEXT
            && HomeKeyboardTwin.keyIndex(physicalKey) < 0) return false;
        return renderer.queuePhysicalKeyboardEdit(
            action, characters, physicalKey, down, event.getDeviceId());
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
        // Ordinary USB/Bluetooth mouse-wheel input scrolls the DOS shelf.
        // Leave guest mouse input untouched outside that menu.
        if (event != null
            && event.getActionMasked() == MotionEvent.ACTION_SCROLL
            && renderer != null
            && renderer.scrollDosLibraryWheel(
                event.getAxisValue(MotionEvent.AXIS_VSCROLL))) {
            return true;
        }
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
            hideCardboardTouchControls();
        }
    }

    @Override
    public void onConnectionStateChanged(
        ControllerProvider.ConnectionState state,
        String message
    ) {
        controllerConnectionMessage =
            message == null ? "" : message;

        if (state != ControllerProvider.ConnectionState.READY) {
            if (inputBindingEngine != null) inputBindingEngine.releaseAll();
            if (inputRouter != null) inputRouter.resetControllerEdges();
            // Do not carry the previous controller-mouse motion forward
            // when a connection is restored and a new pose arrives.
            dosControllerMouseLastPoseNanos = 0L;
        }
        if(state==ControllerProvider.ConnectionState.READY &&
           ReveriePreferences.CONTROLLER_DAYDREAM.equals(pendingControllerSource) &&
           "daydream_ble".equals(controllerManager.getActiveProviderId())) {
            pendingControllerSource="";
            applyControllerSource(ReveriePreferences.CONTROLLER_DAYDREAM);
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
        if (inputRouter == null || snapshot == null ||
            !ControllerSourcePolicy.acceptsProvider(
                preferences.getActiveController(),
                controllerManager.getActiveProviderId())) return;

        if (renderer != null) {
            renderer.setControllerPose(snapshot,
                "daydream_ble".equals(controllerManager.getActiveProviderId()));
        }

        routeDosControllerMouse(snapshot);
        inputRouter.onControllerSnapshot(
            snapshot,
            controllerManager.getActiveProviderDisplayName()
        );
    }

    // Emit relative cursor motion only while the controller-mouse DOS
    // profile is active. Dropped samples, menu entry and tracking loss rearm.
    private void routeDosControllerMouse(ControllerSnapshot pose) {
        if (inputBindingManager == null || inputBindingEngine == null
            || activeDosModuleId == null || activeDosModuleId.isEmpty()
            || isGuestInputSuppressed()
            || !BuiltInBindingProfiles.ID_DOS_CONTROLLER_MOUSE.equals(
                inputBindingManager.getProfile().id)) {
            dosControllerMouseLastPoseNanos = 0L;
            return;
        }
        long time = pose.receivedAtNanos;
        float x = pose.orientationX, y = pose.orientationY;
        float z = pose.orientationZ, w = pose.orientationW;
        float norm = x*x+y*y+z*z+w*w;
        if (!Float.isFinite(norm) || norm < 0.25f || norm > 2.25f
            || time <= 0L) {
            dosControllerMouseLastPoseNanos = 0L;
            return;
        }
        float inv = (float)(1.0/Math.sqrt(norm));
        x *= inv; y *= inv; z *= inv; w *= inv;
        float forwardY = -2.0f*(y*z-w*x);
        // At vertical pointing the yaw angle is undefined. Rearm.
        if (Math.abs(forwardY) > 0.985f) {
            dosControllerMouseLastPoseNanos = 0L;
            return;
        }
        float yaw = (float)Math.atan2(2.0f*(x*z+w*y),
            1.0f-2.0f*(x*x+y*y));
        float pitch = (float)Math.asin(Math.max(-1.0f,
            Math.min(1.0f,forwardY)));
        if (dosControllerMouseLastPoseNanos > 0L
            && time > dosControllerMouseLastPoseNanos
            && time-dosControllerMouseLastPoseNanos < 250000000L) {
            float dyaw = yaw-dosControllerMousePreviousYaw;
            if (dyaw > Math.PI) dyaw -= (float)(2.0*Math.PI);
            if (dyaw < -Math.PI) dyaw += (float)(2.0*Math.PI);
            float dpitch = pitch-dosControllerMousePreviousPitch;
            if (Math.abs(dyaw)<0.35f && Math.abs(dpitch)<0.35f) {
                inputBindingEngine.submitRelative(
                    BindingInput.CONTROLLER_YAW_DELTA,dyaw);
                inputBindingEngine.submitRelative(
                    BindingInput.CONTROLLER_PITCH_DELTA,dpitch);
            }
        }
        dosControllerMousePreviousYaw = yaw;
        dosControllerMousePreviousPitch = pitch;
        dosControllerMouseLastPoseNanos = time;
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
    public boolean usesDosShellMenuNavigation() {
        return renderer != null && renderer.isDosOverlayNavigationActive();
    }

    @Override
    public boolean usesDosLibraryTouchScroll() {
        return renderer != null && renderer.isDosLibraryScrollable();
    }

    @Override
    public void onDosLibraryTouchScroll(float deltaY, boolean ended) {
        if (renderer != null) {
            renderer.scrollDosLibraryTouch(deltaY, ended);
        }
    }

    @Override
    public boolean usesDaydreamDosGameControls() {
        return renderer != null && renderer.isDosGameControlFocused();
    }

    @Override
    public boolean shouldDispatchGamepadShellAction(
        int keyCode, VrInputAction action) {
        return !(action == VrInputAction.BACK
            && keyCode == KeyEvent.KEYCODE_BUTTON_B
            && renderer != null && renderer.isHostedGamepadInputActive());
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
        if (action == VrInputAction.NAV_LEFT
            || action == VrInputAction.NAV_RIGHT
            || action == VrInputAction.NAV_UP
            || action == VrInputAction.NAV_DOWN
            || action == VrInputAction.MENU) {
            renderer.noteControlEvent(action.name().replace('_', ' '));
        }

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
                if (!renderer.requestDosMenuNavigation(VrInputAction.NAV_UP)
                    && !renderer.requestDosLibraryNavigation(VrInputAction.NAV_UP)
                    && !renderer.requestDosLibraryScroll(VrInputAction.NAV_UP)) {
                    renderer.requestPointerNavigation(VrInputAction.NAV_UP);
                }
                break;

            case NAV_DOWN:
                if (!renderer.requestDosMenuNavigation(VrInputAction.NAV_DOWN)
                    && !renderer.requestDosLibraryNavigation(VrInputAction.NAV_DOWN)
                    && !renderer.requestDosLibraryScroll(VrInputAction.NAV_DOWN)) {
                    renderer.requestPointerNavigation(VrInputAction.NAV_DOWN);
                }
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
        if (renderer != null && input != null
            && (down || input == BindingInput.DAYDREAM_PRESS_HOLD)) {
            renderer.noteControlEvent(
                input.name().replace('_', ' ')
                + (down ? "" : " RELEASE"));
        }
        // Looking at an in-room DOS shell button blocks only gameplay
        // Select. Continuous head-view mouse axes, pad directions and
        // other held guest controls stay live without a mouse click.
        if (isGuestInputSuppressed()
            || (input == BindingInput.SELECT && renderer != null
                && renderer.isDosShellControlFocused())) {
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
    public void onBindingRelative(BindingInput input, float delta,
                                  String source) {
        if (inputBindingEngine != null && !isGuestInputSuppressed())
            inputBindingEngine.submitRelative(input, delta);
    }

    private int lastExtremeHead;
    private int lastHeadRoll;
    private long lastSharpTurnAtNanos;

    @Override
    public void onHeadBindingRollAxis(float rollRadians) {
        // Signed roll until S9 calibration establishes left/right polarity.
        // No physical head-view transform is modified.
        if (Math.abs(rollRadians) < 0.17f) lastHeadRoll = 0;
        int direction = rollRadians >= 0.29f ? 1
            : rollRadians <= -0.29f ? -1 : 0;
        if (direction != 0 && direction != lastHeadRoll) {
            lastHeadRoll = direction;
            BindingInput event = direction > 0
                ? BindingInput.HEAD_ROLL_POSITIVE
                : BindingInput.HEAD_ROLL_NEGATIVE;
            if (renderer != null)
                renderer.noteControlEvent(event.name().replace('_', ' '));
            if (inputBindingEngine != null && !isGuestInputSuppressed())
                inputBindingEngine.pulse(event);
        }
    }

    @Override
    public void onHeadBindingUpAxis(float upAxis) {
        int extreme = upAxis >= 0.87f ? 1 :
            upAxis <= -0.87f ? -1 : 0;
        // Re-arm below about 50 degrees, preventing noise near the peak.
        if (Math.abs(upAxis) < 0.76f) lastExtremeHead = 0;
        if (extreme != 0 && extreme != lastExtremeHead) {
            lastExtremeHead = extreme;
            BindingInput event = extreme > 0
                ? BindingInput.HEAD_LOOK_EXTREME_UP
                : BindingInput.HEAD_LOOK_EXTREME_DOWN;
            if (renderer != null)
                renderer.noteControlEvent(event.name().replace('_', ' '));
            if (inputBindingEngine != null && !isGuestInputSuppressed())
                inputBindingEngine.pulse(event);
        }
        // Only the bundled Doom profile interprets absolute vertical gaze
        // as held movement/menu arrows. Other DOS games remain unmodified.
        if (inputBindingEngine == null
            || !BundledDosContentInstaller.DOOM_MODULE_ID.equals(
                activeDosModuleId)) {
            return;
        }
        inputBindingEngine.submitAxis(
            BindingInput.HEAD_PITCH_UP_AXIS,
            isGuestInputSuppressed() ? 0.0f : upAxis);
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

        // Sharp head turns are visible as diagnostics, not automatically
        // bound to a game action or the headset recenter command.
        final long now = System.nanoTime();
        if (Math.abs(yawDeltaRadians) > 0.16f
            && now - lastSharpTurnAtNanos > 850000000L) {
            lastSharpTurnAtNanos = now;
            BindingInput event = yawDeltaRadians > 0
                ? BindingInput.HEAD_SHARP_RIGHT : BindingInput.HEAD_SHARP_LEFT;
            if (renderer != null)
                renderer.noteControlEvent(event.name().replace('_', ' '));
            inputBindingEngine.pulse(event);
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
        if (renderer != null)
            renderer.releasePhysicalKeyboardDevice(deviceId);
        if (inputBindingEngine != null) inputBindingEngine.releaseAll();
        if (standardHidInputRouter != null) standardHidInputRouter.reset();
        if (inputRouter != null) inputRouter.resetControllerEdges();
        dosControllerMouseLastPoseNanos = 0L;
        if (preferences != null) {
            String selected=preferences.getActiveController();
            boolean lost=
                ReveriePreferences.CONTROLLER_SCRCPY.equals(selected) &&
                    AndroidGamepadSupport.firstConnectedScrcpySelectableInput()==null
                || ReveriePreferences.CONTROLLER_STEAM.equals(selected) &&
                    AndroidGamepadSupport.firstConnectedSteamController()==null
                || ReveriePreferences.CONTROLLER_ANDROID.equals(selected) &&
                    !ControllerDeviceSelection.contains(
                        AndroidGamepadSupport.connectedGamepadDescriptors(),
                        preferences.getSelectedAndroidGamepadDescriptor());
            if(lost) {
                // Never strand the headset in a disconnected explicit mode.
                pendingControllerSource="";
                preferences.setActiveController(ReveriePreferences.CONTROLLER_AUTO);
                if (controllerManager != null &&
                    controllerManager.getMissingRuntimePermissions().length==0)
                    controllerManager.pairDaydreamController();
                ReverieLog.incident("VR_CONTROLLER",
                    "Explicit controller disconnected; reverted to Auto.");
            }
        }
        refreshInputSourceStatus();
    }

    @Override
    public void onInputDeviceChanged(int deviceId) {
        refreshInputSourceStatus();
    }

    private void refreshKeyboardTwinStatus() {
        if (renderer == null || preferences == null) return;
        boolean attached = false;
        for (int deviceId : android.view.InputDevice.getDeviceIds()) {
            android.view.InputDevice device =
                android.view.InputDevice.getDevice(deviceId);
            if (device == null || device.isVirtual()
                || AndroidGamepadSupport.isOriginalSteamController(device))
                continue;
            int source = device.getSources();
            boolean keyboard =
                (source & android.view.InputDevice.SOURCE_KEYBOARD)
                    == android.view.InputDevice.SOURCE_KEYBOARD;
            boolean gamepad =
                (source & android.view.InputDevice.SOURCE_GAMEPAD)
                    == android.view.InputDevice.SOURCE_GAMEPAD
                || (source & android.view.InputDevice.SOURCE_JOYSTICK)
                    == android.view.InputDevice.SOURCE_JOYSTICK;
            if (keyboard && !gamepad) {
                attached = true;
                break;
            }
        }
        renderer.setPhysicalKeyboardAttached(
            attached && preferences.isExternalHidEnabled());
    }

    private void refreshInputSourceStatus() {
        refreshKeyboardTwinStatus();
        if (renderer == null || controllerManager == null) return;
        String selected = preferences.getActiveController();
        boolean provider = controllerManager.isReady() &&
            ControllerSourcePolicy.acceptsProvider(
                selected,controllerManager.getActiveProviderId());
        String gamepad = ReveriePreferences.CONTROLLER_ANDROID.equals(selected)
            ? AndroidGamepadSupport.gamepadNameByDescriptor(
                preferences.getSelectedAndroidGamepadDescriptor())
            : AndroidGamepadSupport.gamepadNameByDescriptor(
                AndroidGamepadSupport.firstConnectedGamepadDescriptor());
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
                    && !provider);
        boolean scrcpyPrimary = scrcpy != null && (
            ReveriePreferences.CONTROLLER_SCRCPY.equals(selected)
                || ReveriePreferences.CONTROLLER_AUTO.equals(selected)
                    && !provider && !steamPrimary
                    && (gamepad == null || ScrcpyHidIdentity.matchesName(gamepad)));
        String connectedVrController = steamPrimary ? steam
            : scrcpyPrimary ? scrcpy
            : ReveriePreferences.CONTROLLER_ANDROID.equals(selected)
                ? gamepad
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
                        ? "Selected scrcpy HID disconnected"
                    : ReveriePreferences.CONTROLLER_ANDROID.equals(selected)
                        ? "Selected Android gamepad disconnected"
                    : "Selected "+ControllerSourcePolicy.label(selected)
                        +": "+controllerConnectionMessage
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

    @Override
    public String getControllerSourceStatus(String choice) {
        if(ReveriePreferences.CONTROLLER_AUTO.equals(choice))return "ALWAYS AVAILABLE";
        if(ReveriePreferences.CONTROLLER_DAYDREAM.equals(choice))
            return ReveriePreferences.CONTROLLER_DAYDREAM.equals(pendingControllerSource)
                ? "PAIRING BLE - PREVIOUS SOURCE ACTIVE"
                : controllerManager!=null && controllerManager.isReady() &&
                  "daydream_ble".equals(controllerManager.getActiveProviderId())
                ? "CONNECTED" : "PAIR WHEN SELECTED";
        if(ReveriePreferences.CONTROLLER_STEAM.equals(choice))
            return AndroidGamepadSupport.firstConnectedSteamController()!=null
                ? "CONNECTED" : "NOT DETECTED";
        if(ReveriePreferences.CONTROLLER_SCRCPY.equals(choice))
            return AndroidGamepadSupport.firstConnectedScrcpySelectableInput()!=null
                ? "FORWARDED DEVICE READY" : "START SCRCPY --GAMEPAD=UHID";
        if(ReveriePreferences.CONTROLLER_ANDROID.equals(choice)) {
            String[] available = AndroidGamepadSupport.connectedGamepadDescriptors();
            String active = preferences.getSelectedAndroidGamepadDescriptor();
            String name = AndroidGamepadSupport.gamepadNameByDescriptor(active);
            return available.length == 0 ? "NO UNAMBIGUOUS GAMEPAD FOUND"
                : name == null ? available.length + " GAMEPAD(S): TAP TO SELECT"
                : name + " (" + available.length + "): TAP TO CYCLE";
        }
        if(ReveriePreferences.CONTROLLER_PHONE.equals(choice))
            return controllerManager!=null && controllerManager.isReady() &&
                "daydream_phone_emulator".equals(controllerManager.getActiveProviderId())
                ? "CONNECTED" : "PAIR VIA PHONE FIRST";
        return "UNKNOWN";
    }

    @Override
    public boolean onControllerSourceSelected(String choice) {
        if (preferences==null || controllerManager==null ||
            !java.util.Arrays.asList(ControllerSourcePolicy.CHOICES).contains(choice))
            return false;
        if (ReveriePreferences.CONTROLLER_ANDROID.equals(choice)) {
            String[] connected = AndroidGamepadSupport.connectedGamepadDescriptors();
            if (connected.length == 0) return false;
            String old = preferences.getSelectedAndroidGamepadDescriptor();
            String next = ReveriePreferences.CONTROLLER_ANDROID.equals(
                    preferences.getActiveController())
                ? ControllerDeviceSelection.next(connected, old)
                : ControllerDeviceSelection.retainOrFirst(connected, old);
            preferences.setSelectedAndroidGamepadDescriptor(next);
            applyControllerSource(choice); // atomically release previous pressed inputs
            return true;
        }
        if(choice.equals(preferences.getActiveController())) return true;
        if(ReveriePreferences.CONTROLLER_STEAM.equals(choice) &&
           AndroidGamepadSupport.firstConnectedSteamController()==null)return false;
        if(ReveriePreferences.CONTROLLER_SCRCPY.equals(choice) &&
           AndroidGamepadSupport.firstConnectedScrcpySelectableInput()==null)return false;
        if(ReveriePreferences.CONTROLLER_ANDROID.equals(choice) &&
           !AndroidGamepadSupport.hasConnectedGamepad())return false;
        if(ReveriePreferences.CONTROLLER_PHONE.equals(choice) &&
           (!controllerManager.isReady() ||
            !"daydream_phone_emulator".equals(controllerManager.getActiveProviderId()))) {
            // Phone-emulator pairing requires Stage A, not an inert VR choice.
            onControllerPairingRequested();
            return true;
        }
        if(ReveriePreferences.CONTROLLER_DAYDREAM.equals(choice) &&
           (!controllerManager.isReady() ||
            !"daydream_ble".equals(controllerManager.getActiveProviderId()))) {
            if(controllerManager.getMissingRuntimePermissions().length>0) {
                onControllerPairingRequested();
                return true;
            }
            pendingControllerSource=choice;
            runOnUiThread(() -> controllerManager.pairDaydreamController());
            return true;
        }
        pendingControllerSource="";
        applyControllerSource(choice);
        return true;
    }

    private void applyControllerSource(String choice) {
        if(inputBindingEngine!=null)inputBindingEngine.releaseAll();
        if(standardHidInputRouter!=null)standardHidInputRouter.reset();
        if(inputRouter!=null)inputRouter.resetControllerEdges();
        preferences.setActiveController(choice);
        if (!ReveriePreferences.CONTROLLER_DAYDREAM.equals(choice) &&
            !ReveriePreferences.CONTROLLER_PHONE.equals(choice) &&
            renderer!=null) renderer.clearTrackedControllerPose();
        if(ReveriePreferences.CONTROLLER_STEAM.equals(choice) ||
           ReveriePreferences.CONTROLLER_SCRCPY.equals(choice) ||
           ReveriePreferences.CONTROLLER_ANDROID.equals(choice))
            controllerManager.deselectProvider();
        ReverieLog.milestone("VR_CONTROLLER",
            "Explicit Stage B controller source="+choice);
        refreshInputSourceStatus();
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
    public void onNativeModuleSoundCueRequested(int cueId) {
        if(uiFeedback==null || cueId<1 || cueId>15)return;
        runOnUiThread(() -> uiFeedback.nativeSoundCue(cueId));
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
            if ((safeFlags & NativeModuleRuntime.FEEDBACK_ARTILLERY) != 0) {
                uiFeedback.artilleryStrike();
            }
            if ((safeFlags & NativeModuleRuntime.FEEDBACK_AIRSTRIKE) != 0) {
                uiFeedback.airstrike();
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

    @Override
    public String[] getNativeMusicTrackNames() {
        return menuMusic == null
            ? new String[0] : menuMusic.trackNames();
    }

    @Override
    public String getNativeMusicStatus() {
        return menuMusic == null
            ? "MUSIC UNAVAILABLE" : menuMusic.playbackStatus();
    }

    @Override
    public int getNativeMusicSelectedIndex() {
        return menuMusic == null ? -1 : menuMusic.trackIndex();
    }

    @Override
    public void onNativeMusicChoose(int index) {
        runOnUiThread(() -> {
            if (menuMusic != null) menuMusic.chooseTrack(index);
            if (renderer != null) renderer.requestMusicUiRefresh();
        });
    }

    @Override
    public void onNativeMusicStep(int direction) {
        runOnUiThread(() -> {
            if (menuMusic != null) menuMusic.stepTrack(direction);
            if (renderer != null) renderer.requestMusicUiRefresh();
        });
    }

    @Override
    public void onNativeMusicTogglePause() {
        runOnUiThread(() -> {
            if (menuMusic != null) menuMusic.toggleUserPause();
            if (renderer != null) renderer.requestMusicUiRefresh();
        });
    }

    @Override
    public String getNativeMusicVolumeStatus() {
        return menuMusic == null
            ? "MUSIC UNAVAILABLE" : menuMusic.musicVolumeStatus();
    }

    @Override
    public void onNativeMusicVolumeAdjust(int direction) {
        runOnUiThread(() -> {
            if (menuMusic != null) menuMusic.adjustMusicVolume(direction);
            if (renderer != null) renderer.requestMusicUiRefresh();
        });
    }

    @Override
    public void onNativeMusicToggleMute() {
        runOnUiThread(() -> {
            if (menuMusic != null) menuMusic.toggleMusicMute();
            if (renderer != null) renderer.requestMusicUiRefresh();
        });
    }

    @Override
    public void onHomeAtmosphereChanged(HomeEnvironment environment,
                                        boolean homeActive) {
        // Never create/pause/release Android AudioTrack on Cardboard's GL thread.
        runOnUiThread(() -> {
            if(homeAmbientAudio != null)
                homeAmbientAudio.setEnvironment(environment, homeActive);
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
    public void onEmulatorCatalogOpened() {
        // GitHub queries happen off the renderer; merely opening the list
        // never downloads sources or executables.
        emulatorSourceExecutor.execute(() -> {
            for(EmulatorModuleCatalog.Entry entry:EmulatorModuleCatalog.entries()){
                String current=emulatorSourceStatuses.get(entry.id);
                if(current!=null && (current.startsWith("DOWNLOADING") ||
                                     current.startsWith("CHECKING UPSTREAM")))
                    continue;
                try {
                    emulatorSourceStatuses.put(entry.id,
                        emulatorSourceRepository.inspectUpstream(entry));
                }catch(java.io.IOException e){
                    emulatorSourceStatuses.put(entry.id,
                        "SOURCE UNAVAILABLE: "+e.getMessage());
                }
            }
        });
    }

    @Override
    public String getEmulatorSourceStatus(String moduleId) {
        final EmulatorModuleCatalog.Entry entry =
            EmulatorModuleCatalog.find(moduleId);
        if (entry==null)return "UNAPPROVED MODULE";
        String status=emulatorSourceStatuses.get(moduleId);
        return status==null ? "SOURCE NOT DOWNLOADED" : status;
    }

    @Override
    public void onEmulatorSourceDownloadRequested(String moduleId) {
        final EmulatorModuleCatalog.Entry entry =
            EmulatorModuleCatalog.find(moduleId);
        if (entry==null || emulatorSourceRepository==null) {
            runOnUiThread(() -> uiFeedback.failure());
            return;
        }
        String prior=emulatorSourceStatuses.putIfAbsent(moduleId,"CHECKING UPSTREAM");
        if(prior!=null && (prior.startsWith("CHECKING") ||
                           prior.startsWith("DOWNLOADING"))) {
            runOnUiThread(() -> uiFeedback.failure());
            return;
        }
        emulatorSourceStatuses.put(moduleId,"CHECKING UPSTREAM");
        emulatorSourceExecutor.execute(() -> {
            try {
                emulatorSourceStatuses.put(moduleId,"DOWNLOADING SOURCE");
                EmulatorSourceRepository.Result result=
                    emulatorSourceRepository.download(entry);
                emulatorSourceStatuses.put(moduleId,result.message);
                ReverieLog.milestone("EMULATOR_SOURCES",
                    "Source fetched by explicit VR approval; module="+moduleId+
                    " commit="+result.commit+" sha256="+result.sha256);
                runOnUiThread(() -> uiFeedback.activation());
            } catch(Exception ex) {
                emulatorSourceStatuses.put(moduleId,
                    "FAILED: "+ex.getMessage());
                ReverieLog.incident("EMULATOR_SOURCES",
                    "Source fetch failed for "+moduleId+": "+ex.getMessage());
                runOnUiThread(() -> uiFeedback.failure());
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
            BuiltInBindingProfiles.nativeProfileForModule(moduleId);

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

        runOnUiThread(() -> {
            if (menuMusic != null)
                menuMusic.setHostedNativeModule(moduleId);
        });
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
            && (BundledDosContentInstaller.DOOM_MODULE_ID.equals(module.id)
                || BundledDosArchiveCatalog.findById(module.id) != null)
            && bundledDosContentInstaller != null) {
            try {
                module =
                    bundledDosContentInstaller
                        .prepareForLaunch(module);
            } catch (java.io.IOException exception) {
                ReverieLog.error(
                    "BUNDLED_CONTENT",
                    "Could not prepare bundled DOS game for launch.",
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
        dosControllerMouseLastPoseNanos = 0L;
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
    public void onDosGuestInputFocusLost() {
        // Also clears pending quick taps from VirtualInputBus: suppressing
        // future input without releasing an existing press leaves a DOS
        // mouse button stuck when the player targets the options overlay.
        dosControllerMouseLastPoseNanos = 0L;
        if (inputBindingEngine != null) {
            inputBindingEngine.releaseAll();
        }
    }

    @Override
    public void onDosOverlayPauseRequested() {
        onDosGuestInputFocusLost();
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
        onDosBindingProfileSelected(profiles.get(next).id);
    }

    @Override
    public void onDosBindingProfileSelected(String profileId) {
        if (dosModuleRepository == null || inputBindingManager == null
            || !inputBindingManager.isHostedProfileActive()
            || !BuiltInBindingProfiles.isDosProfileId(profileId)) return;
        DosGameModule module = activeDosModule();
        if (module == null
            || !dosModuleRepository.updateBindingProfileId(module.id,profileId))
            return;
        inputBindingManager.setHostedProfile(
            BuiltInBindingProfiles.byId(profileId));
        dosControllerMouseLastPoseNanos = 0L;
        ReverieLog.milestone("DOS_BINDING",
            "Module="+module.id+" selected profile="+profileId);
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
        dosControllerMouseLastPoseNanos = 0L;
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
                final String uri = preferences.getSelectedVideoUri();
                final int requestId = calibratedMediaEpoch.incrementAndGet();
                if (renderer != null) renderer.setCalibratedVideoMesh(null);
                if (preferences.getVideoProjection()
                        == VideoProjection.CALIBRATED_MESH_VR180) {
                    calibratedMediaExecutor.execute(
                        () -> loadCalibratedVideoMesh(uri, requestId));
                }
                if (menuMusic != null) {
                    menuMusic.setShellActive(false);
                }
                videoPlayer.play(uri);
            }
        });
    }

    private void loadCalibratedVideoMesh(String selectedUri, int requestId) {
        if (requestId != calibratedMediaEpoch.get()) return;
        SpatialMp4Metadata.Result detected = null;
        try {
            android.net.Uri uri = android.net.Uri.parse(selectedUri);
            try (android.os.ParcelFileDescriptor descriptor =
                     getContentResolver().openFileDescriptor(uri, "r")) {
                if (descriptor != null) {
                    try (java.io.FileInputStream in =
                             new java.io.FileInputStream(
                                 descriptor.getFileDescriptor())) {
                        detected = SpatialMp4Metadata.inspect(in.getChannel());
                    }
                }
            } catch (java.io.IOException | UnsupportedOperationException
                     notSeekable) {
                // SAF pipe-backed providers need bounded prefix inspection.
            }
            if (detected == null
                || detected.status == SpatialMp4Metadata.Status.UNKNOWN) {
                try (java.io.InputStream input =
                         getContentResolver().openInputStream(uri)) {
                    detected = SpatialMp4Metadata.inspect(input);
                }
            }
            final SpatialMp4Metadata.Result result = detected;
            runOnUiThread(() -> {
                if (requestId != calibratedMediaEpoch.get()
                    || isFinishing() || isDestroyed()
                    || !selectedUri.equals(preferences.getSelectedVideoUri())
                    || preferences.getVideoProjection()
                        != VideoProjection.CALIBRATED_MESH_VR180) return;
                if (result != null
                    && result.status == SpatialMp4Metadata.Status.SUPPORTED
                    && result.calibratedMesh != null) {
                    renderer.setCalibratedVideoMesh(result.calibratedMesh);
                } else {
                    if (videoPlayer != null) videoPlayer.stop();
                    onVideoError("Calibrated VR180 mesh unavailable or invalid.");
                }
            });
        } catch (java.io.IOException | SecurityException problem) {
            ReverieLog.error("VR180_MESH",
                "Could not read selected camera calibration", problem);
            runOnUiThread(() -> {
                if (requestId != calibratedMediaEpoch.get()
                    || isFinishing() || isDestroyed()) return;
                if (videoPlayer != null) videoPlayer.stop();
                onVideoError("Unable to read the VR180 camera calibration.");
            });
        }
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
        calibratedMediaEpoch.incrementAndGet();
        runOnUiThread(() -> {
            if (renderer != null) renderer.setCalibratedVideoMesh(null);
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

    private void hideCardboardTouchControls() {
        if (cardboardView == null) return;
        // Overlay is owned by CardboardView; its own stereo setup queues
        // a showAll(), so hide on the UI event queue after initialization.
        View close = cardboardView.findViewById(
            com.google.cardboard.sdk.R.id.ui_back_button);
        View options = cardboardView.findViewById(
            com.google.cardboard.sdk.R.id.ui_settings_button);
        if (close != null) close.setVisibility(View.GONE);
        if (options != null) options.setVisibility(View.GONE);
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
