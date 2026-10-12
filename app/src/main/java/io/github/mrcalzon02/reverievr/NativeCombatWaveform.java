package io.github.mrcalzon02.reverievr;

/** Deterministic tiny PCM16 gameplay cues, generated once without audio assets. */
final class NativeCombatWaveform {
    static final int SAMPLE_RATE = 22050;
    static final int FIRE = 0, RELOAD = 1, READY = 2, COUNT = 3;

    private NativeCombatWaveform() {}

    static int durationMillis(int cue) {
        if (cue == FIRE) return 180;
        if (cue == RELOAD) return 230;
        if (cue == READY) return 100;
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
            } else if (cue == RELOAD) {
                double clack = Math.exp(-58.0 * t) + (t >= 0.115 ?
                    .85 * Math.exp(-70.0 * (t - 0.115)) : 0.0);
                signal = envelope * clack * (0.32 * noise
                    + 0.32 * Math.sin(2.0 * Math.PI * 810.0 * t));
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
