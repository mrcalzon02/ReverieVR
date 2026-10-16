package io.github.mrcalzon02.reverievr;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;

/**
 * Original offline low-volume environmental beds for VR Home.
 * A short deterministic PCM16 loop is synthesized ONCE per scene change.
 * AudioTrack loops it in hardware; no renderer callbacks, mixer thread,
 * bundled media files or external network are required.
 *
 * Licensed CC0 field-recording candidates are documented separately.
 * Those external recordings are NOT included by this class.
 */
final class HomeAmbientAudio implements AutoCloseable {
    private static final int RATE=22050;
    private static final int LOOP_SECONDS=4;
    private static final int SAMPLES=RATE*LOOP_SECONDS;
    private static final float OUTPUT_GAIN=.16f;
    private AudioTrack track;
    private HomeEnvironment environment;
    private boolean active;
    private boolean lifecycleActive;
    private boolean closed;

    void setEnvironment(HomeEnvironment next,boolean homeActive) {
        if(closed)return;
        HomeEnvironment safe=next==null?HomeEnvironment.WHITE_ROOM:next;
        if(environment != safe) {
            stopAndRelease();
            environment=safe;
        }
        active=homeActive;
        sync();
    }

    void resumeForLifecycle() {
        if(closed)return;
        lifecycleActive=true;
        sync();
    }

    void pauseForLifecycle() {
        lifecycleActive=false;
        sync();
    }

    private void sync() {
        if(closed || !active || !lifecycleActive) {
            if(track != null) {
                try { track.pause(); } catch(IllegalStateException failure) {
                    ReverieLog.error("HOME_AMBIENT","Cannot pause environmental audio",failure);
                    stopAndRelease();
                }
            }
            return;
        }
        if(track==null && !createLoop())return;
        try {
            if(track.getPlayState()!=AudioTrack.PLAYSTATE_PLAYING) track.play();
        } catch(IllegalStateException failure) {
            ReverieLog.error("HOME_AMBIENT","Cannot play environmental audio",failure);
            stopAndRelease();
        }
    }

    private boolean createLoop() {
        AudioTrack result=null;
        try {
            short[] pcm=generate(environment);
            AudioFormat format=new AudioFormat.Builder()
                .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                .setSampleRate(RATE)
                .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
                .build();
            AudioAttributes attributes=new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
            result=new AudioTrack.Builder()
                .setAudioAttributes(attributes)
                .setAudioFormat(format)
                .setTransferMode(AudioTrack.MODE_STATIC)
                .setBufferSizeInBytes(pcm.length*2)
                .build();
            if(result.getState()!=AudioTrack.STATE_INITIALIZED
                || result.write(pcm,0,pcm.length)!=pcm.length
                || result.setLoopPoints(0,pcm.length,-1)!=AudioTrack.SUCCESS) {
                throw new IllegalStateException("Environmental audio buffer rejected");
            }
            result.setVolume(OUTPUT_GAIN);
            track=result;
            return true;
        } catch(RuntimeException failure) {
            ReverieLog.error("HOME_AMBIENT","Could not initialize environmental bed",failure);
            if(result!=null)result.release();
            return false;
        }
    }

    /** No per-frame audio generation; deterministic seed, fixed buffer budget. */
    static short[] generate(HomeEnvironment environment) {
        HomeEnvironment safe=environment==null?HomeEnvironment.WHITE_ROOM:environment;
        short[] output=new short[SAMPLES];
        int seed=0x6c2a40d3;
        double smooth=0.0, smoother=0.0;
        for(int i=0;i<SAMPLES;i++) {
            seed ^= seed<<13; seed ^= seed>>>17; seed ^= seed<<5;
            double white=(seed & 0xffff)/32768.0-1.0;
            smooth=smooth*.979+white*.021;
            smoother=smoother*.999+white*.001;
            double t=(double)i/RATE;
            double sound;
            switch(safe) {
                case FOREST_GLADE:
                    // Leaves, distant rustle and brief separated songbird calls.
                    sound=.63*smooth+.18*white+.42*smoother;
                    sound+=chirp(t,.7,.18,1710.0,380.0)
                           +chirp(t,1.4,.14,2220.0,-360.0)
                           +chirp(t,2.45,.20,1470.0,520.0)
                           +chirp(t,3.21,.15,2040.0,430.0);
                    break;
                case DUNE_BEACH:
                    // Pulsed broadband surf with light gusts and gull calls.
                    double wave=.30+.70*(.5+.5*Math.sin(2*Math.PI*t/3.8));
                    sound=wave*(.72*smooth+.33*white)+.32*smoother;
                    sound+=.50*chirp(t,1.15,.50,650.0,-240.0)
                           +.48*chirp(t,2.91,.43,820.0,-340.0);
                    break;
                case WHITE_ROOM:
                default:
                    // Quiet HVAC and low-frequency room hum.
                    sound=.27*smooth+.23*smoother
                           +.075*Math.sin(2*Math.PI*58.0*t);
                    break;
            }
            // Smooth exact-loop join; no pops when AudioTrack wraps.
            double edge=Math.min(1.0,Math.min(i,SAMPLES-1-i)/(RATE*.06));
            double bounded=Math.max(-1.0,Math.min(1.0,sound*.55*edge));
            output[i]=(short)Math.round(bounded*32767.0);
        }
        return output;
    }

    private static double chirp(double t,double start,double duration,
                                double hz,double slide) {
        double p=(t-start)/duration;
        if(p<0.0 || p>=1.0)return 0.0;
        double envelope=Math.sin(Math.PI*p);
        return .45*envelope*envelope*
            Math.sin(2.0*Math.PI*(hz*(t-start)
                +.5*slide*(t-start)*(t-start)/duration));
    }

    private void stopAndRelease() {
        AudioTrack previous=track;
        track=null;
        if(previous!=null) {
            try { previous.pause(); } catch(IllegalStateException ignored) {
                // Release is unconditional even for an already-stopped track.
            }
            previous.release();
        }
    }

    @Override public void close() {
        closed=true;
        stopAndRelease();
    }
}
