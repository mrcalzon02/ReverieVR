package io.github.mrcalzon02.reverievr;

enum VrPointerMode {
    AUTO("auto", "Auto"),
    GAZE("gaze", "Gaze"),
    CONTROLLER("controller", "Controller");

    final String preferenceValue;
    final String displayName;

    VrPointerMode(
        String preferenceValue,
        String displayName
    ) {
        this.preferenceValue = preferenceValue;
        this.displayName = displayName;
    }

    static VrPointerMode fromPreference(String value) {
        if (value != null) {
            for (VrPointerMode mode : values()) {
                if (mode.preferenceValue.equals(value)) {
                    return mode;
                }
            }
        }
        return AUTO;
    }

    VrPointerMode next() {
        switch (this) {
            case AUTO:
                return GAZE;
            case GAZE:
                return CONTROLLER;
            case CONTROLLER:
            default:
                return AUTO;
        }
    }
}
