package io.github.mrcalzon02.reverievr;

import java.io.File;

/** Real Java runtime bridge, instrumented JNI double; no Android/GL claim. */
public final class NativeRuntimePauseProbe {
    private static native int calls(int kind);
    private static native void allowExit(boolean allow);
    private static native float lastDelta();
    private static native boolean lastPrimary();
    private static native float lastMoveX();
    private static int checks;
    private static void check(boolean condition, String message) {
        checks++;
        if (!condition) throw new AssertionError(message);
    }
    public static void main(String[] args) throws Exception {
        File root = new File(args[0], "native-modules");
        check(root.mkdirs() || root.isDirectory(), "test storage unavailable");
        check(NativeModuleRuntime.isAvailable(), "test JNI bridge did not load");
        VirtualInputBus bus = new VirtualInputBus();
        try (NativeModuleRuntime runtime = new NativeModuleRuntime(
                root, null, bus, true, null)) {
            check(runtime.start("classic-snake"), "initial start");
            check(calls(0) == 1 && calls(1) == 1, "create and initial resume");
            runtime.update(0, null, null);
            check(calls(3) == 1, "active update must reach JNI");
            check(!runtime.start("../invalid") && runtime.isRunning()
                && !runtime.isPaused(), "invalid replacement preserves active session and focus");
            runtime.setShellPaused(true);
            check(calls(2) == 1, "menu entry must pause exactly once");
            for (int i = 0; i < 100; i++) runtime.update(0, null, null);
            check(calls(3) == 1, "menu must suppress actual JNI updates");
            runtime.pauseForLifecycle();
            check(calls(2) == 1, "overlapping pause must not duplicate callback");
            runtime.setShellPaused(false);
            runtime.update(0, null, null);
            check(runtime.isPaused() && calls(1) == 1 && calls(3) == 1,
                "menu close must not override Activity pause");
            Thread.sleep(250);
            runtime.resumeForLifecycle();
            runtime.update(0, null, null);
            check(!runtime.isPaused() && calls(1) == 2 && calls(3) == 2,
                "final pause release must resume once");
            check(lastDelta() < .1f, "pause duration must not become catch-up time");
            runtime.setShellPaused(true);
            runtime.pauseForLifecycle();
            runtime.resumeForLifecycle();
            runtime.update(0, null, null);
            check(runtime.isPaused() && calls(1) == 2 && calls(3) == 2,
                "Activity resume must not override an open menu");
            allowExit(false);
            check(!runtime.prepareForExit() && runtime.isRunning(),
                "failed save must retain live instance");
            check(!runtime.start("classic-pong")
                && "classic-snake".equals(runtime.getActiveModuleId())
                && calls(0) == 1 && calls(4) == 0,
                "new launch must not destroy a blocked session");
            allowExit(true);
            check(runtime.prepareForExit(), "exit retry must become possible");
            runtime.stop();
            check(!runtime.isRunning() && calls(4) == 1, "approved stop destroys once");
            runtime.pauseForLifecycle();
            check(runtime.start("classic-pong"), "start under Activity pause");
            runtime.update(0, null, null);
            check(calls(3) == 2, "start under Activity pause must stay paused");
            runtime.resumeForLifecycle();
            runtime.update(0, null, null);
            check(calls(3) == 3, "resumed second game must update");
            bus.applyDigital(VirtualOutput.joystickButton(0), true);
            bus.applyDigital(VirtualOutput.joystickButton(0), false);
            runtime.update(0, null, null);
            check(lastPrimary(), "completed inter-frame click must reach native input once");
            runtime.update(0, null, null);
            check(!lastPrimary(), "completed click must not replay on the next frame");
            allowExit(false);
            check(!runtime.start("classic-snake") && !runtime.isPaused()
                && "classic-pong".equals(runtime.getActiveModuleId()),
                "refused active replacement restores previous focus intent");
            int before = calls(3);
            runtime.update(0, null, null);
            check(calls(3) == before + 1, "retained game remains playable after refused replacement");
            allowExit(true);
            check(runtime.start("sitka"), "test-double start for source-only Sitka bridge");
            runtime.setShellPaused(true);
            runtime.setSitkaWalkTouchpad(.8f, 0f, true);
            runtime.setShellPaused(false);
            runtime.update(0, null, null);
            check(lastMoveX() == 0f, "paused touch injection cannot survive into resumed Sitka");
            runtime.setSitkaWalkTouchpad(.8f, 0f, true);
            runtime.update(0, null, null);
            check(lastMoveX() > .7f, "fresh active Sitka touch still reaches native input");
        }
        System.out.println("PASS " + checks + " real Java runtime lifecycle assertions; JNI is a counting test double");
    }
}
