package io.github.mrcalzon02.reverievr;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.graphics.SurfaceTexture;
import android.hardware.input.InputManager;
import android.media.AudioManager;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
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
        DosSession.Listener {

    private static final float SAFE_VIEWER_FALLBACK_IPD_METERS = 0.060f;

    private CardboardView cardboardView;
    private VrShellRenderer renderer;
    private ControllerManager controllerManager;
    private ReveriePreferences preferences;
    private UiFeedback uiFeedback;
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
                updateBatteryTemperature(intent);
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

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        enterImmersiveMode();

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
                virtualInputBus
            );
        inputManager =
            (InputManager) getSystemService(Context.INPUT_SERVICE);
        inputRouter = new VrInputRouter(this);
        inputRouter.setBindingListener(this);

        CardboardView.setUseCardboardGlSurfaceView(true);
        cardboardView = new CardboardView(this);
        cardboardView.setStereoRenderMode(true);

        float viewerIpd = readViewerInterLensDistance();

        renderer =
            new VrShellRenderer(
                preferences,
                viewerIpd,
                dosSession,
                nativeModuleRuntime,
                this
            );
        renderer.setPhoneBattery(readPhoneBattery());
        refreshDosModuleStatus();

        cardboardView.setRenderer(renderer);
        cardboardView.setOnBackButtonClick(this::finish);
        cardboardView.setOnSettingsButtonClick(
            () -> inputRouter.submitAction(
                VrInputAction.BACK,
                "Cardboard system control"
            )
        );
        cardboardView.setOnTriggerEvent(
            () -> inputRouter.submitAction(
                VrInputAction.SELECT,
                "Cardboard trigger"
            )
        );

        setContentView(cardboardView);
        controllerManager.addListener(this);
        refreshInputSourceStatus();
    }

    @Override
    protected void onResume() {
        super.onResume();
        registerInputDeviceListener();
        registerPerformanceEnvironmentMonitoring();
        enterImmersiveMode();

        if (cardboardView != null) {
            cardboardView.onResume();
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

        if (inputBindingEngine != null) {
            inputBindingEngine.releaseAll();
        }
        if (standardHidInputRouter != null) {
            standardHidInputRouter.reset();
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
        if (cardboardView != null) {
            cardboardView.onPause();
        }
        unregisterPerformanceEnvironmentMonitoring();
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        unregisterInputDeviceListener();

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
        if (cardboardView != null) {
            cardboardView.onDestroy();
        }
        if (nativeModuleRuntime != null) {
            nativeModuleRuntime.close();
        }
        if (uiFeedback != null) {
            uiFeedback.close();
        }
        super.onDestroy();
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (inputRouter != null && inputRouter.onKeyEvent(event)) {
            return true;
        }
        if (standardHidInputRouter != null
            && standardHidInputRouter.onKeyboardEvent(event)) {
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {
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
        if (renderer != null && controllerManager.isReady()) {
            renderer.setControllerBattery(percentage);
        }
    }

    @Override
    public void onControllerStateChanged(
        ControllerSnapshot snapshot
    ) {
        if (inputRouter == null || snapshot == null) {
            return;
        }

        inputRouter.onControllerSnapshot(
            snapshot,
            controllerManager.getActiveProviderDisplayName()
        );
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

            case RECENTER:
                renderer.requestRecenter();
                break;

            case NAV_LEFT:
                renderer.requestVideoSeek(-10000);
                break;

            case NAV_RIGHT:
                renderer.requestVideoSeek(10000);
                break;

            case VOLUME_UP:
                adjustMediaVolume(AudioManager.ADJUST_RAISE);
                break;

            case VOLUME_DOWN:
                adjustMediaVolume(AudioManager.ADJUST_LOWER);
                break;

            case NAV_UP:
            case NAV_DOWN:
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
        if (isGuestInputSuppressed()) {
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
        if (isGuestInputSuppressed()) {
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
        if (renderer == null || controllerManager == null) {
            return;
        }

        if (controllerManager.isReady()) {
            String provider =
                controllerManager.getActiveProviderDisplayName();
            renderer.setControllerState(
                true,
                provider == null || provider.trim().isEmpty()
                    ? controllerConnectionMessage
                    : provider + " ready"
            );
            renderer.setControllerBattery(
                controllerManager.getBatteryPercentage()
            );
            return;
        }

        String gamepad =
            AndroidGamepadSupport.firstConnectedGamepadName();
        if (gamepad != null) {
            renderer.setControllerState(
                true,
                "Android gamepad ready: " + gamepad
            );
            renderer.setControllerBattery(-1);
        } else {
            renderer.setControllerState(
                false,
                controllerConnectionMessage
            );
            renderer.setControllerBattery(-1);
        }
    }

    @Override
    public void onExitToPhoneRequested() {
        runOnUiThread(this::finish);
    }

    @Override
    public void onUiActionRejected() {
        runOnUiThread(() -> uiFeedback.failure());
    }

    @Override
    public void onSetupCompleted() {
        // The renderer persists the setup profile before this callback.
    }

    @Override
    public void onControllerRecenterRequested() {
        if (controllerManager != null) {
            controllerManager.recenterController();
        }
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
    public boolean onNativeModulePlaybackRequested(
        String moduleId
    ) {
        if (nativeModuleRuntime == null
            || inputBindingManager == null) {
            return false;
        }

        inputBindingManager.beginHostedProfile(
            BuiltInBindingProfiles.ID_NATIVE_TEST_CHAMBER
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

        ReverieLog.milestone(
            "NATIVE_MODULE",
            "Stage B launched module="
                + moduleId
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
        refreshDosModuleStatus();
    }

    @Override
    public void onDosSessionEnded(
        String message,
        boolean error
    ) {
        runOnUiThread(() -> {
            activeDosModuleId = "";
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

    private void registerPerformanceEnvironmentMonitoring() {
        if (!batteryTemperatureReceiverRegistered) {
            try {
                Intent sticky = registerReceiver(
                    batteryTemperatureReceiver,
                    new IntentFilter(Intent.ACTION_BATTERY_CHANGED)
                );
                batteryTemperatureReceiverRegistered = true;
                updateBatteryTemperature(sticky);
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

    private void updateBatteryTemperature(Intent intent) {
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
