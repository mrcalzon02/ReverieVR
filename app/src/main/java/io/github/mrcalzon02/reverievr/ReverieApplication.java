package io.github.mrcalzon02.reverievr;

import android.app.Application;

public final class ReverieApplication extends Application {
    private ControllerManager controllerManager;

    @Override
    public void onCreate() {
        super.onCreate();
        controllerManager = new ControllerManager(this);
    }

    ControllerManager getControllerManager() {
        return controllerManager;
    }

    @Override
    public void onTerminate() {
        if (controllerManager != null) {
            controllerManager.close();
        }
        super.onTerminate();
    }
}
