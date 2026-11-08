package android.content;

import android.content.res.AssetManager;
import android.media.AudioManager;

public class Context {
    public static final String AUDIO_SERVICE = "audio";
    private final AssetManager assets = new AssetManager();
    private final AudioManager audioManager = new AudioManager();
    public Context getApplicationContext() { return this; }
    public Object getSystemService(String name) { return audioManager; }
    public AssetManager getAssets() { return assets; }
}
