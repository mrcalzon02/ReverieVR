package io.github.mrcalzon02.reverievr;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

/** Reusable fixed-size shell sound bank; never synthesizes PCM per frame/eye. */
final class NativeCombatSound implements AutoCloseable {
    // The fastest upgraded gun cycles in 160ms, faster than its 180ms sound.
    // Two preloaded voices let consecutive rounds overlap without cutting
    // off the previous report or allocating a new AudioTrack on the frame.
    private final AudioTrack[][] tracks = new AudioTrack[NativeCombatWaveform.COUNT][];
    private final int[] nextVoice = new int[NativeCombatWaveform.COUNT];

    NativeCombatSound() {
        for (int cue = 0; cue < tracks.length; cue++) {
            byte[] pcm = NativeCombatWaveform.generatePcm16(cue);
            tracks[cue] = new AudioTrack[NativeCombatWaveform.voiceCount(cue)];
            for (int voice = 0; voice < tracks[cue].length; voice++) {
                AudioTrack candidate = null;
                try {
                    candidate = new AudioTrack(AudioManager.STREAM_MUSIC,
                        NativeCombatWaveform.SAMPLE_RATE, AudioFormat.CHANNEL_OUT_MONO,
                        AudioFormat.ENCODING_PCM_16BIT, pcm.length, AudioTrack.MODE_STATIC);
                    if (candidate.getState() != AudioTrack.STATE_INITIALIZED
                        || candidate.write(pcm, 0, pcm.length) != pcm.length) {
                        candidate.release();
                        continue;
                    }
                    candidate.setStereoVolume(.34f, .34f);
                    tracks[cue][voice] = candidate;
                } catch (RuntimeException unavailable) {
                    if (candidate != null) candidate.release();
                }
            }
        }
    }

    void play(int cue) {
        if (cue < 0 || cue >= tracks.length) return;
        AudioTrack[] voices = tracks[cue];
        int start = nextVoice[cue];
        for (int offset = 0; offset < voices.length; offset++) {
            int slot = (start + offset) % voices.length;
            AudioTrack track = voices[slot];
            if (track == null) continue;
            nextVoice[cue] = (slot + 1) % voices.length;
            try {
                if (track.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) track.stop();
                track.reloadStaticData();
                track.setPlaybackHeadPosition(0);
                track.play();
            } catch (RuntimeException unavailable) {
                // Loss of audio route must not interrupt the running native game.
            }
            return;
        }
    }

    void stop() {
        for (AudioTrack[] voices : tracks) {
            if (voices == null) continue;
            for (AudioTrack track : voices) {
                if (track == null) continue;
                try {
                    if (track.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) track.stop();
                } catch (RuntimeException unavailable) {
                    // Activity lifecycle may already have torn down the route.
                }
            }
        }
    }

    @Override
    public void close() {
        stop();
        for (AudioTrack[] voices : tracks) {
            if (voices == null) continue;
            for (int i = 0; i < voices.length; i++) {
                if (voices[i] != null) {
                    voices[i].release();
                    voices[i] = null;
                }
            }
        }
    }
}
