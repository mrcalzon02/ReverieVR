package io.github.mrcalzon02.reverievr;

import android.content.Context;

import java.util.concurrent.CopyOnWriteArrayList;

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
    private final CopyOnWriteArrayList<Listener> listeners =
        new CopyOnWriteArrayList<>();

    private volatile ControllerProvider.ConnectionState connectionState =
        ControllerProvider.ConnectionState.IDLE;
    private volatile String connectionMessage = "Controller is not connected.";
    private volatile int batteryPercentage = -1;
    private volatile int batteryMillivolts = -1;
    private volatile ControllerSnapshot lastSnapshot;

    ControllerManager(Context context) {
        daydreamProvider = new DaydreamControllerProvider(context);
        daydreamProvider.setListener(this);
    }

    void addListener(Listener listener) {
        if (listener == null) {
            return;
        }

        listeners.addIfAbsent(listener);
        listener.onConnectionStateChanged(connectionState, connectionMessage);
        listener.onBatteryChanged(batteryPercentage, batteryMillivolts);

        ControllerSnapshot snapshot = lastSnapshot;
        if (snapshot != null) {
            listener.onControllerStateChanged(snapshot);
        }
    }

    void removeListener(Listener listener) {
        listeners.remove(listener);
    }

    String[] getMissingRuntimePermissions() {
        return daydreamProvider.getMissingRuntimePermissions();
    }

    void pairDaydreamController() {
        daydreamProvider.startPairing();
    }

    void recenterController() {
        daydreamProvider.recenter();
    }

    boolean isReady() {
        return daydreamProvider.isReady();
    }

    int getBatteryPercentage() {
        return batteryPercentage;
    }

    int getBatteryMillivolts() {
        return batteryMillivolts;
    }

    ControllerSnapshot getLastSnapshot() {
        return lastSnapshot;
    }

    @Override
    public void onConnectionStateChanged(
        ControllerProvider.ConnectionState state,
        String message
    ) {
        connectionState = state;
        connectionMessage = message == null ? "" : message;
        for (Listener listener : listeners) {
            listener.onConnectionStateChanged(state, connectionMessage);
        }
    }

    @Override
    public void onBatteryChanged(int percentage, int millivolts) {
        batteryPercentage = percentage;
        batteryMillivolts = millivolts;
        for (Listener listener : listeners) {
            listener.onBatteryChanged(percentage, millivolts);
        }
    }

    @Override
    public void onControllerStateChanged(ControllerSnapshot snapshot) {
        lastSnapshot = snapshot;
        for (Listener listener : listeners) {
            listener.onControllerStateChanged(snapshot);
        }
    }

    @Override
    public void close() {
        listeners.clear();
        daydreamProvider.close();
    }
}
