package io.github.mrcalzon02.reverievr;

import android.content.Context;

import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

final class ControllerManager implements AutoCloseable {
    interface Listener {
        void onConnectionStateChanged(
            ControllerProvider.ConnectionState state,
            String message
        );

        void onBatteryChanged(int percentage, int millivolts);

        void onControllerStateChanged(ControllerSnapshot snapshot);
    }

    private final ControllerProvider daydreamProvider;
    private final PhoneControllerEmulatorProvider phoneEmulatorProvider;
    private final CopyOnWriteArrayList<Listener> listeners =
        new CopyOnWriteArrayList<>();

    private volatile ControllerProvider activeProvider;
    private volatile ControllerProvider.ConnectionState connectionState =
        ControllerProvider.ConnectionState.IDLE;
    private volatile String connectionMessage = "Controller is not connected.";
    private volatile int batteryPercentage = -1;
    private volatile int batteryMillivolts = -1;
    private volatile ControllerSnapshot lastSnapshot;

    ControllerManager(Context context) {
        daydreamProvider = new DaydreamControllerProvider(context);
        phoneEmulatorProvider = new PhoneControllerEmulatorProvider(context);

        daydreamProvider.setListener(
            new ProviderListener(daydreamProvider)
        );
        phoneEmulatorProvider.setListener(
            new ProviderListener(phoneEmulatorProvider)
        );
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

    String[] getMissingPhoneEmulatorPermissions() {
        return phoneEmulatorProvider.getMissingRuntimePermissions();
    }

    List<PhoneControllerTarget> getPairedPhoneTargets() {
        return phoneEmulatorProvider.getBondedTargets();
    }

    void pairDaydreamController() {
        activateProvider(daydreamProvider);
        daydreamProvider.startPairing();
    }

    void connectPhoneEmulator(PhoneControllerTarget target) {
        activateProvider(phoneEmulatorProvider);
        phoneEmulatorProvider.setSelectedTarget(target);
        phoneEmulatorProvider.startPairing();
    }

    boolean recenterController() {
        ControllerProvider provider = activeProvider;
        return provider != null && provider.recenter();
    }

    boolean isReady() {
        ControllerProvider provider = activeProvider;
        return provider != null && provider.isReady();
    }

    String getActiveProviderId() {
        ControllerProvider provider = activeProvider;
        return provider == null ? "" : provider.getProviderId();
    }

    String getActiveProviderDisplayName() {
        ControllerProvider provider = activeProvider;
        return provider == null ? "" : provider.getDisplayName();
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

    private void activateProvider(ControllerProvider provider) {
        ControllerProvider previous = activeProvider;
        activeProvider = provider;

        if (previous != null && previous != provider) {
            previous.disconnect();
        }

        connectionState = ControllerProvider.ConnectionState.IDLE;
        connectionMessage =
            provider == null
                ? "Controller is not connected."
                : provider.getDisplayName() + " selected.";
        batteryPercentage = -1;
        batteryMillivolts = -1;
        lastSnapshot = null;

        notifyConnection();
        notifyBattery();
    }

    private void notifyConnection() {
        for (Listener listener : listeners) {
            listener.onConnectionStateChanged(
                connectionState,
                connectionMessage
            );
        }
    }

    private void notifyBattery() {
        for (Listener listener : listeners) {
            listener.onBatteryChanged(
                batteryPercentage,
                batteryMillivolts
            );
        }
    }

    @Override
    public void close() {
        listeners.clear();
        daydreamProvider.close();
        phoneEmulatorProvider.close();
    }

    private final class ProviderListener implements ControllerProvider.Listener {
        private final ControllerProvider provider;

        ProviderListener(ControllerProvider provider) {
            this.provider = provider;
        }

        @Override
        public void onConnectionStateChanged(
            ControllerProvider.ConnectionState state,
            String message
        ) {
            if (provider != activeProvider) {
                return;
            }

            connectionState = state;
            connectionMessage = message == null ? "" : message;
            notifyConnection();
        }

        @Override
        public void onBatteryChanged(int percentage, int millivolts) {
            if (provider != activeProvider) {
                return;
            }

            batteryPercentage = percentage;
            batteryMillivolts = millivolts;
            notifyBattery();
        }

        @Override
        public void onControllerStateChanged(ControllerSnapshot snapshot) {
            if (provider != activeProvider) {
                return;
            }

            lastSnapshot = snapshot;
            for (Listener listener : listeners) {
                listener.onControllerStateChanged(snapshot);
            }
        }
    }
}
