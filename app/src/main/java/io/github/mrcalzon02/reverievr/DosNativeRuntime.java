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

    // DOSBox Pure receives both a relative mouse and a normalized libretro
    // pointer. Their positions must not diverge when an input source (head,
    // controller, touchpad) only generates relative movement.
    private static final float POINTER_MOUSE_WIDTH = 640.0f;
    private static final float POINTER_MOUSE_HEIGHT = 480.0f;
    private volatile float guestPointerX = 0.5f;
    private volatile float guestPointerY = 0.5f;
    private float previousAbsoluteX = 0.5f;
    private float previousAbsoluteY = 0.5f;
    private float fractionalMouseX;
    private float fractionalMouseY;

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
            fractionalMouseX = 0.0f;
            fractionalMouseY = 0.0f;
            nativeReleaseAllInput(handle);
            return;
        }

        bus.capture(inputSnapshot);
        // A slow 3DoF aim may generate less than one native mouse tick
        // per frame. Carry the fractional pixels into the next DOS frame
        // instead of rounding them away permanently.
        float totalX = clampRelative(inputSnapshot.mouseRelativeX
            + fractionalMouseX);
        float totalY = clampRelative(inputSnapshot.mouseRelativeY
            + fractionalMouseY);
        float relativeX = Math.round(totalX);
        float relativeY = Math.round(totalY);
        fractionalMouseX = totalX - relativeX;
        fractionalMouseY = totalY - relativeY;

        // Honor explicit absolute HID movement where supplied; otherwise
        // integrate the same rounded/clamped relative units delivered to the
        // native RETRO_DEVICE_MOUSE into RETRO_DEVICE_POINTER coordinates.
        // This makes pointer-based launchers clickable without requiring an
        // external physical mouse and keeps the VR cursor tied to the guest.
        float absoluteX = inputSnapshot.mouseAbsoluteX;
        float absoluteY = inputSnapshot.mouseAbsoluteY;
        if (absoluteX != previousAbsoluteX) {
            guestPointerX = clamp01(absoluteX);
        } else {
            guestPointerX = clamp01(guestPointerX
                + relativeX / POINTER_MOUSE_WIDTH);
        }
        if (absoluteY != previousAbsoluteY) {
            guestPointerY = clamp01(absoluteY);
        } else {
            guestPointerY = clamp01(guestPointerY
                + relativeY / POINTER_MOUSE_HEIGHT);
        }
        previousAbsoluteX = absoluteX;
        previousAbsoluteY = absoluteY;

        nativeUpdateInput(
            handle,
            RETRO_KEY_MAP,
            inputSnapshot.keys,
            inputSnapshot.mouseButtons,
            relativeX,
            relativeY,
            guestPointerX,
            guestPointerY,
            inputSnapshot.mouseWheel,
            inputSnapshot.joystickAxisX,
            inputSnapshot.joystickAxisY,
            inputSnapshot.joystickButtons
        );
    }

    boolean copyGuestPointer(float[] outUv) {
        if (outUv == null || outUv.length < 2 || handle == 0L) {
            return false;
        }
        outUv[0] = guestPointerX;
        outUv[1] = guestPointerY;
        return true;
    }

    private static float clamp01(float value) {
        return Math.max(0.0f, Math.min(1.0f, value));
    }

    private static float clampRelative(float value) {
        if (!Float.isFinite(value)) return 0.0f;
        return Math.max(-32767.0f, Math.min(32767.0f, value));
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
        fractionalMouseX = 0.0f;
        fractionalMouseY = 0.0f;
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
