package io.github.mrcalzon02.reverievr;

interface ControllerProvider extends AutoCloseable {
    enum ConnectionState {
        IDLE,
        PERMISSION_REQUIRED,
        BLUETOOTH_DISABLED,
        SCANNING,
        BONDING,
        CONNECTING,
        DISCOVERING,
        READY,
        DISCONNECTED,
        ERROR
    }

    interface Listener {
        void onConnectionStateChanged(ConnectionState state, String message);
        void onBatteryChanged(int percentage, int millivolts);
        void onControllerStateChanged(ControllerSnapshot snapshot);
    }

    String getProviderId();

    String getDisplayName();

    String[] getMissingRuntimePermissions();

    void setListener(Listener listener);

    void startPairing();

    void disconnect();

    void recenter();

    boolean isReady();

    @Override
    void close();
}
