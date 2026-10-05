package io.github.mrcalzon02.reverievr;

enum LoggingMode {
    STANDARD,
    DEVELOPMENT;

    static LoggingMode fromPreference(String value) {
        if (value != null) {
            try {
                return LoggingMode.valueOf(value);
            } catch (IllegalArgumentException ignored) {
                // Fall through to the safe default.
            }
        }
        return STANDARD;
    }
}
