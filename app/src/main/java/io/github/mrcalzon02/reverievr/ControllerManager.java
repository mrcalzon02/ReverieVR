package io.github.mrcalzon02.reverievr;

import android.content.Context;

final class ControllerManager implements ControllerProvider.Listener, AutoCloseable {
    interface Listener {
        void onConnectionStateChanged(
            ControllerProvider.ConnectionState state,
            String message
        );

        void onBatteryChanged(int percentage, int millivolts);

        void onControllerStateChanged(ControllerSnapshot snapshot);
    }

    private final ControllerProvider daydreamProvider;
    private Listener listener;

    ControllerManager(Context context) {
        daydreamProvider = new DaydreamControllerProvider(context);
        daydreamProvider.setListener(this);
    }

    void setListener(Listener listener) {
        this.listener = listener;
    }

    String[] getMissingRuntimePermissions() {
        return daydreamProvider.getMissingRuntimePermissions();
    }

    void pairDaydreamController() {
        daydreamProvider.startPairing();
    }

    void recenter() {
        daydreamProvider.recenter();
    }

    boolean isReady() {
        return daydreamProvider.isReady();
    }

    @Override
    public void onConnectionStateChanged(
        ControllerProvider.ConnectionState state,
        String message
    ) {
        Listener target = listener;
        if (target != null) {
            target.onConnectionStateChanged(state, message);
        }
    }

    @Override
    public void onBatteryChanged(int percentage, int millivolts) {
        Listener target = listener;
        if (target != null) {
            target.onBatteryChanged(percentage, millivolts);
        }
    }

    @Override
    public void onControllerStateChanged(ControllerSnapshot snapshot) {
        Listener target = listener;
        if (target != null) {
            target.onControllerStateChanged(snapshot);
        }
    }

    @Override
    public void close() {
        daydreamProvider.close();
    }
}
