package android.content.res;

public final class AssetFileDescriptor implements AutoCloseable {
    public final String path;
    private final long length;
    AssetFileDescriptor(String path, long length) { this.path = path; this.length = length; }
    public long getLength() { return length; }
    @Override public void close() { }
}
