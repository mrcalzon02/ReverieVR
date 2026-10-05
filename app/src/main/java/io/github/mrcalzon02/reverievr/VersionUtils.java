package io.github.mrcalzon02.reverievr;

final class VersionUtils {
    private VersionUtils() {
    }

    static boolean isNewer(String candidate, String current) {
        ParsedVersion candidateVersion = ParsedVersion.parse(candidate);
        ParsedVersion currentVersion = ParsedVersion.parse(current);

        for (int index = 0; index < 3; index++) {
            if (candidateVersion.parts[index] != currentVersion.parts[index]) {
                return candidateVersion.parts[index] > currentVersion.parts[index];
            }
        }

        if (candidateVersion.preRelease == currentVersion.preRelease) {
            return false;
        }

        // A stable release is newer than the matching prerelease/dev build.
        return currentVersion.preRelease && !candidateVersion.preRelease;
    }

    private static final class ParsedVersion {
        final int[] parts;
        final boolean preRelease;

        ParsedVersion(int[] parts, boolean preRelease) {
            this.parts = parts;
            this.preRelease = preRelease;
        }

        static ParsedVersion parse(String raw) {
            String value = raw == null ? "" : raw.trim();
            if (value.startsWith("v") || value.startsWith("V")) {
                value = value.substring(1);
            }

            int plusIndex = value.indexOf('+');
            if (plusIndex >= 0) {
                value = value.substring(0, plusIndex);
            }

            boolean preRelease = value.contains("-");
            String core = value.split("-", 2)[0];
            String[] split = core.split("\\.");

            int[] parts = new int[] {0, 0, 0};
            for (int index = 0; index < parts.length && index < split.length; index++) {
                try {
                    parts[index] = Integer.parseInt(split[index].replaceAll("[^0-9]", ""));
                } catch (NumberFormatException exception) {
                    parts[index] = 0;
                }
            }

            return new ParsedVersion(parts, preRelease);
        }
    }
}
