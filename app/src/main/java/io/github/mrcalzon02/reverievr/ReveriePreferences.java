package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.content.SharedPreferences;

final class ReveriePreferences {
    private static final String FILE_NAME = "reverie_settings";

    private static final String KEY_BATTERY_HUD = "battery_hud";
    private static final String KEY_LOOK_UP_REVEAL = "look_up_reveal";
    private static final String KEY_SHOW_PERCENTAGES = "show_percentages";
    private static final String KEY_RETRO_MODE = "retro_mode";
    private static final String KEY_AUTO_UPDATE_CHECK = "auto_update_check";

    private final SharedPreferences preferences;

    ReveriePreferences(Context context) {
        preferences = context.getSharedPreferences(FILE_NAME, Context.MODE_PRIVATE);
    }

    boolean isBatteryHudEnabled() {
        return preferences.getBoolean(KEY_BATTERY_HUD, true);
    }

    void setBatteryHudEnabled(boolean enabled) {
        preferences.edit().putBoolean(KEY_BATTERY_HUD, enabled).apply();
    }

    boolean isLookUpRevealEnabled() {
        return preferences.getBoolean(KEY_LOOK_UP_REVEAL, true);
    }

    void setLookUpRevealEnabled(boolean enabled) {
        preferences.edit().putBoolean(KEY_LOOK_UP_REVEAL, enabled).apply();
    }

    boolean isShowPercentagesEnabled() {
        return preferences.getBoolean(KEY_SHOW_PERCENTAGES, true);
    }

    void setShowPercentagesEnabled(boolean enabled) {
        preferences.edit().putBoolean(KEY_SHOW_PERCENTAGES, enabled).apply();
    }

    boolean isRetroModeEnabled() {
        return preferences.getBoolean(KEY_RETRO_MODE, true);
    }

    void setRetroModeEnabled(boolean enabled) {
        preferences.edit().putBoolean(KEY_RETRO_MODE, enabled).apply();
    }

    boolean isAutoUpdateCheckEnabled() {
        return preferences.getBoolean(KEY_AUTO_UPDATE_CHECK, true);
    }

    void setAutoUpdateCheckEnabled(boolean enabled) {
        preferences.edit().putBoolean(KEY_AUTO_UPDATE_CHECK, enabled).apply();
    }

    void reset() {
        preferences.edit().clear().apply();
    }
}
