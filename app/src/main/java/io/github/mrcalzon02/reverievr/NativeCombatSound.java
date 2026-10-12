package io.github.mrcalzon02.reverievr;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

/** Reusable fixed-size shell sound bank; never synthesizes PCM per frame/eye. */
final class NativeCombatSound implements AutoCloseable {
    private final AudioTrack[] tracks = new AudioTrack[NativeCombatWaveform.COUNT];

    NativeCombatSound() {
        for (int cue = 0; cue < tracks.length; cue++) {
            AudioTrack candidate = null;
            try {
                byte[] pcm = NativeCombatWaveform.generatePcm16(cue);
                candidate = new AudioTrack(AudioManager.STREAM_MUSIC,
                    NativeCombatWaveform.SAMPLE_RATE, AudioFormat.CHANNEL_OUT_MONO,
                    AudioFormat.ENCODING_PCM_16BIT, pcm.length, AudioTrack.MODE_STATIC);
                if (candidate.getState() != AudioTrack.STATE_INITIALIZED
                    || candidate.write(pcm, 0, pcm.length) != pcm.length) {
                    candidate.release();
                    continue;
                }
                candidate.setStereoVolume(.34f, .34f);
                tracks[cue] = candidate;
            } catch (RuntimeException unavailable) {
                if (candidate != null) candidate.release();
            }
        }
    }

    void play(int cue) {
        if (cue < 0 || cue >= tracks.length || tracks[cue] == null) return;
        try {
            AudioTrack track = tracks[cue];
            if (track.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) track.stop();
            track.reloadStaticData();
            track.setPlaybackHeadPosition(0);
            track.play();
        } catch (RuntimeException unavailable) {
            // Loss of audio route must not interrupt the running native game.
        }
    }

    void stop() {
        for (AudioTrack track : tracks) {
            if (track == null) continue;
            try {
                if (track.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) track.stop();
            } catch (RuntimeException unavailable) {
                // Activity lifecycle may already have torn down the route.
            }
        }
    }

    @Override
    public void close() {
        stop();
        for (int i = 0; i < tracks.length; i++) {
            if (tracks[i] != null) {
                tracks[i].release();
                tracks[i] = null;
            }
        }
    }
}
