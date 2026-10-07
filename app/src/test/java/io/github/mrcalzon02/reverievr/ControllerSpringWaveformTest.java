package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class ControllerSpringWaveformTest {
    @Test
    public void deterministicShortSpringCueFadesToSilence() {
        byte[] pcm = ControllerSpringWaveform.generatePcm16();
        assertEquals(
            ControllerSpringWaveform.SAMPLE_RATE
                * ControllerSpringWaveform.DURATION_MILLIS / 1000 * 2,
            pcm.length
        );
        assertArrayEquals(pcm, ControllerSpringWaveform.generatePcm16());
        int peak = 0, tail = 0;
        for (int i = 0; i < pcm.length / 2; i++) {
            int sample = (short) ((pcm[i * 2 + 1] << 8)
                | (pcm[i * 2] & 0xff));
            peak = Math.max(peak, Math.abs(sample));
            if (i > pcm.length / 2 - 100) {
                tail = Math.max(tail, Math.abs(sample));
            }
        }
        assertTrue(peak > 2000);
        assertTrue(tail < peak / 20);
    }
}
