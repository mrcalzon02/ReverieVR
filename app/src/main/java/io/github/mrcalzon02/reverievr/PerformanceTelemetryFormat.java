package io.github.mrcalzon02.reverievr;

import java.util.Locale;

/** Observed Cardboard callback and CPU timing, never guessed GPU/display timings. */
final class PerformanceTelemetryFormat {
    private PerformanceTelemetryFormat() {}

    static String[] lines(int mode, int fps,
                          FramePerformanceTracker.Snapshot frame,
                          EyeRenderPerformanceTracker.Snapshot eyes,
                          PerformanceEnvironmentSnapshot environment) {
        String[] lines = new String[8];
        lines[0] = "APP CALLBACK FPS " + (fps < 0 ? "N/A" : fps)
            + "  MODE " + mode;
        if (frame == null || frame.sampleCount == 0) {
            lines[1] = "FRAME AVG/P95/P99: N/A";
            lines[2] = "FRAME MAX/JITTER: N/A";
            lines[3] = "GAPS >33/>50ms: N/A";
        } else {
            lines[1] = String.format(Locale.US,
                "FRAME AVG %.1f P95 %.1f P99 %.1f ms",
                frame.averageMillis, frame.p95Millis, frame.p99Millis);
            lines[2] = String.format(Locale.US,
                "MAX %.1f JITTER %.1f ms (%d frames)",
                frame.maximumMillis, frame.jitterMillis, frame.sampleCount);
            lines[3] = String.format(Locale.US,
                "GAPS >33ms %d  >50ms %d",
                frame.gapsOver33Millis, frame.gapsOver50Millis);
        }
        lines[4] = cpuLine("CPU LEFT ", eyes == null ? null : eyes.left);
        lines[5] = cpuLine("CPU RIGHT", eyes == null ? null : eyes.right);
        String temp = environment == null ||
            environment.batteryTemperatureTenthsC ==
                PerformanceEnvironmentSnapshot.BATTERY_TEMPERATURE_UNAVAILABLE
            ? "N/A" : String.format(Locale.US, "%.1fC",
                environment.batteryTemperatureTenthsC / 10.0);
        String thermal = environment == null ? "unavailable" :
            PerformanceEnvironmentSnapshot.thermalStatusLabel(
                environment.thermalStatus);
        lines[6] = "BAT " + temp + "  THERM " + thermal;
        lines[7] = "GPU / DISPLAY PRESENT: NOT MEASURED";
        return lines;
    }

    private static String cpuLine(String label,
                                  EyeRenderPerformanceTracker.EyeStats eye) {
        if (eye == null || eye.count == 0) return label + " P95/P99 N/A";
        return String.format(Locale.US, "%s P95 %.1f P99 %.1f ms",
            label, eye.p95Millis, eye.p99Millis);
    }

    static String datedReport(long epochMillis, String[] lines) {
        StringBuilder report = new StringBuilder(400);
        report.append("sampleUnixMillis=").append(epochMillis).append('\n');
        report.append("source=Cardboard callbacks and CPU GL eye submission, not actual GPU/compositor/presentation timing\n");
        for (String line : lines) report.append(line).append('\n');
        return report.toString();
    }
}
