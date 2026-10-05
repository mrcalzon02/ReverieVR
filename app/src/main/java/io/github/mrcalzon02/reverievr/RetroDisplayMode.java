package io.github.mrcalzon02.reverievr;

final class RetroDisplayMode {
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
}
