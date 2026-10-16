package io.github.mrcalzon02.reverievr;

final class RetroDisplayMode {
    // Intentional black safety margin around a complete DOS screen. Only
    // fullscreen DOS uses this: native games/media retain their own scale.
    static final float FULLSCREEN_OPTICAL_SCALE = 0.70f;

    final int width;
    final int height;
    final int displayAspectNumerator;
    final int displayAspectDenominator;
    final String label;
    final boolean squarePixels;

    RetroDisplayMode(
        int width,
        int height,
        int displayAspectNumerator,
        int displayAspectDenominator,
        String label,
        boolean squarePixels
    ) {
        if (width <= 0 || height <= 0) {
            throw new IllegalArgumentException(
                "Display dimensions must be positive."
            );
        }
        if (displayAspectNumerator <= 0
            || displayAspectDenominator <= 0) {
            throw new IllegalArgumentException(
                "Display aspect must be positive."
            );
        }

        this.width = width;
        this.height = height;
        this.displayAspectNumerator =
            displayAspectNumerator;
        this.displayAspectDenominator =
            displayAspectDenominator;
        this.label = label == null ? "" : label;
        this.squarePixels = squarePixels;
    }

    float sourceAspect() {
        return (float) width / (float) height;
    }

    float displayAspect() {
        return (float) displayAspectNumerator
            / (float) displayAspectDenominator;
    }

    float pixelWidthToHeightRatio() {
        return displayAspect() / sourceAspect();
    }

    float virtualScreenWidth(float screenHeight) {
        return screenHeight * displayAspect();
    }

    // Aspect-fit a guest image into an existing pixel viewport. Call both
    // methods with the SAME original width/height, before centering the
    // result. No allocations in the two-eye rendering path.
    static int fittedWidth(int maxWidth, int maxHeight, float aspect) {
        if (maxWidth <= 0 || maxHeight <= 0
            || !Float.isFinite(aspect) || aspect <= 0f) return 0;
        return Math.min(maxWidth, Math.max(1, Math.round(maxHeight * aspect)));
    }

    static int fittedHeight(int maxWidth, int maxHeight, float aspect) {
        if (maxWidth <= 0 || maxHeight <= 0
            || !Float.isFinite(aspect) || aspect <= 0f) return 0;
        return Math.min(maxHeight, Math.max(1, Math.round(maxWidth / aspect)));
    }
}
