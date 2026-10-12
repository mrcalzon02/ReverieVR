package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.content.SharedPreferences;

final class ReveriePreferences {
    static final int CURRENT_VR_SETUP_VERSION = 2;

    private static final String FILE_NAME = "reverie_settings";

    private static final String KEY_BATTERY_HUD = "battery_hud";
    private static final String KEY_LOOK_UP_REVEAL = "look_up_reveal";
    private static final String KEY_SHOW_PERCENTAGES = "show_percentages";
    private static final String KEY_AUTO_UPDATE_CHECK = "auto_update_check";
    private static final String KEY_LOGGING_MODE = "logging_mode";
    private static final String KEY_VR_SETUP_VERSION = "vr_setup_version";
    private static final String KEY_VR_SETUP_STEP = "vr_setup_step";
    private static final String KEY_USER_IPD_METERS = "user_ipd_meters";
    private static final String KEY_UI_SCALE = "ui_scale";
    private static final String KEY_VIDEO_URI = "video_uri";
    private static final String KEY_VIDEO_DISPLAY_NAME = "video_display_name";
    private static final String KEY_VIDEO_PROJECTION = "video_projection";
    private static final String KEY_VIDEO_LENS_UNWARP = "video_lens_unwarp";
    private static final String KEY_HOME_ENVIRONMENT = "home_environment";
    private static final String KEY_VR_POINTER_MODE = "vr_pointer_mode";
    private static final String KEY_CONTROLLER_LEFT_HANDED = "controller_left_handed";
    private static final String KEY_ACTIVE_CONTROLLER = "selected_controller";
    static final String CONTROLLER_AUTO = "auto";
    static final String CONTROLLER_DAYDREAM = "daydream";
    static final String CONTROLLER_STEAM = "steam";
    static final String CONTROLLER_PHONE = "phone";
    static final String CONTROLLER_SCRCPY = "scrcpy";
    private static final String KEY_LAST_ACTIVITY_TYPE = "last_activity_type";
    private static final String KEY_LAST_DOS_MODULE_ID = "last_dos_module_id";
    private static final String KEY_LAST_DOS_MODULE_NAME = "last_dos_module_name";
    private static final String KEY_LAST_NATIVE_MODULE_ID = "last_native_module_id";
    private static final String KEY_LAST_NATIVE_MODULE_NAME = "last_native_module_name";

    static final String LAST_ACTIVITY_NONE = "";
    static final String LAST_ACTIVITY_MEDIA = "media";
    static final String LAST_ACTIVITY_DOS = "dos";
    static final String LAST_ACTIVITY_NATIVE = "native";

    private final SharedPreferences preferences;

    ReveriePreferences(Context context) {
        preferences = context.getSharedPreferences(FILE_NAME, Context.MODE_PRIVATE);
    }

    String getActiveController() {
        String choice = preferences.getString(KEY_ACTIVE_CONTROLLER, CONTROLLER_AUTO);
        return CONTROLLER_DAYDREAM.equals(choice) || CONTROLLER_STEAM.equals(choice)
            || CONTROLLER_PHONE.equals(choice) || CONTROLLER_SCRCPY.equals(choice)
            ? choice : CONTROLLER_AUTO;
    }

    void setActiveController(String choice) {
        if (!CONTROLLER_AUTO.equals(choice) && !CONTROLLER_DAYDREAM.equals(choice)
            && !CONTROLLER_STEAM.equals(choice) && !CONTROLLER_PHONE.equals(choice)
            && !CONTROLLER_SCRCPY.equals(choice))
            throw new IllegalArgumentException("Unknown controller");
        preferences.edit().putString(KEY_ACTIVE_CONTROLLER, choice).apply();
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

    boolean isAutoUpdateCheckEnabled() {
        return preferences.getBoolean(KEY_AUTO_UPDATE_CHECK, true);
    }

    void setAutoUpdateCheckEnabled(boolean enabled) {
        preferences.edit().putBoolean(KEY_AUTO_UPDATE_CHECK, enabled).apply();
    }

    LoggingMode getLoggingMode() {
        return LoggingMode.fromPreference(
            preferences.getString(
                KEY_LOGGING_MODE,
                LoggingMode.STANDARD.name()
            )
        );
    }

    void setLoggingMode(LoggingMode mode) {
        LoggingMode safe =
            mode == null
                ? LoggingMode.STANDARD
                : mode;
        preferences.edit()
            .putString(KEY_LOGGING_MODE, safe.name())
            .apply();
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

        SharedPreferences.Editor editor = preferences.edit()
            .putString(KEY_VIDEO_URI, uri)
            .putString(
                KEY_VIDEO_DISPLAY_NAME,
                displayName == null ? "" : displayName
            );
        // A new video has its own format. Never carry a previous movie's 3D
        // interpretation into an unrelated ordinary 2D file.
        if (!uri.equals(getSelectedVideoUri())) {
            editor.putString(KEY_VIDEO_PROJECTION,
                VideoProjection.suggestForFileName(displayName).name());
            // Optical adjustment belongs to this recording, never the next.
            editor.putInt(KEY_VIDEO_LENS_UNWARP, 0);
        }
        editor.apply();
    }

    void clearSelectedVideo() {
        SharedPreferences.Editor editor =
            preferences.edit()
                .remove(KEY_VIDEO_URI)
                .remove(KEY_VIDEO_DISPLAY_NAME)
                .remove(KEY_VIDEO_PROJECTION)
                .remove(KEY_VIDEO_LENS_UNWARP);

        if (LAST_ACTIVITY_MEDIA.equals(
                getLastActivityType()
            )) {
            editor
                .remove(KEY_LAST_ACTIVITY_TYPE)
                .remove(KEY_LAST_DOS_MODULE_ID)
                .remove(KEY_LAST_DOS_MODULE_NAME);
        }

        editor.apply();
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

    int getVideoLensUnwarpStep() {
        return HeadsetLensUnwarp.clampStep(
            preferences.getInt(KEY_VIDEO_LENS_UNWARP, 0));
    }

    void setVideoLensUnwarpStep(int step) {
        preferences.edit().putInt(
            KEY_VIDEO_LENS_UNWARP, HeadsetLensUnwarp.clampStep(step)).apply();
    }

    /**
     * Respect both selection identity and an in-VR format override made while
     * metadata was being inspected. This check and update run on the UI thread.
     */
    boolean applyDetectedVideoProjection(String uri,
                                         VideoProjection expected,
                                         VideoProjection detected) {
        if (uri == null || detected == null
            || !uri.equals(getSelectedVideoUri())
            || getVideoProjection() != expected) {
            return false;
        }
        if (detected == expected) return false;
        setVideoProjection(detected);
        return true;
    }

    HomeEnvironment getHomeEnvironment() {
        return HomeEnvironment.fromPreference(
            preferences.getString(
                KEY_HOME_ENVIRONMENT,
                HomeEnvironment.WHITE_ROOM.preferenceValue
            )
        );
    }

    void setHomeEnvironment(
        HomeEnvironment environment
    ) {
        HomeEnvironment safe =
            environment == null
                ? HomeEnvironment.WHITE_ROOM
                : environment;
        preferences.edit()
            .putString(
                KEY_HOME_ENVIRONMENT,
                safe.preferenceValue
            )
            .apply();
    }

    VrPointerMode getVrPointerMode() {
        return VrPointerMode.fromPreference(
            preferences.getString(
                KEY_VR_POINTER_MODE,
                VrPointerMode.AUTO.preferenceValue
            )
        );
    }

    void setVrPointerMode(VrPointerMode mode) {
        VrPointerMode safe =
            mode == null
                ? VrPointerMode.AUTO
                : mode;
        preferences.edit()
            .putString(
                KEY_VR_POINTER_MODE,
                safe.preferenceValue
            )
            .apply();
    }

    boolean isControllerLeftHanded() {
        return preferences.getBoolean(KEY_CONTROLLER_LEFT_HANDED, false);
    }

    void setControllerLeftHanded(boolean leftHanded) {
        preferences.edit()
            .putBoolean(KEY_CONTROLLER_LEFT_HANDED, leftHanded)
            .apply();
    }

    String getLastActivityType() {
        String value =
            preferences.getString(
                KEY_LAST_ACTIVITY_TYPE,
                LAST_ACTIVITY_NONE
            );
        if (LAST_ACTIVITY_MEDIA.equals(value)
            || LAST_ACTIVITY_DOS.equals(value)
            || LAST_ACTIVITY_NATIVE.equals(value)) {
            return value;
        }
        return LAST_ACTIVITY_NONE;
    }

    void markLastActivityMedia() {
        preferences.edit()
            .putString(
                KEY_LAST_ACTIVITY_TYPE,
                LAST_ACTIVITY_MEDIA
            )
            .remove(KEY_LAST_DOS_MODULE_ID)
            .remove(KEY_LAST_DOS_MODULE_NAME)
            .remove(KEY_LAST_NATIVE_MODULE_ID)
            .remove(KEY_LAST_NATIVE_MODULE_NAME)
            .apply();
    }

    void markLastActivityDos(
        String moduleId,
        String displayName
    ) {
        String safeId =
            moduleId == null
                ? ""
                : moduleId.trim();
        if (safeId.isEmpty()) {
            return;
        }

        preferences.edit()
            .putString(
                KEY_LAST_ACTIVITY_TYPE,
                LAST_ACTIVITY_DOS
            )
            .putString(
                KEY_LAST_DOS_MODULE_ID,
                safeId
            )
            .putString(
                KEY_LAST_DOS_MODULE_NAME,
                displayName == null
                    ? ""
                    : displayName.trim()
            )
            .remove(KEY_LAST_NATIVE_MODULE_ID)
            .remove(KEY_LAST_NATIVE_MODULE_NAME)
            .apply();
    }

    void markLastActivityNative(
        String moduleId,
        String displayName
    ) {
        String safeId =
            moduleId == null
                ? ""
                : moduleId.trim();
        if (safeId.isEmpty()) {
            return;
        }

        preferences.edit()
            .putString(
                KEY_LAST_ACTIVITY_TYPE,
                LAST_ACTIVITY_NATIVE
            )
            .putString(
                KEY_LAST_NATIVE_MODULE_ID,
                safeId
            )
            .putString(
                KEY_LAST_NATIVE_MODULE_NAME,
                displayName == null
                    ? ""
                    : displayName.trim()
            )
            .remove(KEY_LAST_DOS_MODULE_ID)
            .remove(KEY_LAST_DOS_MODULE_NAME)
            .apply();
    }

    String getLastDosModuleId() {
        String value =
            preferences.getString(
                KEY_LAST_DOS_MODULE_ID,
                ""
            );
        return value == null
            ? ""
            : value.trim();
    }

    String getLastDosModuleName() {
        String value =
            preferences.getString(
                KEY_LAST_DOS_MODULE_NAME,
                ""
            );
        return value == null
            ? ""
            : value.trim();
    }

    String getLastNativeModuleId() {
        String value =
            preferences.getString(
                KEY_LAST_NATIVE_MODULE_ID,
                ""
            );
        return value == null
            ? ""
            : value.trim();
    }

    String getLastNativeModuleName() {
        String value =
            preferences.getString(
                KEY_LAST_NATIVE_MODULE_NAME,
                ""
            );
        return value == null
            ? ""
            : value.trim();
    }

    void clearLastActivity() {
        preferences.edit()
            .remove(KEY_LAST_ACTIVITY_TYPE)
            .remove(KEY_LAST_DOS_MODULE_ID)
            .remove(KEY_LAST_DOS_MODULE_NAME)
            .remove(KEY_LAST_NATIVE_MODULE_ID)
            .remove(KEY_LAST_NATIVE_MODULE_NAME)
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
