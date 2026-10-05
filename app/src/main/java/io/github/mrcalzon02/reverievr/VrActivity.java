package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.os.BatteryManager;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import com.google.cardboard.proto.CardboardDevice;
import com.google.cardboard.sdk.CardboardView;
import com.google.cardboard.sdk.CardboardViewApi;

public final class VrActivity extends Activity
    implements ControllerManager.Listener, VrShellRenderer.Host {

    private static final float SAFE_VIEWER_FALLBACK_IPD_METERS = 0.060f;

    private CardboardView cardboardView;
    private VrShellRenderer renderer;
    private ControllerManager controllerManager;

    private boolean previousTouchpadPressed;
    private boolean previousMenuPressed;
    private boolean previousHomePressed;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        enterImmersiveMode();

        controllerManager =
            ((ReverieApplication) getApplication()).getControllerManager();

        ReveriePreferences preferences = new ReveriePreferences(this);
        float viewerIpd = readViewerInterLensDistance();

        CardboardView.setUseCardboardGlSurfaceView(true);
        cardboardView = new CardboardView(this);
        cardboardView.setStereoRenderMode(true);

        renderer = new VrShellRenderer(preferences, viewerIpd, this);
        renderer.setPhoneBattery(readPhoneBattery());
        renderer.setControllerBattery(controllerManager.getBatteryPercentage());
        renderer.setControllerState(
            controllerManager.isReady(),
            controllerManager.isReady()
                ? "Controller ready"
                : "Controller disconnected"
        );

        cardboardView.setRenderer(renderer);
        cardboardView.setOnBackButtonClick(this::finish);
        cardboardView.setOnSettingsButtonClick(renderer::requestBack);
        cardboardView.setOnTriggerEvent(renderer::requestSelect);

        setContentView(cardboardView);
        controllerManager.addListener(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        enterImmersiveMode();
        if (cardboardView != null) {
            cardboardView.onResume();
        }
        if (renderer != null) {
            renderer.setPhoneBattery(readPhoneBattery());
        }
    }

    @Override
    protected void onPause() {
        if (cardboardView != null) {
            cardboardView.onPause();
        }
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        if (controllerManager != null) {
            controllerManager.removeListener(this);
        }
        if (cardboardView != null) {
            cardboardView.onDestroy();
        }
        super.onDestroy();
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
        if (renderer == null) {
            return;
        }

        boolean ready = state == ControllerProvider.ConnectionState.READY;
        renderer.setControllerState(ready, message);
    }

    @Override
    public void onBatteryChanged(int percentage, int millivolts) {
        if (renderer != null) {
            renderer.setControllerBattery(percentage);
        }
    }

    @Override
    public void onControllerStateChanged(ControllerSnapshot snapshot) {
        if (renderer == null || snapshot == null) {
            return;
        }

        boolean selectEdge =
            snapshot.touchpadPressed && !previousTouchpadPressed;
        boolean menuEdge =
            snapshot.menuPressed && !previousMenuPressed;
        boolean homeEdge =
            snapshot.homePressed && !previousHomePressed;

        previousTouchpadPressed = snapshot.touchpadPressed;
        previousMenuPressed = snapshot.menuPressed;
        previousHomePressed = snapshot.homePressed;

        if (selectEdge) {
            renderer.requestSelect();
        }
        if (menuEdge) {
            renderer.requestBack();
        }
        if (homeEdge) {
            renderer.requestRecenter();
        }
    }

    @Override
    public void onExitToPhoneRequested() {
        runOnUiThread(this::finish);
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

    private float readViewerInterLensDistance() {
        CardboardViewApi api = new CardboardViewApi(this);
        try {
            if (!api.hasSavedDeviceParams()) {
                return SAFE_VIEWER_FALLBACK_IPD_METERS;
            }

            CardboardDevice.DeviceParams params = api.getSavedDeviceParams();
            if (params == null || !params.hasInterLensDistance()) {
                return SAFE_VIEWER_FALLBACK_IPD_METERS;
            }

            float value = params.getInterLensDistance();
            if (value < 0.050f || value > 0.080f) {
                return SAFE_VIEWER_FALLBACK_IPD_METERS;
            }
            return value;
        } catch (RuntimeException exception) {
            return SAFE_VIEWER_FALLBACK_IPD_METERS;
        } finally {
            api.close();
        }
    }

    private int readPhoneBattery() {
        BatteryManager batteryManager =
            (BatteryManager) getSystemService(Context.BATTERY_SERVICE);
        if (batteryManager == null) {
            return -1;
        }

        int percentage =
            batteryManager.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY);
        return percentage >= 0 && percentage <= 100 ? percentage : -1;
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
