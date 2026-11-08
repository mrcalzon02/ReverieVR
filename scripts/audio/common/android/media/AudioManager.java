package android.media;

public class AudioManager {
    public static final int STREAM_MUSIC = 3;
    public static int plays;
    public void playSoundEffect(int effect, float volume) { plays++; }
}
