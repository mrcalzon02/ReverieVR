package io.github.mrcalzon02.reverievr;

/** Converts ACTION_BATTERY_CHANGED level/scale to a bounded percentage. */
final class PhoneBatteryPercentage {
    private PhoneBatteryPercentage() { }

    static int fromLevelAndScale(int level, int scale) {
        if (level < 0 || scale <= 0 || level > scale) {
            return -1;
        }
        return (int) Math.round((level * 100.0) / scale);
    }
}
