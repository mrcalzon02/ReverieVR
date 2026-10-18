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

    private long retryAfterMs;
    private boolean closed;

    NativeCombatSound() {
        for(int cue=0;cue<tracks.length;cue++)
            tracks[cue]=new AudioTrack[NativeCombatWaveform.voiceCount(cue)];
        prepareMissingVoices();
    }

    /** Android can reject static voices before its audio route is ready.
     * Recreate only missing voices at lifecycle resume and after a bounded
     * three-second playback retry; never initialize on the GL render thread. */
    void resumeForLifecycle() { prepareMissingVoices(); }

    private void prepareMissingVoices() {
        if(closed)return;
        for(int cue=0;cue<tracks.length;cue++) {
            boolean needed=false;
            for(AudioTrack voice:tracks[cue])if(voice==null)needed=true;
            if(!needed)continue;
            final byte[] pcm=NativeCombatWaveform.generatePcm16(cue);
            for(int i=0;i<tracks[cue].length;i++) {
                if(tracks[cue][i]!=null)continue;
                AudioTrack candidate=null;
                try {
                    candidate=new AudioTrack(AudioManager.STREAM_MUSIC,
                        NativeCombatWaveform.SAMPLE_RATE,
                        AudioFormat.CHANNEL_OUT_MONO,
                        AudioFormat.ENCODING_PCM_16BIT,
                        pcm.length,AudioTrack.MODE_STATIC);
                    if(candidate.getState()!=AudioTrack.STATE_INITIALIZED ||
                       candidate.write(pcm,0,pcm.length)!=pcm.length) {
                        candidate.release();
                        ReverieLog.incident("NATIVE_AUDIO",
                            "Could not preload native cue " + cue);
                        continue;
                    }
                    candidate.setStereoVolume(.72f,.72f);
                    tracks[cue][i]=candidate;
                } catch(RuntimeException failure) {
                    if(candidate!=null)candidate.release();
                    ReverieLog.error("NATIVE_AUDIO",
                        "Static audio voice creation failed for cue " + cue,failure);
                }
            }
        }
    }

    // Native cues are numbered 1..5; this mapping stays in the shell's
    // playback layer, not in a game-specific Android renderer.
    void playNativeCue(int cueId) {
        final int cue=NativeCombatWaveform.cueForNativeEvent(cueId);
        if(cue>=0)play(cue);
    }

    void play(int cue) {
        if (cue < 0 || cue >= tracks.length) return;
        if(closed)return;
        AudioTrack[] voices = tracks[cue];
        boolean missing=true;
        for(AudioTrack voice:voices)if(voice!=null)missing=false;
        if(missing && android.os.SystemClock.uptimeMillis()>=retryAfterMs) {
            retryAfterMs=android.os.SystemClock.uptimeMillis()+3000L;
            prepareMissingVoices();
        }
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
                // Release the failed static voice; the next bounded retry
                // recreates it rather than leaving combat silent forever.
                ReverieLog.error("NATIVE_AUDIO",
                    "Audio playback failed for native cue "+cue,unavailable);
                try {track.release();} catch(RuntimeException ignored){}
                voices[slot]=null;
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
        closed=true;
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
