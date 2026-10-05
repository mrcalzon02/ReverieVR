package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.content.SharedPreferences;

final class VrStartupGuard {
    private static final String PREFS =
        "reverie-vr-startup";
    private static final String KEY_ACTIVE =
        "active";
    private static final String KEY_PHASE =
        "phase";
    private static final String KEY_DETAIL =
        "detail";

    private VrStartupGuard() {
    }

    static void begin(Context context) {
        preferences(context)
            .edit()
            .putBoolean(KEY_ACTIVE, true)
            .putString(KEY_PHASE, "launch-requested")
            .putString(KEY_DETAIL, "")
            .apply();
    }

    static void setPhase(
        Context context,
        String phase
    ) {
        SharedPreferences preferences =
            preferences(context);
        if (!preferences.getBoolean(KEY_ACTIVE, false)) {
            return;
        }

        preferences
            .edit()
            .putString(
                KEY_PHASE,
                sanitize(phase, 96)
            )
            .apply();
    }

    static void recordFailure(
        Context context,
        String phase,
        Throwable throwable
    ) {
        String detail = "";
        if (throwable != null) {
            String message = throwable.getMessage();
            detail =
                throwable.getClass().getSimpleName()
                    + (message == null
                        || message.trim().isEmpty()
                        ? ""
                        : ": " + message.trim());
        }

        preferences(context)
            .edit()
            .putBoolean(KEY_ACTIVE, true)
            .putString(
                KEY_PHASE,
                sanitize(phase, 96)
            )
            .putString(
                KEY_DETAIL,
                sanitize(detail, 320)
            )
            .commit();
    }

    static void markFirstFrame(Context context) {
        clear(context);
    }

    static String consumePendingFailure(
        Context context
    ) {
        SharedPreferences preferences =
            preferences(context);
        if (!preferences.getBoolean(KEY_ACTIVE, false)) {
            return "";
        }

        String phase =
            preferences.getString(
                KEY_PHASE,
                "unknown"
            );
        String detail =
            preferences.getString(
                KEY_DETAIL,
                ""
            );

        clear(context);

        StringBuilder message =
            new StringBuilder(
                "The previous VR launch ended before the first frame."
            );
        message
            .append("\n\nStartup phase: ")
            .append(
                phase == null
                    || phase.trim().isEmpty()
                    ? "unknown"
                    : phase.trim()
            );

        if (detail != null
            && !detail.trim().isEmpty()) {
            message
                .append("\nFailure: ")
                .append(detail.trim());
        }

        message.append(
            "\n\nEnable Development logging and export a diagnostic bundle "
                + "after reproducing if this happens again."
        );

        return message.toString();
    }

    static void clear(Context context) {
        preferences(context)
            .edit()
            .clear()
            .commit();
    }

    private static SharedPreferences preferences(
        Context context
    ) {
        return context
            .getApplicationContext()
            .getSharedPreferences(
                PREFS,
                Context.MODE_PRIVATE
            );
    }

    private static String sanitize(
        String value,
        int maxLength
    ) {
        if (value == null) {
            return "";
        }

        String cleaned =
            value.replace('\n', ' ')
                .replace('\r', ' ')
                .trim();
        if (cleaned.length() <= maxLength) {
            return cleaned;
        }
        return cleaned.substring(
            0,
            Math.max(0, maxLength)
        );
    }
}
