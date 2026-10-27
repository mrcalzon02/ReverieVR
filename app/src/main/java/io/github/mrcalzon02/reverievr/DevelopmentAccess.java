package io.github.mrcalzon02.reverievr;

/**
 * Explicit testing privileges. A persisted preference can never turn a
 * Google Play binary into a sideloading or executable-downloading app.
 * LoggingMode.DEVELOPMENT is intentionally unrelated to these privileges.
 */
final class DevelopmentAccess {
    private DevelopmentAccess() {}

    static boolean permitted(boolean playDistribution, boolean developerMode,
                             boolean selectedFeature) {
        return !playDistribution && developerMode && selectedFeature;
    }

    static boolean gameContent(ReveriePreferences settings) {
        return permitted(BuildConfig.PLAY_DISTRIBUTION,
            settings.isDeveloperModeEnabled(),
            settings.isDeveloperContentDownloadsEnabled());
    }

    static boolean companionApks(ReveriePreferences settings) {
        return permitted(BuildConfig.PLAY_DISTRIBUTION,
            settings.isDeveloperModeEnabled(),
            settings.isDeveloperCompanionApksEnabled());
    }

    static boolean emulatorSources(ReveriePreferences settings) {
        return permitted(BuildConfig.PLAY_DISTRIBUTION,
            settings.isDeveloperModeEnabled(),
            settings.isDeveloperEmulatorSourcesEnabled());
    }

    static boolean selfUpdates(ReveriePreferences settings) {
        return BuildConfig.UPDATE_CHANNEL_ENABLED
            && permitted(BuildConfig.PLAY_DISTRIBUTION,
                settings.isDeveloperModeEnabled(),
                settings.isDeveloperAppUpdatesEnabled());
    }

    static boolean gameManager(ReveriePreferences settings) {
        return gameContent(settings) || companionApks(settings);
    }
}
