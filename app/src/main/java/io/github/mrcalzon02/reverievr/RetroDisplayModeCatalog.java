package io.github.mrcalzon02.reverievr;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

final class RetroDisplayModeCatalog {
    private static final List<RetroDisplayMode> KNOWN =
        Collections.unmodifiableList(
            Arrays.asList(
                new RetroDisplayMode(
                    320, 200, 4, 3,
                    "VGA 320x200 / Mode 13h",
                    false
                ),
                new RetroDisplayMode(
                    320, 240, 4, 3,
                    "320x240 square-pixel 4:3",
                    true
                ),
                new RetroDisplayMode(
                    640, 350, 4, 3,
                    "EGA 640x350",
                    false
                ),
                new RetroDisplayMode(
                    640, 400, 4, 3,
                    "VGA 640x400",
                    false
                ),
                new RetroDisplayMode(
                    640, 480, 4, 3,
                    "VGA 640x480",
                    true
                ),
                new RetroDisplayMode(
                    720, 400, 4, 3,
                    "DOS/VGA text 720x400",
                    false
                ),
                new RetroDisplayMode(
                    800, 600, 4, 3,
                    "SVGA 800x600",
                    true
                ),
                new RetroDisplayMode(
                    1024, 768, 4, 3,
                    "XGA 1024x768",
                    true
                ),
                new RetroDisplayMode(
                    1280, 1024, 5, 4,
                    "SXGA 1280x1024",
                    true
                ),
                new RetroDisplayMode(
                    160, 144, 10, 9,
                    "Game Boy 160x144",
                    true
                ),
                new RetroDisplayMode(
                    240, 160, 3, 2,
                    "Game Boy Advance 240x160",
                    true
                )
            )
        );

    private RetroDisplayModeCatalog() {
    }

    static List<RetroDisplayMode> knownModes() {
        return KNOWN;
    }

    static RetroDisplayMode resolve(
        int width,
        int height
    ) {
        for (RetroDisplayMode mode : KNOWN) {
            if (mode.width == width
                && mode.height == height) {
                return mode;
            }
        }

        return new RetroDisplayMode(
            width,
            height,
            width,
            height,
            width + "x" + height + " native aspect",
            true
        );
    }

    static int largestIntegerScale(
        int sourceWidth,
        int sourceHeight,
        int targetWidth,
        int targetHeight
    ) {
        if (sourceWidth <= 0
            || sourceHeight <= 0
            || targetWidth <= 0
            || targetHeight <= 0) {
            return 0;
        }

        return Math.min(
            targetWidth / sourceWidth,
            targetHeight / sourceHeight
        );
    }
}
