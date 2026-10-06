package io.github.mrcalzon02.reverievr;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

final class NativeModuleRuntime implements AutoCloseable {
    static final String ID_RED_LEDGER =
        "between-deliveries-red-ledger";

    static final int POINTER_NONE = 0;
    static final int POINTER_TRACKED_CONTROLLER = 1;
    static final int POINTER_VIRTUAL_CONTROLLER = 2;

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

    private final File storageRoot;
    private final VirtualInputBus inputBus;
    private final boolean developmentModulesAllowed;

    private long handle;
    private long lastUpdateNanos;
    private String activeModuleId = "";

    NativeModuleRuntime(
        File storageRoot,
        VirtualInputBus inputBus,
        boolean developmentModulesAllowed
    ) {
        this.storageRoot = storageRoot;
        this.inputBus = inputBus;
        this.developmentModulesAllowed =
            developmentModulesAllowed;
    }

    static boolean isAvailable() {
        return AVAILABLE;
    }

    static List<Descriptor> listBuiltIns() {
        return listBuiltIns(false);
    }

    static List<Descriptor> listBuiltIns(
        boolean includeDevelopmentModules
    ) {
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

            String safeId = id.trim();
            String safeName = name.trim();

            if (!includeDevelopmentModules
                && ID_RED_LEDGER.equals(
                    safeId
                )) {
                continue;
            }

            result.add(
                new Descriptor(
                    safeId,
                    safeName
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

        String safeModuleId =
            moduleId == null
                ? ""
                : moduleId.trim();
        if (!isSafeModuleId(safeModuleId)) {
            ReverieLog.incident(
                "NATIVE_MODULE",
                "Rejected unsafe native module id."
            );
            return false;
        }

        if (ID_RED_LEDGER.equals(
                safeModuleId
            )
            && !developmentModulesAllowed) {
            ReverieLog.incident(
                "NATIVE_MODULE",
                "Rejected development-only native module outside Development logging mode."
            );
            return false;
        }

        if (storageRoot == null) {
            ReverieLog.incident(
                "NATIVE_MODULE",
                "Native module storage root is unavailable."
            );
            return false;
        }

        File moduleStorage =
            new File(
                storageRoot,
                safeModuleId
            );
        if ((!moduleStorage.isDirectory()
                && !moduleStorage.mkdirs())
            || !moduleStorage.isDirectory()) {
            ReverieLog.incident(
                "NATIVE_MODULE",
                "Could not create private native module storage."
            );
            return false;
        }

        long created =
            nativeCreate(
                safeModuleId,
                moduleStorage.getAbsolutePath()
            );
        if (created == 0L) {
            ReverieLog.incident(
                "NATIVE_MODULE",
                "Native module launch failed: "
                    + getLastError()
            );
            return false;
        }

        handle = created;
        activeModuleId = safeModuleId;
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

    synchronized void update(
        int pointerKind,
        float[] pointerOrigin,
        float[] pointerDirection
    ) {
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

        int safePointerKind = pointerKind;
        if (safePointerKind != POINTER_TRACKED_CONTROLLER
            && safePointerKind != POINTER_VIRTUAL_CONTROLLER) {
            safePointerKind = POINTER_NONE;
        }

        float originX = 0.0f;
        float originY = 0.0f;
        float originZ = 0.0f;
        float directionX = 0.0f;
        float directionY = 0.0f;
        float directionZ = 0.0f;

        if (safePointerKind != POINTER_NONE
            && pointerOrigin != null
            && pointerOrigin.length >= 3
            && pointerDirection != null
            && pointerDirection.length >= 3) {
            originX = pointerOrigin[0];
            originY = pointerOrigin[1];
            originZ = pointerOrigin[2];
            directionX = pointerDirection[0];
            directionY = pointerDirection[1];
            directionZ = pointerDirection[2];
        } else {
            safePointerKind = POINTER_NONE;
        }

        nativeUpdate(
            handle,
            deltaSeconds,
            moveX,
            moveY,
            primary,
            secondary,
            safePointerKind,
            originX,
            originY,
            originZ,
            directionX,
            directionY,
            directionZ
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
        String moduleId,
        String storageRoot
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
        boolean secondaryDown,
        int pointerKind,
        float pointerOriginX,
        float pointerOriginY,
        float pointerOriginZ,
        float pointerDirectionX,
        float pointerDirectionY,
        float pointerDirectionZ
    );

    private static native boolean nativeRenderEye(
        long handle,
        int eyeIndex,
        float[] view,
        float[] projection
    );

    private static boolean isSafeModuleId(
        String value
    ) {
        if (value == null
            || value.isEmpty()
            || value.length() > 80) {
            return false;
        }

        for (int index = 0;
             index < value.length();
             index++) {
            char ch = value.charAt(index);
            boolean safe =
                (ch >= 'a' && ch <= 'z')
                    || (ch >= 'A' && ch <= 'Z')
                    || (ch >= '0' && ch <= '9')
                    || ch == '-'
                    || ch == '_'
                    || ch == '.';
            if (!safe) {
                return false;
            }
        }
        return true;
    }

    private static native String nativeGetLastError();
}
