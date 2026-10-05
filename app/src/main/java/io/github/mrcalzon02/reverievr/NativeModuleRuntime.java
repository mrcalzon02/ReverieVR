package io.github.mrcalzon02.reverievr;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

final class NativeModuleRuntime implements AutoCloseable {
    static final class Descriptor {
        final String id;
        final String displayName;

        Descriptor(
            String id,
            String displayName
        ) {
            this.id = id == null ? "" : id;
            this.displayName =
                displayName == null
                    ? ""
                    : displayName;
        }
    }

    private static final boolean AVAILABLE;

    static {
        boolean loaded = false;
        try {
            System.loadLibrary(
                "reverie_native_host"
            );
            loaded = true;
        } catch (UnsatisfiedLinkError error) {
            ReverieLog.error(
                "NATIVE_MODULE",
                "Native module host library is unavailable.",
                error
            );
        }
        AVAILABLE = loaded;
    }

    private final VirtualInputBus inputBus;

    private long handle;
    private long lastUpdateNanos;
    private String activeModuleId = "";

    NativeModuleRuntime(
        VirtualInputBus inputBus
    ) {
        this.inputBus = inputBus;
    }

    static boolean isAvailable() {
        return AVAILABLE;
    }

    static List<Descriptor> listBuiltIns() {
        if (!AVAILABLE) {
            return Collections.emptyList();
        }

        String[] flattened;
        try {
            flattened = nativeListBuiltIns();
        } catch (RuntimeException exception) {
            ReverieLog.error(
                "NATIVE_MODULE",
                "Could not enumerate native modules.",
                exception
            );
            return Collections.emptyList();
        }

        if (flattened == null
            || flattened.length == 0) {
            return Collections.emptyList();
        }

        List<Descriptor> result =
            new ArrayList<>();
        for (int index = 0;
             index + 1 < flattened.length;
             index += 2) {
            String id = flattened[index];
            String name = flattened[index + 1];

            if (id == null
                || id.trim().isEmpty()
                || name == null
                || name.trim().isEmpty()) {
                continue;
            }

            result.add(
                new Descriptor(
                    id.trim(),
                    name.trim()
                )
            );
        }

        return Collections.unmodifiableList(
            result
        );
    }

    synchronized boolean start(
        String moduleId
    ) {
        if (!AVAILABLE) {
            return false;
        }

        stop();

        long created =
            nativeCreate(moduleId);
        if (created == 0L) {
            ReverieLog.incident(
                "NATIVE_MODULE",
                "Native module launch failed: "
                    + getLastError()
            );
            return false;
        }

        handle = created;
        activeModuleId =
            moduleId == null
                ? ""
                : moduleId.trim();
        lastUpdateNanos =
            System.nanoTime();

        nativeResume(handle);

        ReverieLog.milestone(
            "NATIVE_MODULE",
            "Started module="
                + activeModuleId
        );
        return true;
    }

    synchronized boolean isRunning() {
        return handle != 0L;
    }

    synchronized String getActiveModuleId() {
        return activeModuleId;
    }

    synchronized boolean onSurfaceCreated() {
        if (handle == 0L) {
            return false;
        }

        boolean ready =
            nativeOnSurfaceCreated(handle);
        if (!ready) {
            ReverieLog.incident(
                "NATIVE_MODULE",
                "Native module GL initialization failed: "
                    + getLastError()
            );
        }
        return ready;
    }

    synchronized void releaseSurface() {
        if (handle != 0L) {
            nativeReleaseSurface(handle);
        }
    }

    synchronized void resumeForLifecycle() {
        if (handle == 0L) {
            return;
        }

        lastUpdateNanos =
            System.nanoTime();
        nativeResume(handle);
    }

    synchronized void pauseForLifecycle() {
        if (handle != 0L) {
            nativePause(handle);
        }
    }

    synchronized void update() {
        if (handle == 0L) {
            return;
        }

        long now = System.nanoTime();
        float deltaSeconds =
            (now - lastUpdateNanos)
                / 1_000_000_000.0f;
        lastUpdateNanos = now;

        float moveX = 0.0f;
        float moveY = 0.0f;
        boolean primary = false;
        boolean secondary = false;

        if (inputBus != null) {
            moveX =
                inputBus.getJoystickAxisX();
            moveY =
                inputBus.getJoystickAxisY();
            primary =
                inputBus.isJoystickButtonDown(0);
            secondary =
                inputBus.isJoystickButtonDown(1);
        }

        nativeUpdate(
            handle,
            deltaSeconds,
            moveX,
            moveY,
            primary,
            secondary
        );
    }

    synchronized boolean renderEye(
        int eyeIndex,
        float[] view,
        float[] projection
    ) {
        if (handle == 0L) {
            return false;
        }

        boolean rendered =
            nativeRenderEye(
                handle,
                eyeIndex,
                view,
                projection
            );

        if (!rendered) {
            ReverieLog.incident(
                "NATIVE_MODULE",
                "Native module render failed: "
                    + getLastError()
            );
        }

        return rendered;
    }

    synchronized String getLastError() {
        if (!AVAILABLE) {
            return "Native module host library is unavailable.";
        }

        String value =
            nativeGetLastError();
        return value == null ? "" : value.trim();
    }

    synchronized void stop() {
        long active = handle;
        if (active == 0L) {
            return;
        }

        handle = 0L;
        activeModuleId = "";
        lastUpdateNanos = 0L;

        nativePause(active);
        nativeDestroy(active);

        ReverieLog.milestone(
            "NATIVE_MODULE",
            "Native module stopped."
        );
    }

    @Override
    public synchronized void close() {
        stop();
    }

    private static native String[] nativeListBuiltIns();

    private static native long nativeCreate(
        String moduleId
    );

    private static native void nativeDestroy(
        long handle
    );

    private static native boolean nativeOnSurfaceCreated(
        long handle
    );

    private static native void nativeReleaseSurface(
        long handle
    );

    private static native void nativeResume(
        long handle
    );

    private static native void nativePause(
        long handle
    );

    private static native void nativeUpdate(
        long handle,
        float deltaSeconds,
        float moveX,
        float moveY,
        boolean primaryDown,
        boolean secondaryDown
    );

    private static native boolean nativeRenderEye(
        long handle,
        int eyeIndex,
        float[] view,
        float[] projection
    );

    private static native String nativeGetLastError();
}
