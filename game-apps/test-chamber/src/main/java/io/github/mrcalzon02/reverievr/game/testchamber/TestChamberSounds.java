package io.github.mrcalzon02.reverievr.game.testchamber;

import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.AudioAttributes;
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
    private final int[] ids=new int[FILES.length];
    private final boolean[] ready=new boolean[FILES.length];
    private int next=0;

    TestChamberSounds(AssetManager assets)throws IOException{
        AudioAttributes attrs=new AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_GAME)
            .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
            .build();
        pool=new SoundPool.Builder().setMaxStreams(4)
            .setAudioAttributes(attrs).build();
        pool.setOnLoadCompleteListener((soundPool,id,status)->{
            if(status!=0){
                Log.e("TestChamberSound","Failed to decode cue "+id);
                return;
            }
            synchronized(ready){
                for(int i=0;i<ids.length;i++)
                    if(ids[i]==id)ready[i]=true;
            }
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
    void playNext(){
        final int index=next++%FILES.length;
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
    @Override public void close(){
        pool.release();
    }
}
