package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.content.SharedPreferences;

final class InputBindingManager {
    private static final String FILE_NAME = "reverie_input_bindings";
    private static final String KEY_PROFILE = "active_profile";

    private final SharedPreferences preferences;
    private final VirtualInputBus bus = new VirtualInputBus();
    private final InputBindingEngine engine;

    InputBindingManager(Context context) {
        preferences = context.getSharedPreferences(
            FILE_NAME,
            Context.MODE_PRIVATE
        );

        BindingProfile profile = loadProfile();
        engine = new InputBindingEngine(bus, profile);
    }

    InputBindingEngine getEngine() {
        return engine;
    }

    VirtualInputBus getBus() {
        return bus;
    }

    private BindingProfile hostedPreviousProfile;

    BindingProfile getProfile() {
        return engine.getProfile();
    }

    synchronized void beginHostedProfile(String id) {
        beginHostedProfile(
            BuiltInBindingProfiles.byId(id)
        );
    }

    synchronized void beginHostedProfile(
        BindingProfile profile
    ) {
        if (hostedPreviousProfile == null) {
            hostedPreviousProfile = engine.getProfile();
        }
        engine.setProfile(profile);
    }

    synchronized void selectHostedProfile(String id) {
        setHostedProfile(
            BuiltInBindingProfiles.byId(id)
        );
    }

    synchronized void setHostedProfile(
        BindingProfile profile
    ) {
        if (hostedPreviousProfile == null) {
            throw new IllegalStateException(
                "No hosted binding session is active."
            );
        }
        engine.releaseAll();
        engine.setProfile(profile);
    }

    synchronized boolean isHostedProfileActive() {
        return hostedPreviousProfile != null;
    }

    synchronized void endHostedProfile() {
        if (hostedPreviousProfile == null) {
            return;
        }

        BindingProfile restore = hostedPreviousProfile;
        hostedPreviousProfile = null;
        engine.setProfile(restore);
    }

    void selectBuiltInProfile(String id) {
        setProfile(BuiltInBindingProfiles.byId(id), true);
    }

    void setCustomProfile(BindingProfile profile) {
        setProfile(profile, true);
    }

    void clearHostedBindings() {
        setProfile(
            BuiltInBindingProfiles.byId(BuiltInBindingProfiles.ID_NONE),
            true
        );
    }

    private void setProfile(
        BindingProfile profile,
        boolean persist
    ) {
        engine.setProfile(profile);

        if (persist) {
            preferences.edit()
                .putString(
                    KEY_PROFILE,
                    BindingProfileCodec.encode(profile)
                )
                .apply();
        }
    }

    private BindingProfile loadProfile() {
        String encoded = preferences.getString(KEY_PROFILE, "");
        if (encoded == null || encoded.trim().isEmpty()) {
            return BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_NONE
            );
        }

        try {
            return BindingProfileCodec.decode(encoded);
        } catch (RuntimeException ignored) {
            preferences.edit().remove(KEY_PROFILE).apply();
            return BuiltInBindingProfiles.byId(
                BuiltInBindingProfiles.ID_NONE
            );
        }
    }
}
