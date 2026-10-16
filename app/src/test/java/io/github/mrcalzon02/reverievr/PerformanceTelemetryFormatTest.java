package io.github.mrcalzon02.reverievr;
import static org.junit.Assert.*;
import org.junit.Test;

public final class PerformanceTelemetryFormatTest {
    @Test public void missingMetricsAreHonest() {
        String[] lines = PerformanceTelemetryFormat.lines(5, -1,
            new FramePerformanceTracker().snapshot(),
            new EyeRenderPerformanceTracker().snapshot(),
            PerformanceEnvironmentSnapshot.unavailable());
        assertEquals(8, lines.length);
        assertTrue(lines[0].contains("FPS N/A"));
        assertTrue(lines[1].contains("N/A"));
        assertTrue(lines[4].contains("N/A"));
        assertTrue(lines[7].contains("NOT MEASURED"));
    }

    @Test public void measuredFieldsAndDateSurviveReport() {
        FramePerformanceTracker frames = new FramePerformanceTracker();
        frames.recordFrame(1_000_000_000L);
        frames.recordFrame(1_017_000_000L);
        EyeRenderPerformanceTracker eyes = new EyeRenderPerformanceTracker();
        eyes.recordEye(5, 0, 2_000_000L);
        eyes.recordEye(5, 1, 4_000_000L);
        String[] lines = PerformanceTelemetryFormat.lines(5, 58,
            frames.snapshot(), eyes.snapshot(),
            new PerformanceEnvironmentSnapshot(405, 2));
        assertTrue(lines[1].contains("P99"));
        assertTrue(lines[2].contains("JITTER"));
        assertTrue(lines[3].contains("GAPS"));
        assertTrue(lines[4].contains("2.0"));
        assertTrue(lines[5].contains("4.0"));
        assertTrue(lines[6].contains("40.5C"));
        assertTrue(lines[6].contains("moderate"));
        String report = PerformanceTelemetryFormat.datedReport(1234L, lines);
        assertTrue(report.contains("sampleUnixMillis=1234"));
        assertTrue(report.contains("not actual GPU/compositor/presentation"));
    }
}
