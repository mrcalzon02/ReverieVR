package io.github.mrcalzon02.reverievr;

enum HomeEnvironment {
    WHITE_ROOM(
        "white_room",
        "Birch Panel Living Room"
    ),
    FOREST_GLADE(
        "forest_glade",
        "Pastoral Forest Glade"
    ),
    DUNE_BEACH(
        "dune_beach",
        "Windswept Dune Beach"
    );

    final String preferenceValue;
    final String displayName;

    HomeEnvironment(
        String preferenceValue,
        String displayName
    ) {
        this.preferenceValue = preferenceValue;
        this.displayName = displayName;
    }

    static HomeEnvironment fromPreference(
        String value
    ) {
        if (value != null) {
            for (HomeEnvironment environment : values()) {
                if (environment.preferenceValue.equals(value)) {
                    return environment;
                }
            }
        }
        return WHITE_ROOM;
    }
}
