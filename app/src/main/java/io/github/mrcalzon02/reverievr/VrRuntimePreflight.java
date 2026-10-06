package io.github.mrcalzon02.reverievr;

final class VrRuntimePreflight {
    static final class Result {
        final boolean available;
        final String detail;

        Result(boolean available, String detail) {
            this.available = available;
            this.detail = detail == null ? "" : detail;
        }
    }

    private static Result cached;

    private VrRuntimePreflight() {
    }

    static synchronized Result check() {
        if (cached != null) {
            return cached;
        }

        try {
            System.loadLibrary("cardboard_sdk_jni");
            cached = new Result(
                true,
                "Cardboard native runtime loaded."
            );
        } catch (RuntimeException | LinkageError failure) {
            String message = failure.getMessage();
            String detail =
                failure.getClass().getSimpleName()
                    + (
                        message == null
                            || message.trim().isEmpty()
                            ? ""
                            : ": " + sanitize(message)
                    );
            cached = new Result(false, detail);
        }

        return cached;
    }

    private static String sanitize(String value) {
        String cleaned =
            value == null
                ? ""
                : value
                    .replace('\n', ' ')
                    .replace('\r', ' ')
                    .trim();

        if (cleaned.length() <= 240) {
            return cleaned;
        }
        return cleaned.substring(0, 240);
    }
}
