package io.github.mrcalzon02.reverievr.game.testchamber;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import android.media.SoundPool;
import android.util.Log;
import java.io.IOException;

/** Six actual, packaged, bounded 22.05kHz PCM effects; no menu-only clicks. */
final class TestChamberSounds implements AutoCloseable {
    private static final String[] FILES={
        "select.wav","wood_knock.wav","metal_clang.wav",
        "coin_chime.wav","mechanism.wav","beacon.wav"
    };
    private final SoundPool pool;
    private final AudioManager manager;
    private final AudioFocusRequest focusRequest;
    private final int[] ids=new int[FILES.length];
    private final boolean[] ready=new boolean[FILES.length];
    private boolean allLoaded=false;
    private boolean welcomePlayed=false;
    private volatile boolean focused=false;
    private volatile boolean foreground=false;

    TestChamberSounds(Context context,AssetManager assets)throws IOException{
        AudioAttributes attrs=new AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_GAME)
            .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
            .build();
        manager=(AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
        if(manager==null)throw new IOException("Android AudioManager unavailable");
        pool=new SoundPool.Builder().setMaxStreams(4)
            .setAudioAttributes(attrs).build();
        focusRequest=new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN_TRANSIENT)
            .setAudioAttributes(attrs)
            .setOnAudioFocusChangeListener(change->{
                if(change==AudioManager.AUDIOFOCUS_GAIN){
                    focused=true;
                    if(foreground){
                        pool.autoResume();
                        maybeWelcome();
                    }
                }else{
                    focused=false;
                    pool.autoPause();
                }
            }).build();
        pool.setOnLoadCompleteListener((soundPool,id,status)->{
            if(status!=0){
                Log.e("TestChamberSound","Failed to decode cue "+id);
                return;
            }
            synchronized(ready){
                for(int i=0;i<ids.length;i++)if(ids[i]==id)ready[i]=true;
                boolean loaded=true;
                for(boolean value:ready)if(!value)loaded=false;
                allLoaded=loaded;
            }
            // Welcome cue plays once only after the app is foregrounded AND
            // Android actually granted audio focus, regardless of load order.
            maybeWelcome();
        });
        for(int i=0;i<FILES.length;i++){
            try(AssetFileDescriptor asset=assets.openFd("sfx/"+FILES[i])){
                // A tiny WAV may finish decoding before load() returns.
                // Serialize ID assignment with the onLoadComplete callback.
                synchronized(ready){
                    ids[i]=pool.load(asset,1);
                    if(ids[i]==0)throw new IOException("SoundPool refused "+FILES[i]);
                }
            }catch(IOException error){
                pool.release();
                throw error;
            }
        }
    }
    private void maybeWelcome(){
        int cue=0;
        synchronized(ready){
            if(foreground&&focused&&allLoaded&&!welcomePlayed){
                welcomePlayed=true;
                cue=ids[5];
            }
        }
        if(cue!=0){
            int played=pool.play(cue,0.7f,0.7f,1,0,1.f);
            Log.i("TestChamberSound",played==0?
                "Welcome cue failed despite audio focus":
                "All six WAVs loaded; focused welcome beacon played");
        }
    }

    void playForFocus(int model){
        if(!foreground||!focused){
            Log.w("TestChamberSound","Sound suppressed: activity lacks audio focus");
            return;
        }
        // A stable object -> cue mapping makes each gallery asset testable.
        // Empty space selects the general beacon (sixth effect).
        final int index=TestChamberTargeting.cueFor(model);
        synchronized(ready){
            if(!ready[index]){
                Log.w("TestChamberSound","Cue not loaded yet: "+FILES[index]);
                return;
            }
        }
        int stream=pool.play(ids[index],0.92f,0.92f,1,0,1.f);
        if(stream==0)Log.w("TestChamberSound","Cue not playing: "+FILES[index]);
        else Log.i("TestChamberSound","Audible test cue "+FILES[index]);
    }
    void pause(){
        foreground=false;
        focused=false;
        pool.autoPause();
        manager.abandonAudioFocusRequest(focusRequest);
    }
    void resume(){
        foreground=true;
        focused=manager.requestAudioFocus(focusRequest)==
            AudioManager.AUDIOFOCUS_REQUEST_GRANTED;
        if(focused){
            pool.autoResume();
            maybeWelcome();
        }else Log.w("TestChamberSound","Audio focus not granted");
    }
    @Override public void close(){
        pause();
        pool.release();
    }
}
