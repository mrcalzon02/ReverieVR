package android.media;

public final class AudioAttributes {
    public static final int USAGE_GAME = 14;
    public static final int CONTENT_TYPE_SONIFICATION = 4;
    public final int usage, contentType;
    private AudioAttributes(int usage, int contentType) {
        this.usage = usage; this.contentType = contentType;
    }
    public static final class Builder {
        private int usage, contentType;
        public Builder setUsage(int value) { usage = value; return this; }
        public Builder setContentType(int value) { contentType = value; return this; }
        public AudioAttributes build() { return new AudioAttributes(usage, contentType); }
    }
}
