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
    private final SteamControllerBatteryMonitor steamBatteryMonitor;
    private final CopyOnWriteArrayList<Listener> listeners =
        new CopyOnWriteArrayList<>();

    private volatile ControllerProvider activeProvider;
    private volatile ControllerProvider.ConnectionState connectionState =
        ControllerProvider.ConnectionState.IDLE;
    private volatile String connectionMessage = "Controller is not connected.";
    private volatile int batteryPercentage = -1;
    private volatile int batteryMillivolts = -1;
    private volatile ControllerSnapshot lastSnapshot;
    private final ControllerResumeGate resumeGate = new ControllerResumeGate();

    ControllerManager(Context context) {
        daydreamProvider = new DaydreamControllerProvider(context);
        phoneEmulatorProvider = new PhoneControllerEmulatorProvider(context);
        steamBatteryMonitor = new SteamControllerBatteryMonitor(context, this::notifyBattery);

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

        ControllerSnapshot snapshot = getLastSnapshot();
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

    /** Switch to Android HID without unpairing either Bluetooth controller. */
    void deselectProvider() {
        ControllerProvider old = activeProvider;
        activeProvider = null;
        if (old != null) old.disconnect();
        connectionState = ControllerProvider.ConnectionState.IDLE;
        connectionMessage = "Android HID controller selected.";
        batteryPercentage = -1;
        batteryMillivolts = -1;
        lastSnapshot = null;
        resumeGate.requireNeutral();
        notifyConnection();
        notifyBattery();
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

    void refreshSteamBattery(boolean selectedAndConnected) {
        steamBatteryMonitor.refresh(selectedAndConnected);
    }

    int getSteamBatteryPercentage() {
        int publicApi = AndroidGamepadSupport.steamBatteryPercentage();
        return publicApi >= 0 ? publicApi : steamBatteryMonitor.getPercentage();
    }

    int getBatteryPercentage() {
        return batteryPercentage;
    }

    int getBatteryMillivolts() {
        return batteryMillivolts;
    }

    ControllerSnapshot getLastSnapshot() {
        return connectionState == ControllerProvider.ConnectionState.READY
            ? lastSnapshot : null;
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
        resumeGate.requireNeutral();

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
        steamBatteryMonitor.close();
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

            ControllerProvider.ConnectionState previous = connectionState;
            connectionState = state;
            connectionMessage = message == null ? "" : message;
            if (state != ControllerProvider.ConnectionState.READY
                || previous != ControllerProvider.ConnectionState.READY) {
                resumeGate.requireNeutral();
            }
            if (state != ControllerProvider.ConnectionState.READY) {
                lastSnapshot = null;
                batteryPercentage = -1;
                batteryMillivolts = -1;
            }
            notifyConnection();
            if (state != ControllerProvider.ConnectionState.READY) {
                notifyBattery();
            }
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
            // A disconnect can race the last BLE/companion input callback.
            // Discard late down edges instead of reasserting guest buttons
            // after the host already released every held control.
            if (provider != activeProvider
                || connectionState != ControllerProvider.ConnectionState.READY
                || !resumeGate.accept(snapshot)) {
                return;
            }

            lastSnapshot = snapshot;
            for (Listener listener : listeners) {
                listener.onControllerStateChanged(snapshot);
            }
        }
    }
}
