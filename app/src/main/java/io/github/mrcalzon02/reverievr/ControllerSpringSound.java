package io.github.mrcalzon02.reverievr;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

/** One reusable static PCM track; called and released on Activity/UI thread. */
final class ControllerSpringSound implements AutoCloseable {
    private AudioTrack track;

    ControllerSpringSound() {
        AudioTrack candidate = null;
        try {
            byte[] pcm = ControllerSpringWaveform.generatePcm16();
            candidate = new AudioTrack(
                AudioManager.STREAM_MUSIC,
                ControllerSpringWaveform.SAMPLE_RATE,
                AudioFormat.CHANNEL_OUT_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                pcm.length,
                AudioTrack.MODE_STATIC
            );
            if (candidate.getState() != AudioTrack.STATE_INITIALIZED
                || candidate.write(pcm, 0, pcm.length) != pcm.length) {
                candidate.release();
                return;
            }
            candidate.setStereoVolume(0.32f, 0.32f);
            track = candidate;
        } catch (RuntimeException failure) {
            if (candidate != null) {
                candidate.release();
            }
        }
    }

    void play() {
        if (track == null) return;
        try {
            if (track.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
                track.stop();
            }
            track.reloadStaticData();
            track.setPlaybackHeadPosition(0);
            track.play();
        } catch (RuntimeException ignored) {
            // An unavailable audio route must not interrupt tracking.
        }
    }

    void stop() {
        if (track == null) return;
        try {
            if (track.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
                track.stop();
            }
        } catch (RuntimeException ignored) {
            // The Activity may already be stopping its audio route.
        }
    }

    @Override
    public void close() {
        if (track != null) {
            stop();
            track.release();
            track = null;
        }
    }
}
