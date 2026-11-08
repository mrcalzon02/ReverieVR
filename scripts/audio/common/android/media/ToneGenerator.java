package android.media;

import java.util.ArrayList;
import java.util.List;

public final class ToneGenerator {
    public static final int TONE_PROP_ACK = 1, TONE_PROP_NACK = 2;
    public static final List<Integer> tones = new ArrayList<>();
    public ToneGenerator(int stream, int volume) { }
    public void startTone(int tone, int duration) { tones.add(tone); }
    public void stopTone() { }
    public void release() { }
}
