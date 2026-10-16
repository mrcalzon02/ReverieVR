package io.github.mrcalzon02.reverievr;

/** Immutable, safe world-space DOS desk screen placement; never touches optics. */
final class DosScreenPlacement {
    static final float MIN_SCALE = 0.75f, MAX_SCALE = 1.20f;
    static final float MIN_DISTANCE = 2.20f, MAX_DISTANCE = 4.20f;
    static final float MIN_HEIGHT = -0.48f, MAX_HEIGHT = 0.48f;
    static final DosScreenPlacement DEFAULT = new DosScreenPlacement(1f, 3.20f, 0f);

    final float scale;
    final float distance;
    final float height;

    DosScreenPlacement(float scale, float distance, float height) {
        this.scale = bounded(scale, MIN_SCALE, MAX_SCALE, 1f);
        this.distance = bounded(distance, MIN_DISTANCE, MAX_DISTANCE, 3.20f);
        this.height = bounded(height, MIN_HEIGHT, MAX_HEIGHT, 0f);
    }

    DosScreenPlacement adjust(int action) {
        switch (action) {
            case 0: return new DosScreenPlacement(scale - .05f, distance, height);
            case 1: return new DosScreenPlacement(scale + .05f, distance, height);
            case 2: return new DosScreenPlacement(scale, distance - .15f, height);
            case 3: return new DosScreenPlacement(scale, distance + .15f, height);
            case 4: return new DosScreenPlacement(scale, distance, height + .08f);
            case 5: return new DosScreenPlacement(scale, distance, height - .08f);
            case 6: return DEFAULT;
            default: return this;
        }
    }

    boolean sameAs(DosScreenPlacement other) {
        return other != null && scale == other.scale
            && distance == other.distance && height == other.height;
    }

    private static float bounded(float value, float min, float max, float fallback) {
        if (!Float.isFinite(value)) return fallback;
        return Math.max(min, Math.min(max, value));
    }
}
