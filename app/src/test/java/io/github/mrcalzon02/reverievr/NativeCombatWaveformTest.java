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

    @Test
    public void rapidGunfireUsesTwoReusableVoices() {
        assertEquals(2, NativeCombatWaveform.voiceCount(NativeCombatWaveform.FIRE));
        for (int cue = 1; cue < NativeCombatWaveform.COUNT; cue++)
            assertEquals(cue==NativeCombatWaveform.MACHINE?2:1,
                NativeCombatWaveform.voiceCount(cue));
        // The maximum fire-rate upgrade reduces 300ms to 160ms. A single
        // 180ms report would be cut short; two voices can alternate safely.
        assertTrue(NativeCombatWaveform.durationMillis(NativeCombatWaveform.FIRE) > 160);
        assertTrue(NativeCombatWaveform.durationMillis(NativeCombatWaveform.FIRE) <= 320);
    }

    @Test
    public void fiveWeaponReportsHaveUniquePcmAndBoundedSize() {
        java.util.HashSet<Integer> signatures=new java.util.HashSet<>();
        assertEquals(-1,NativeCombatWaveform.weaponCueForNativeEvent(0));
        assertEquals(-1,NativeCombatWaveform.weaponCueForNativeEvent(6));
        for(int id=1;id<=5;id++)
            assertEquals(NativeCombatWaveform.MACHINE+id-1,
                NativeCombatWaveform.weaponCueForNativeEvent(id));
        for (int id=NativeCombatWaveform.MACHINE;
             id<=NativeCombatWaveform.CANNON;id++) {
            byte[] pcm=NativeCombatWaveform.generatePcm16(id);
            assertTrue(pcm.length>5000 && pcm.length<25000);
            assertTrue(signatures.add(java.util.Arrays.hashCode(pcm)));
        }
        assertEquals(5,signatures.size());
        assertEquals(170,NativeCombatWaveform.durationMillis(
            NativeCombatWaveform.MACHINE));
        assertEquals(360,NativeCombatWaveform.durationMillis(
            NativeCombatWaveform.CANNON));
    }

    @Test
    public void tenGameEventEffectsHaveUniqueBoundedWaveforms() {
        java.util.HashSet<Integer> hashes = new java.util.HashSet<>();
        for(int id=6;id<=15;id++) {
            int cue=NativeCombatWaveform.cueForNativeEvent(id);
            assertEquals(NativeCombatWaveform.HULL+id-6,cue);
            byte[] sample=NativeCombatWaveform.generatePcm16(cue);
            assertTrue(sample.length>2000 && sample.length<30000);
            assertTrue(hashes.add(java.util.Arrays.hashCode(sample)));
            assertEquals(1,NativeCombatWaveform.voiceCount(cue));
        }
        assertEquals(10,hashes.size());
        assertEquals(-1,NativeCombatWaveform.cueForNativeEvent(0));
        assertEquals(-1,NativeCombatWaveform.cueForNativeEvent(20));
        assertEquals(-1,NativeCombatWaveform.weaponCueForNativeEvent(6));
    }

    @Test
    public void fourProgressionEffectsHaveBoundedDistinctSoundAndNativeRouting() {
        java.util.HashSet<Integer> hashes = new java.util.HashSet<>();
        for (int nativeId=16;nativeId<=19;nativeId++) {
            int cue=NativeCombatWaveform.cueForNativeEvent(nativeId);
            assertEquals(NativeCombatWaveform.PURCHASE+nativeId-16,cue);
            byte[] pcm=NativeCombatWaveform.generatePcm16(cue);
            assertTrue(pcm.length>=6000 && pcm.length<40000);
            assertTrue(hashes.add(java.util.Arrays.hashCode(pcm)));
            assertEquals(1,NativeCombatWaveform.voiceCount(cue));
        }
        assertEquals(4,hashes.size());
        assertEquals(-1,NativeCombatWaveform.cueForNativeEvent(20));
        assertEquals(-1,NativeCombatWaveform.weaponCueForNativeEvent(16));
    }

    @Test(expected = IllegalArgumentException.class)
    public void invalidVoiceCueRejected() {
        NativeCombatWaveform.voiceCount(NativeCombatWaveform.COUNT);
    }

    @Test(expected = IllegalArgumentException.class)
    public void invalidCueRejected() {
        NativeCombatWaveform.generatePcm16(NativeCombatWaveform.COUNT);
    }
}
