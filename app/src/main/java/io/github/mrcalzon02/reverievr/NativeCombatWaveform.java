package io.github.mrcalzon02.reverievr;

/** Deterministic tiny PCM16 gameplay cues, generated once without audio assets. */
final class NativeCombatWaveform {
    static final int SAMPLE_RATE = 22050;
    static final int FIRE = 0, RELOAD = 1, READY = 2;
    static final int ARTILLERY = 3, AIRSTRIKE = 4;
    // Byte-sized native ordered sound ids 1–5 map to this private bank
    // without changing the generic shell feedback/voice cue indices.
    static final int MACHINE = 5, HEAVY = 6, FLAK = 7;
    static final int MISSILE = 8, CANNON = 9, COUNT = 10;

    private NativeCombatWaveform() {}

    // The native ABI uses ordered cue IDs 1..5 (not the PCM bank's
    // integer constants 5..9). This mapping is pure JVM-testable.
    static int weaponCueForNativeEvent(int id) {
        return id>=1 && id<=5 ? MACHINE+id-1 : -1;
    }

    // Two overlapping reports are enough for the upgraded 160ms gun cycle;
    // all other cues retain one preloaded static voice.
    static int voiceCount(int cue) {
        if (cue < 0 || cue >= COUNT) throw new IllegalArgumentException("Unknown cue");
        return cue == FIRE || cue == MACHINE ? 2 : 1;
    }

    static int durationMillis(int cue) {
        if (cue == FIRE) return 180;
        if (cue == RELOAD) return 230;
        if (cue == READY) return 100;
        if (cue == ARTILLERY) return 520;
        if (cue == AIRSTRIKE) return 650;
        if (cue == MACHINE) return 170;
        if (cue == HEAVY) return 240;
        if (cue == FLAK) return 270;
        if (cue == MISSILE) return 390;
        if (cue == CANNON) return 360;
        throw new IllegalArgumentException("Unknown cue");
    }

    static byte[] generatePcm16(int cue) {
        int count = SAMPLE_RATE * durationMillis(cue) / 1000;
        byte[] data = new byte[count * 2];
        int noiseState = 0x64ad017c ^ ((cue + 1) * 0x61c88647);
        double phase = 0.0;
        for (int i = 0; i < count; i++) {
            double t = i / (double) SAMPLE_RATE;
            double p = i / (double) (count - 1);
            noiseState ^= noiseState << 13;
            noiseState ^= noiseState >>> 17;
            noiseState ^= noiseState << 5;
            double noise = ((noiseState & 65535) / 32767.5) - 1.0;
            double envelope = Math.min(1.0, t * 900.0)
                * Math.pow(1.0 - p, 2.3);
            double signal;
            if (cue == FIRE) {
                phase += 2.0 * Math.PI * (90.0 +
                    220.0 * Math.exp(-22.0 * t)) / SAMPLE_RATE;
                signal = envelope * (0.48 * noise * Math.exp(-12.0 * t)
                    + 0.35 * Math.sin(phase) * Math.exp(-10.0 * t));
            } else if (cue >= MACHINE && cue <= CANNON) {
                // Distinct original synthesizer recipes, not a second
                // short gunshot played at five arbitrary playback speeds.
                // One-shot envelopes carry weight without sample packs.
                if (cue == MACHINE) {
                    phase += 2.0 * Math.PI * (122.0 +
                        245.0 * Math.exp(-27.0 * t)) / SAMPLE_RATE;
                    signal = envelope * (0.53 * noise * Math.exp(-17.0 * t)
                        + .32 * Math.sin(phase) * Math.exp(-12.0 * t));
                } else if (cue == HEAVY) {
                    phase += 2.0 * Math.PI * (57.0 +
                        172.0 * Math.exp(-16.0 * t)) / SAMPLE_RATE;
                    signal = envelope * (.43 * noise * Math.exp(-9.0 * t)
                        + .47 * Math.sin(phase) * Math.exp(-9.5 * t));
                } else if (cue == FLAK) {
                    phase += 2.0 * Math.PI * (86.0 +
                        300.0 * Math.exp(-30.0 * t)) / SAMPLE_RATE;
                    double snap = Math.exp(-52.0 * t);
                    signal = envelope * (.69 * noise * snap +
                        .28 * Math.sin(phase) * Math.exp(-14.0 * t));
                } else if (cue == MISSILE) {
                    phase += 2.0 * Math.PI * (470.0 -
                        310.0 * p) / SAMPLE_RATE;
                    double exhaust = Math.exp(-5.0 * t);
                    signal = envelope * exhaust *
                        (.39 * noise + .45 * Math.sin(phase));
                } else {
                    phase += 2.0 * Math.PI * (48.0 +
                        125.0 * Math.exp(-11.0 * t)) / SAMPLE_RATE;
                    signal = envelope * (.40 * noise * Math.exp(-8.0 * t)
                        + .51 * Math.sin(phase) * Math.exp(-7.0 * t));
                }
            } else if (cue == RELOAD) {
                double clack = Math.exp(-58.0 * t) + (t >= 0.115 ?
                    .85 * Math.exp(-70.0 * (t - 0.115)) : 0.0);
                signal = envelope * clack * (0.32 * noise
                    + 0.32 * Math.sin(2.0 * Math.PI * 810.0 * t));
            } else if (cue == ARTILLERY) {
                // Three staggered low-frequency shells, distinct from the MG.
                double local = t % 0.17;
                double impact = Math.exp(-30.0 * local);
                signal = envelope * impact * (0.46 * noise
                    + 0.46 * Math.sin(2.0 * Math.PI * 72.0 * local));
            } else if (cue == AIRSTRIKE) {
                // Descending engine whistle followed by an overhead blast.
                phase += 2.0 * Math.PI * (210.0 - 140.0 * p) / SAMPLE_RATE;
                double blast = t >= 0.30 ? Math.exp(-16.0 * (t - 0.30)) : 0.0;
                signal = envelope * (0.42 * Math.sin(phase) * (1.0 - p)
                    + blast * (0.62 * noise +
                        0.30 * Math.sin(2.0 * Math.PI * 65.0 * (t - 0.30))));
            } else {
                signal = envelope * Math.exp(-30.0 * t) *
                    (0.24 * noise + 0.45 * Math.sin(2.0 * Math.PI * 1060.0 * t));
            }
            int sample = (int) Math.round(Math.max(-1.0,
                Math.min(1.0, signal)) * 32767.0);
            data[2 * i] = (byte) sample;
            data[2 * i + 1] = (byte) (sample >>> 8);
        }
        return data;
    }
}
