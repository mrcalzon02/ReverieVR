package io.github.mrcalzon02.reverievr;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;

final class DosContentSupport {
    private static final Set<String> EXTENSIONS =
        Collections.unmodifiableSet(
            new HashSet<>(
                Arrays.asList(
                    "zip",
                    "dosz",
                    "exe",
                    "com",
                    "bat",
                    "iso",
                    "chd",
                    "cue",
                    "ins",
                    "img",
                    "ima",
                    "vhd",
                    "jrc",
                    "m3u",
                    "m3u8",
                    "conf"
                )
            )
        );

    private DosContentSupport() {
    }

    static boolean isSupportedFileName(String fileName) {
        return !extensionOf(fileName).isEmpty();
    }

    static String extensionOf(String fileName) {
        if (fileName == null) {
            return "";
        }

        String trimmed = fileName.trim();
        int dot = trimmed.lastIndexOf('.');
        if (dot < 0 || dot >= trimmed.length() - 1) {
            return "";
        }

        String extension =
            trimmed.substring(dot + 1).toLowerCase(Locale.US);
        return EXTENSIONS.contains(extension)
            ? extension
            : "";
    }

    static String supportedExtensionsSummary() {
        return "ZIP/DOSZ, EXE/COM/BAT, ISO/CHD/CUE, "
            + "IMG/IMA/VHD/JRC, M3U/M3U8, CONF";
    }
}
