package io.github.mrcalzon02.reverievr;

enum VideoProjection {
    FLAT_CINEMA,
    MONO_EQUIRECTANGULAR_360;

    static VideoProjection fromPreference(String value) {
        if (value == null || value.trim().isEmpty()) {
            return FLAT_CINEMA;
        }

        try {
            return valueOf(value);
        } catch (IllegalArgumentException exception) {
            return FLAT_CINEMA;
        }
    }
}
