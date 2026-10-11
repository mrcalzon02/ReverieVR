package io.github.mrcalzon02.reverievr;
import java.util.Locale;

/** Recognition of scrcpy-labelled UHID, not arbitrary keyboard/mouse sources. */
final class ScrcpyHidIdentity {
    static boolean matchesName(String name) {
        if (name == null) return false;
        String value = name.trim().toLowerCase(Locale.ROOT);
        return value.equals("scrcpy") || value.startsWith("scrcpy ")
            || value.startsWith("scrcpy-") || value.startsWith("scrcpy_");
    }
    private ScrcpyHidIdentity() {}
}
