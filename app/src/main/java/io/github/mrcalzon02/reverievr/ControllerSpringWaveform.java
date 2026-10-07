package io.github.mrcalzon02.reverievr;

/** A tiny deterministic spring/boing cue, generated once without assets. */
final class ControllerSpringWaveform {
    static final int SAMPLE_RATE = 22050;
    static final int DURATION_MILLIS = 460;

    private ControllerSpringWaveform() {}

    static byte[] generatePcm16() {
        int samples = SAMPLE_RATE * DURATION_MILLIS / 1000;
        byte[] pcm = new byte[samples * 2];
        double phase = 0.0;
        for (int i = 0; i < samples; i++) {
            double t = i / (double) SAMPLE_RATE;
            double progress = i / (double) (samples - 1);
            double envelope = Math.min(1.0, t * 130.0)
                * Math.pow(1.0 - progress, 2.3);
            double pitch = 145.0 + 450.0 * Math.exp(-9.0 * t);
            phase += 2.0 * Math.PI * pitch / SAMPLE_RATE;
            double wobble = 0.55 * Math.sin(2.0 * Math.PI * 13.5 * t)
                * Math.exp(-5.0 * t);
            int sample = (int) Math.round(
                Math.sin(phase + wobble) * envelope * 0.30 * 32767.0
            );
            pcm[i * 2] = (byte) (sample & 0xff);
            pcm[i * 2 + 1] = (byte) ((sample >>> 8) & 0xff);
        }
        return pcm;
    }
}
