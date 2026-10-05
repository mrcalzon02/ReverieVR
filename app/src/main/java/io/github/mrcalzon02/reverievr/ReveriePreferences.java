package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.content.SharedPreferences;

final class ReveriePreferences {
    static final int CURRENT_VR_SETUP_VERSION = 2;

    private static final String FILE_NAME = "reverie_settings";

    private static final String KEY_BATTERY_HUD = "battery_hud";
    private static final String KEY_LOOK_UP_REVEAL = "look_up_reveal";
    private static final String KEY_SHOW_PERCENTAGES = "show_percentages";
    private static final String KEY_RETRO_MODE = "retro_mode";
    private static final String KEY_AUTO_UPDATE_CHECK = "auto_update_check";
    private static final String KEY_VR_SETUP_VERSION = "vr_setup_version";
    private static final String KEY_VR_SETUP_STEP = "vr_setup_step";
    private static final String KEY_USER_IPD_METERS = "user_ipd_meters";
    private static final String KEY_UI_SCALE = "ui_scale";
    private static final String KEY_VIDEO_URI = "video_uri";
    private static final String KEY_VIDEO_DISPLAY_NAME = "video_display_name";
    private static final String KEY_VIDEO_PROJECTION = "video_projection";

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

    boolean isVrSetupCurrent() {
        return preferences.getInt(KEY_VR_SETUP_VERSION, 0) >= CURRENT_VR_SETUP_VERSION;
    }

    void markVrSetupCurrent() {
        preferences.edit()
            .putInt(KEY_VR_SETUP_VERSION, CURRENT_VR_SETUP_VERSION)
            .putInt(KEY_VR_SETUP_STEP, 0)
            .apply();
    }

    int getVrSetupStep() {
        return preferences.getInt(KEY_VR_SETUP_STEP, 0);
    }

    void setVrSetupStep(int step) {
        preferences.edit().putInt(KEY_VR_SETUP_STEP, Math.max(0, step)).apply();
    }

    float getUserIpdMeters(float viewerDefaultMeters) {
        float value = preferences.getFloat(KEY_USER_IPD_METERS, viewerDefaultMeters);
        return clamp(value, 0.050f, 0.080f);
    }

    void setUserIpdMeters(float meters) {
        preferences.edit()
            .putFloat(KEY_USER_IPD_METERS, clamp(meters, 0.050f, 0.080f))
            .apply();
    }

    float getUiScale() {
        return clamp(preferences.getFloat(KEY_UI_SCALE, 1.0f), 0.75f, 1.50f);
    }

    void setUiScale(float scale) {
        preferences.edit()
            .putFloat(KEY_UI_SCALE, clamp(scale, 0.75f, 1.50f))
            .apply();
    }

    boolean hasSelectedVideo() {
        String value = preferences.getString(KEY_VIDEO_URI, "");
        return value != null && !value.trim().isEmpty();
    }

    String getSelectedVideoUri() {
        String value = preferences.getString(KEY_VIDEO_URI, "");
        return value == null ? "" : value;
    }

    String getSelectedVideoDisplayName() {
        String value = preferences.getString(KEY_VIDEO_DISPLAY_NAME, "");
        return value == null ? "" : value;
    }

    void setSelectedVideo(String uri, String displayName) {
        if (uri == null || uri.trim().isEmpty()) {
            clearSelectedVideo();
            return;
        }

        preferences.edit()
            .putString(KEY_VIDEO_URI, uri)
            .putString(
                KEY_VIDEO_DISPLAY_NAME,
                displayName == null ? "" : displayName
            )
            .apply();
    }

    void clearSelectedVideo() {
        preferences.edit()
            .remove(KEY_VIDEO_URI)
            .remove(KEY_VIDEO_DISPLAY_NAME)
            .apply();
    }

    VideoProjection getVideoProjection() {
        return VideoProjection.fromPreference(
            preferences.getString(KEY_VIDEO_PROJECTION, null)
        );
    }

    void setVideoProjection(VideoProjection projection) {
        VideoProjection safe = projection == null
            ? VideoProjection.FLAT_CINEMA
            : projection;
        preferences.edit()
            .putString(KEY_VIDEO_PROJECTION, safe.name())
            .apply();
    }

    void resetVrSetup() {
        preferences.edit()
            .remove(KEY_VR_SETUP_VERSION)
            .remove(KEY_VR_SETUP_STEP)
            .remove(KEY_USER_IPD_METERS)
            .remove(KEY_UI_SCALE)
            .apply();
    }

    void reset() {
        preferences.edit().clear().apply();
    }

    private static float clamp(float value, float minimum, float maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }
}
