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

    /**
     * Requests a hardware/provider recenter when that transport supports it.
     *
     * @return true when a provider-specific recenter command was accepted;
     *         false when recenter is software-only for this provider.
     */
    boolean recenter();

    boolean isReady();

    @Override
    void close();
}
