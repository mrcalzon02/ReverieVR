package io.github.mrcalzon02.reverievr;

import android.app.Application;

public final class ReverieApplication extends Application {
    private ControllerManager controllerManager;
    private InputBindingManager inputBindingManager;

    @Override
    public void onCreate() {
        super.onCreate();
        controllerManager = new ControllerManager(this);
        inputBindingManager = new InputBindingManager(this);
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
        super.onTerminate();
    }
}
