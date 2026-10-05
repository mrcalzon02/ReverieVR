package io.github.mrcalzon02.reverievr;

import android.app.Application;

public final class ReverieApplication extends Application {
    private ControllerManager controllerManager;
    private InputBindingManager inputBindingManager;

    @Override
    public void onCreate() {
        super.onCreate();

        ReveriePreferences preferences =
            new ReveriePreferences(this);
        ReverieLog.initialize(
            this,
            preferences.getLoggingMode()
        );
        installCrashCapture();

        ReverieLog.milestone(
            "APP",
            "Application process started."
        );

        controllerManager = new ControllerManager(this);
        inputBindingManager = new InputBindingManager(this);
    }

    private void installCrashCapture() {
        Thread.UncaughtExceptionHandler previous =
            Thread.getDefaultUncaughtExceptionHandler();

        Thread.setDefaultUncaughtExceptionHandler(
            (thread, throwable) -> {
                String threadName =
                    thread == null
                        ? "unknown"
                        : thread.getName();

                ReverieLog.fatal(
                    "CRASH",
                    "Unhandled exception on thread "
                        + threadName,
                    throwable
                );

                if (previous != null) {
                    previous.uncaughtException(
                        thread,
                        throwable
                    );
                }
            }
        );
    }

    ControllerManager getControllerManager() {
        return controllerManager;
    }

    InputBindingManager getInputBindingManager() {
        return inputBindingManager;
    }

    @Override
    public void onTerminate() {
        if (inputBindingManager != null) {
            inputBindingManager.getEngine().releaseAll();
        }
        if (controllerManager != null) {
            controllerManager.close();
        }
        ReverieLog.milestone(
            "APP",
            "Application process terminating."
        );
        ReverieLog.shutdown();
        super.onTerminate();
    }
}
