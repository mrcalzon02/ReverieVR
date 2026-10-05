package io.github.mrcalzon02.reverievr.controller;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.UUID;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

final class ControllerServer implements AutoCloseable {
    interface Listener {
        void onServerStateChanged(State state, String message);
    }

    enum State {
        STOPPED,
        LISTENING,
        CONNECTED,
        ERROR
    }

    static final UUID SERVICE_UUID =
        UUID.fromString("ab001ac1-d740-4abb-a8e6-1cb5a49628fa");

    private static final String SERVICE_NAME = "ReverieVR Controller";
    private static final int MAX_QUEUED_MESSAGES = 64;

    private final Context context;
    private final BluetoothAdapter adapter;
    private final Listener listener;
    private final ExecutorService acceptExecutor =
        Executors.newSingleThreadExecutor();
    private final ExecutorService writeExecutor =
        Executors.newSingleThreadExecutor();
    private final ArrayBlockingQueue<byte[]> sendQueue =
        new ArrayBlockingQueue<>(MAX_QUEUED_MESSAGES);

    private final Object connectionLock = new Object();

    private volatile boolean running;
    private volatile boolean connected;
    private volatile BluetoothServerSocket serverSocket;
    private volatile BluetoothSocket clientSocket;
    private volatile DataOutputStream output;

    ControllerServer(Context context, Listener listener) {
        this.context = context.getApplicationContext();
        this.listener = listener;

        BluetoothManager manager =
            (BluetoothManager) this.context.getSystemService(
                Context.BLUETOOTH_SERVICE
            );
        adapter = manager == null ? null : manager.getAdapter();

        writeExecutor.execute(this::writeLoop);
    }

    boolean hasConnectPermission() {
        return Build.VERSION.SDK_INT < Build.VERSION_CODES.S
            || context.checkSelfPermission(
                Manifest.permission.BLUETOOTH_CONNECT
            ) == PackageManager.PERMISSION_GRANTED;
    }

    boolean isConnected() {
        return connected;
    }

    void start() {
        if (running) {
            return;
        }

        if (adapter == null) {
            emit(
                State.ERROR,
                "This phone does not expose Bluetooth."
            );
            return;
        }

        if (!hasConnectPermission()) {
            emit(
                State.ERROR,
                "Bluetooth permission is required."
            );
            return;
        }

        try {
            if (!adapter.isEnabled()) {
                emit(
                    State.ERROR,
                    "Bluetooth is turned off."
                );
                return;
            }
        } catch (SecurityException exception) {
            emit(
                State.ERROR,
                "Bluetooth permission was removed."
            );
            return;
        }

        running = true;
        acceptExecutor.execute(this::acceptLoop);
    }

    void stop() {
        running = false;
        connected = false;
        sendQueue.clear();
        closeServerSocket();
        closeClient();
        emit(State.STOPPED, "Controller server stopped.");
    }

    void send(byte[] payload) {
        if (!connected || payload == null || payload.length == 0) {
            return;
        }

        if (!sendQueue.offer(payload)) {
            sendQueue.poll();
            sendQueue.offer(payload);
        }
    }

    private void acceptLoop() {
        while (running) {
            BluetoothServerSocket localServer = null;
            BluetoothSocket localClient = null;

            try {
                localServer = adapter.listenUsingRfcommWithServiceRecord(
                    SERVICE_NAME,
                    SERVICE_UUID
                );
                serverSocket = localServer;
                emit(
                    State.LISTENING,
                    "Waiting for the headset phone…"
                );

                localClient = localServer.accept();
                if (!running) {
                    break;
                }

                synchronized (connectionLock) {
                    clientSocket = localClient;
                    output = new DataOutputStream(
                        localClient.getOutputStream()
                    );
                    connected = true;
                    sendQueue.clear();
                }

                closeServerSocket();
                emit(
                    State.CONNECTED,
                    describeClient(localClient)
                );

                InputStream input = localClient.getInputStream();
                while (running && connected) {
                    int value = input.read();
                    if (value < 0) {
                        break;
                    }
                }
            } catch (SecurityException exception) {
                if (running) {
                    emit(
                        State.ERROR,
                        "Bluetooth permission was removed."
                    );
                }
            } catch (IOException exception) {
                if (running) {
                    emit(
                        State.ERROR,
                        "Controller Bluetooth session ended: "
                            + safeMessage(exception)
                    );
                }
            } finally {
                connected = false;
                sendQueue.clear();

                synchronized (connectionLock) {
                    output = null;
                }

                if (localClient != null) {
                    try {
                        localClient.close();
                    } catch (IOException ignored) {
                        // Socket already closed.
                    }
                }

                clientSocket = null;
                closeServerSocket();

                if (running) {
                    emit(
                        State.LISTENING,
                        "Headset disconnected. Waiting for reconnection…"
                    );
                }
            }
        }
    }

    private void writeLoop() {
        while (!Thread.currentThread().isInterrupted()) {
            try {
                byte[] payload = sendQueue.poll(
                    1,
                    TimeUnit.SECONDS
                );
                if (payload == null) {
                    continue;
                }

                DataOutputStream active;
                synchronized (connectionLock) {
                    active = output;
                }

                if (active == null || !connected) {
                    continue;
                }

                try {
                    active.writeInt(payload.length);
                    active.write(payload);
                    active.flush();
                } catch (IOException exception) {
                    handleWriteFailure(exception);
                }
            } catch (InterruptedException exception) {
                Thread.currentThread().interrupt();
                return;
            }
        }
    }

    private void handleWriteFailure(IOException exception) {
        connected = false;
        sendQueue.clear();
        closeClient();

        if (running) {
            emit(
                State.ERROR,
                "Headset connection lost: " + safeMessage(exception)
            );
        }
    }

    private String describeClient(BluetoothSocket socket) {
        if (socket == null) {
            return "Headset connected.";
        }

        try {
            if (socket.getRemoteDevice() != null) {
                String name = socket.getRemoteDevice().getName();
                if (name != null && !name.trim().isEmpty()) {
                    return "Connected to " + name + ".";
                }
            }
        } catch (SecurityException ignored) {
            // Connection itself remains valid without exposing the name.
        }

        return "Headset connected.";
    }

    private void closeServerSocket() {
        BluetoothServerSocket active = serverSocket;
        serverSocket = null;
        if (active != null) {
            try {
                active.close();
            } catch (IOException ignored) {
                // Server socket already closed.
            }
        }
    }

    private void closeClient() {
        BluetoothSocket active = clientSocket;
        clientSocket = null;

        synchronized (connectionLock) {
            output = null;
        }

        if (active != null) {
            try {
                active.close();
            } catch (IOException ignored) {
                // Client socket already closed.
            }
        }
    }

    private void emit(State state, String message) {
        if (listener != null) {
            listener.onServerStateChanged(
                state,
                message == null ? "" : message
            );
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
        stop();
        acceptExecutor.shutdownNow();
        writeExecutor.shutdownNow();
    }
}
