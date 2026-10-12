package io.github.mrcalzon02.reverievr;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public final class NativeCombatWaveformTest {
    @Test
    public void deterministicNonSilentFadingCues() {
        for (int cue = 0; cue < NativeCombatWaveform.COUNT; cue++) {
            byte[] sound = NativeCombatWaveform.generatePcm16(cue);
            assertEquals(NativeCombatWaveform.SAMPLE_RATE *
                NativeCombatWaveform.durationMillis(cue) / 1000 * 2,
                sound.length);
            assertArrayEquals(sound, NativeCombatWaveform.generatePcm16(cue));
            int peak = 0, tail = 0;
            for (int i = 0; i < sound.length / 2; i++) {
                int sample = (short) (((sound[2 * i + 1] & 255) << 8)
                    | (sound[2 * i] & 255));
                int abs = Math.abs(sample);
                peak = Math.max(peak, abs);
                if (i >= sound.length / 2 - 80) tail = Math.max(tail, abs);
            }
            assertTrue(peak > 1000);
            assertTrue(tail < peak / 12);
        }
    }

    @Test
    public void supportCuesHaveDistinctAudioAndLengths() {
        byte[] gun = NativeCombatWaveform.generatePcm16(NativeCombatWaveform.FIRE);
        byte[] artillery = NativeCombatWaveform.generatePcm16(NativeCombatWaveform.ARTILLERY);
        byte[] air = NativeCombatWaveform.generatePcm16(NativeCombatWaveform.AIRSTRIKE);
        assertTrue(artillery.length > gun.length);
        assertTrue(air.length > artillery.length);
        assertTrue(!java.util.Arrays.equals(artillery, air));
    }

    @Test(expected = IllegalArgumentException.class)
    public void invalidCueRejected() {
        NativeCombatWaveform.generatePcm16(7);
    }
}
