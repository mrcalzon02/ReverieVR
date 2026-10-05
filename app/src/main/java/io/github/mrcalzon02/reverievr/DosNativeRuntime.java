package io.github.mrcalzon02.reverievr;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

final class DosNativeRuntime implements AutoCloseable {
    private static final int[] RETRO_KEY_MAP =
        DosLibretroKeyMap.create();

    private static final boolean NATIVE_LOADED;

    static {
        boolean loaded = false;
        if (BuildConfig.DOS_RUNTIME_BUILT) {
            try {
                System.loadLibrary("retro");
                System.loadLibrary("reverie_dos_host");
                loaded = true;
            } catch (UnsatisfiedLinkError ignored) {
                loaded = false;
            }
        }
        NATIVE_LOADED = loaded;
    }

    private final VirtualInputSnapshot inputSnapshot =
        new VirtualInputSnapshot();

    private long handle;
    private ByteBuffer frameBuffer;
    private ByteBuffer audioBuffer;

    static boolean isAvailable() {
        return NATIVE_LOADED;
    }

    DosNativeRuntime(
        String systemDirectory,
        String saveDirectory
    ) {
        if (!NATIVE_LOADED) {
            throw new IllegalStateException(
                "Native DOS runtime was not built into this APK."
            );
        }

        handle = nativeCreate(
            systemDirectory,
            saveDirectory
        );
        if (handle == 0L) {
            throw new IllegalStateException(
                "Could not create the DOS runtime host."
            );
        }
    }

    boolean loadContent(String contentPath) {
        requireOpen();
        return nativeLoadContent(handle, contentPath);
    }

    void updateInput(VirtualInputBus bus) {
        requireOpen();
        if (bus == null) {
            nativeReleaseAllInput(handle);
            return;
        }

        bus.capture(inputSnapshot);

        nativeUpdateInput(
            handle,
            RETRO_KEY_MAP,
            inputSnapshot.keys,
            inputSnapshot.mouseButtons,
            inputSnapshot.mouseRelativeX,
            inputSnapshot.mouseRelativeY,
            inputSnapshot.mouseAbsoluteX,
            inputSnapshot.mouseAbsoluteY,
            inputSnapshot.mouseWheel,
            inputSnapshot.joystickAxisX,
            inputSnapshot.joystickAxisY,
            inputSnapshot.joystickButtons
        );
    }

    void runFrame() {
        requireOpen();
        nativeRunFrame(handle);
    }

    double getFramesPerSecond() {
        requireOpen();
        return nativeGetFramesPerSecond(handle);
    }

    double getAudioSampleRate() {
        requireOpen();
        return nativeGetAudioSampleRate(handle);
    }

    int getFrameWidth() {
        requireOpen();
        return nativeGetFrameWidth(handle);
    }

    int getFrameHeight() {
        requireOpen();
        return nativeGetFrameHeight(handle);
    }

    long getFrameSerial() {
        requireOpen();
        return nativeGetFrameSerial(handle);
    }

    ByteBuffer copyLatestFrame() {
        requireOpen();

        int required = nativeGetFrameByteCount(handle);
        if (required <= 0) {
            return null;
        }

        if (frameBuffer == null
            || frameBuffer.capacity() < required) {
            frameBuffer = ByteBuffer.allocateDirect(required)
                .order(ByteOrder.nativeOrder());
        }

        frameBuffer.clear();
        int copied = nativeCopyLatestFrame(
            handle,
            frameBuffer,
            frameBuffer.capacity()
        );
        if (copied <= 0) {
            return null;
        }

        frameBuffer.limit(copied);
        frameBuffer.position(0);
        return frameBuffer;
    }

    ByteBuffer readAudio(int maxStereoFrames) {
        requireOpen();

        if (maxStereoFrames <= 0) {
            return null;
        }

        int bytes = maxStereoFrames * 2 * 2;
        if (audioBuffer == null
            || audioBuffer.capacity() < bytes) {
            audioBuffer = ByteBuffer.allocateDirect(bytes)
                .order(ByteOrder.nativeOrder());
        }

        audioBuffer.clear();
        int frames = nativeReadAudio(
            handle,
            audioBuffer,
            maxStereoFrames
        );
        if (frames <= 0) {
            return null;
        }

        audioBuffer.limit(frames * 2 * 2);
        audioBuffer.position(0);
        return audioBuffer;
    }

    boolean isShutdownRequested() {
        requireOpen();
        return nativeIsShutdownRequested(handle);
    }

    String getLastError() {
        if (handle == 0L) {
            return "DOS runtime is closed.";
        }

        String value = nativeGetLastError(handle);
        return value == null ? "" : value;
    }

    void releaseAllInput() {
        if (handle != 0L) {
            nativeReleaseAllInput(handle);
        }
    }

    private void requireOpen() {
        if (handle == 0L) {
            throw new IllegalStateException(
                "DOS runtime is closed."
            );
        }
    }

    @Override
    public void close() {
        long current = handle;
        handle = 0L;

        if (current != 0L) {
            nativeDestroy(current);
        }
    }

    private static native long nativeCreate(
        String systemDirectory,
        String saveDirectory
    );

    private static native boolean nativeLoadContent(
        long handle,
        String contentPath
    );

    private static native void nativeUpdateInput(
        long handle,
        int[] retroKeyMap,
        boolean[] keys,
        boolean[] mouseButtons,
        float mouseRelativeX,
        float mouseRelativeY,
        float mouseAbsoluteX,
        float mouseAbsoluteY,
        float mouseWheel,
        float joystickAxisX,
        float joystickAxisY,
        boolean[] joystickButtons
    );

    private static native void nativeReleaseAllInput(
        long handle
    );

    private static native void nativeRunFrame(long handle);
    private static native double nativeGetFramesPerSecond(long handle);
    private static native double nativeGetAudioSampleRate(long handle);
    private static native int nativeGetFrameWidth(long handle);
    private static native int nativeGetFrameHeight(long handle);
    private static native int nativeGetFrameByteCount(long handle);
    private static native long nativeGetFrameSerial(long handle);

    private static native int nativeCopyLatestFrame(
        long handle,
        ByteBuffer target,
        int capacity
    );

    private static native int nativeReadAudio(
        long handle,
        ByteBuffer target,
        int maxStereoFrames
    );

    private static native boolean nativeIsShutdownRequested(
        long handle
    );

    private static native String nativeGetLastError(long handle);
    private static native void nativeDestroy(long handle);
}
