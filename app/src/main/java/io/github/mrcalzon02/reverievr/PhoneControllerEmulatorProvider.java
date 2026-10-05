package io.github.mrcalzon02.reverievr;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;

import java.io.DataInputStream;
import java.io.EOFException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

final class PhoneControllerEmulatorProvider implements ControllerProvider {
    private static final UUID EMULATOR_RFCOMM_UUID =
        UUID.fromString("ab001ac1-d740-4abb-a8e6-1cb5a49628fa");
    private static final int MAX_MESSAGE_BYTES = 4096;

    private final Context context;
    private final BluetoothAdapter adapter;
    private final ExecutorService executor = Executors.newSingleThreadExecutor();

    private volatile Listener listener;
    private volatile BluetoothSocket socket;
    private volatile boolean ready;
    private volatile int generation;
    private volatile String selectedAddress;

    private int packetIndex;
    private float orientationX;
    private float orientationY;
    private float orientationZ;
    private float orientationW = 1.0f;
    private float accelX;
    private float accelY;
    private float accelZ;
    private float gyroX;
    private float gyroY;
    private float gyroZ;
    private boolean touching;
    private int touchX;
    private int touchY;
    private boolean volumeUp;
    private boolean volumeDown;
    private boolean menu;
    private boolean home;
    private boolean click;

    PhoneControllerEmulatorProvider(Context context) {
        this.context = context.getApplicationContext();
        BluetoothManager manager =
            (BluetoothManager) this.context.getSystemService(Context.BLUETOOTH_SERVICE);
        adapter = manager == null ? null : manager.getAdapter();
    }

    @Override
    public String getProviderId() {
        return "daydream_phone_emulator";
    }

    @Override
    public String getDisplayName() {
        return "Phone controller emulator";
    }

    @Override
    public String[] getMissingRuntimePermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S
            && context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)
                != PackageManager.PERMISSION_GRANTED) {
            return new String[] {Manifest.permission.BLUETOOTH_CONNECT};
        }
        return new String[0];
    }

    @Override
    public void setListener(Listener listener) {
        this.listener = listener;
    }

    List<PhoneControllerTarget> getBondedTargets() {
        List<PhoneControllerTarget> result = new ArrayList<>();

        if (adapter == null || getMissingRuntimePermissions().length > 0) {
            return result;
        }

        try {
            Set<BluetoothDevice> devices = adapter.getBondedDevices();
            if (devices == null) {
                return result;
            }

            for (BluetoothDevice device : devices) {
                if (device == null) {
                    continue;
                }

                String address = device.getAddress();
                String name = device.getName();
                if (address != null && !address.trim().isEmpty()) {
                    result.add(new PhoneControllerTarget(address, name));
                }
            }
        } catch (SecurityException ignored) {
            return new ArrayList<>();
        }

        result.sort(Comparator.comparing(target -> target.displayName.toLowerCase()));
        return result;
    }

    void setSelectedTarget(PhoneControllerTarget target) {
        selectedAddress = target == null ? null : target.address;
    }

    @Override
    public void startPairing() {
        String[] missing = getMissingRuntimePermissions();
        if (missing.length > 0) {
            emitConnection(
                ConnectionState.PERMISSION_REQUIRED,
                "Bluetooth connect permission is required for the phone controller."
            );
            return;
        }

        if (adapter == null) {
            emitConnection(
                ConnectionState.ERROR,
                "This phone does not expose a Bluetooth adapter."
            );
            return;
        }

        try {
            if (!adapter.isEnabled()) {
                emitConnection(
                    ConnectionState.BLUETOOTH_DISABLED,
                    "Bluetooth is turned off."
                );
                return;
            }
        } catch (SecurityException exception) {
            emitConnection(
                ConnectionState.PERMISSION_REQUIRED,
                "Bluetooth permission was removed."
            );
            return;
        }

        String address = selectedAddress;
        if (address == null || address.trim().isEmpty()) {
            emitConnection(
                ConnectionState.ERROR,
                "Choose an already-paired controller phone first."
            );
            return;
        }

        disconnect();
        selectedAddress = address;
        final int currentGeneration = ++generation;

        emitConnection(
            ConnectionState.CONNECTING,
            "Connecting to paired controller phone…"
        );

        executor.execute(() -> connectAndRead(address, currentGeneration));
    }

    @Override
    public void disconnect() {
        generation++;
        ready = false;

        BluetoothSocket active = socket;
        socket = null;
        if (active != null) {
            try {
                active.close();
            } catch (IOException ignored) {
                // The RFCOMM socket is already closing.
            }
        }

        resetState();
    }

    @Override
    public boolean recenter() {
        // The historical phone-emulator transport has no documented remote
        // recenter command. ReverieVR still performs its software yaw recenter.
        return false;
    }

    @Override
    public boolean isReady() {
        return ready;
    }

    private void connectAndRead(String address, int currentGeneration) {
        BluetoothSocket localSocket = null;
        boolean connectedOnce = false;
        boolean terminalStateReported = false;

        try {
            BluetoothDevice device = adapter.getRemoteDevice(address);
            if (device == null) {
                throw new IOException("Paired controller phone is unavailable.");
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S
                && context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED) {
                emitConnection(
                    ConnectionState.PERMISSION_REQUIRED,
                    "Bluetooth permission was removed."
                );
                return;
            }

            try {
                adapter.cancelDiscovery();
                localSocket =
                    device.createRfcommSocketToServiceRecord(EMULATOR_RFCOMM_UUID);
                socket = localSocket;
                localSocket.connect();
            } catch (SecurityException exception) {
                emitConnection(
                    ConnectionState.PERMISSION_REQUIRED,
                    "Bluetooth permission was removed."
                );
                return;
            }

            if (currentGeneration != generation) {
                return;
            }

            connectedOnce = true;
            emitConnection(
                ConnectionState.DISCOVERING,
                "Connected to controller phone. Waiting for emulator input…"
            );

            DataInputStream input =
                new DataInputStream(localSocket.getInputStream());

            while (currentGeneration == generation) {
                int length = input.readInt();
                if (length <= 0 || length > MAX_MESSAGE_BYTES) {
                    throw new IOException(
                        "Controller phone sent invalid message length: " + length
                    );
                }

                byte[] payload = new byte[length];
                input.readFully(payload);

                PhoneControllerProtocol.Event event =
                    PhoneControllerProtocol.parse(payload);
                if (!applyEvent(event)) {
                    continue;
                }

                if (!ready) {
                    ready = true;
                    emitConnection(
                        ConnectionState.READY,
                        "Phone controller emulator connected and streaming."
                    );
                    emitBatteryUnknown();
                }
            }
        } catch (EOFException exception) {
            if (currentGeneration == generation) {
                terminalStateReported = true;
                emitConnection(
                    ConnectionState.DISCONNECTED,
                    "Controller phone disconnected."
                );
            }
        } catch (IOException | IllegalArgumentException exception) {
            if (currentGeneration == generation) {
                terminalStateReported = true;
                emitConnection(
                    ConnectionState.ERROR,
                    "Phone controller connection failed: "
                        + safeMessage(exception)
                );
            }
        } finally {
            if (localSocket != null) {
                try {
                    localSocket.close();
                } catch (IOException ignored) {
                    // Socket already closed.
                }
            }

            if (currentGeneration == generation) {
                socket = null;
                ready = false;
                resetState();

                if (connectedOnce && !terminalStateReported) {
                    emitConnection(
                        ConnectionState.DISCONNECTED,
                        "Phone controller emulator disconnected."
                    );
                }
            }
        }
    }

    private synchronized boolean applyEvent(
        PhoneControllerProtocol.Event event
    ) {
        if (event == null) {
            return false;
        }

        switch (event.type) {
            case PhoneControllerProtocol.TYPE_MOTION:
                if (event.hasPointer) {
                    touchX = Math.round(event.x * 255.0f);
                    touchY = Math.round(event.y * 255.0f);
                }
                touching =
                    event.action == PhoneControllerProtocol.ACTION_DOWN
                        || event.action == PhoneControllerProtocol.ACTION_MOVE;
                break;

            case PhoneControllerProtocol.TYPE_GYROSCOPE:
                gyroX = event.x;
                gyroY = event.y;
                gyroZ = event.z;
                break;

            case PhoneControllerProtocol.TYPE_ACCELEROMETER:
                accelX = event.x;
                accelY = event.y;
                accelZ = -event.z;
                break;

            case PhoneControllerProtocol.TYPE_ORIENTATION:
                // Match the historical Daydream controller-client coordinate
                // transform without depending on Google VR Services.
                orientationX = -event.x;
                orientationY = -event.z;
                orientationZ = event.y;
                orientationW = event.w;
                break;

            case PhoneControllerProtocol.TYPE_KEY:
                boolean down = event.action == PhoneControllerProtocol.ACTION_DOWN;
                switch (event.keyCode) {
                    case PhoneControllerProtocol.KEY_CLICK:
                        click = down;
                        break;
                    case PhoneControllerProtocol.KEY_APP:
                        menu = down;
                        break;
                    case PhoneControllerProtocol.KEY_HOME:
                        home = down;
                        break;
                    case PhoneControllerProtocol.KEY_VOLUME_UP:
                        volumeUp = down;
                        break;
                    case PhoneControllerProtocol.KEY_VOLUME_DOWN:
                        volumeDown = down;
                        break;
                    default:
                        break;
                }
                break;

            default:
                return false;
        }

        packetIndex = (packetIndex + 1) & 0x1f;

        ControllerSnapshot snapshot = new ControllerSnapshot(
            System.nanoTime(),
            (int) ((System.nanoTime() / 1_000_000L) & 0x1ff),
            packetIndex,
            orientationX,
            orientationY,
            orientationZ,
            orientationW,
            accelX,
            accelY,
            accelZ,
            gyroX,
            gyroY,
            gyroZ,
            touching,
            touchX,
            touchY,
            volumeUp,
            volumeDown,
            menu,
            home,
            click,
            0
        );

        Listener target = listener;
        if (target != null) {
            target.onControllerStateChanged(snapshot);
        }
        return true;
    }

    private void resetState() {
        synchronized (this) {
            orientationX = 0.0f;
            orientationY = 0.0f;
            orientationZ = 0.0f;
            orientationW = 1.0f;
            accelX = 0.0f;
            accelY = 0.0f;
            accelZ = 0.0f;
            gyroX = 0.0f;
            gyroY = 0.0f;
            gyroZ = 0.0f;
            touching = false;
            touchX = 0;
            touchY = 0;
            volumeUp = false;
            volumeDown = false;
            menu = false;
            home = false;
            click = false;
        }
    }

    private void emitBatteryUnknown() {
        Listener target = listener;
        if (target != null) {
            target.onBatteryChanged(-1, -1);
        }
    }

    private void emitConnection(ConnectionState state, String message) {
        Listener target = listener;
        if (target != null) {
            target.onConnectionStateChanged(state, message);
        }
    }

    private static String safeMessage(Exception exception) {
        String message = exception.getMessage();
        return message == null || message.trim().isEmpty()
            ? exception.getClass().getSimpleName()
            : message;
    }

    @Override
    public void close() {
        disconnect();
        executor.shutdownNow();
    }
}
