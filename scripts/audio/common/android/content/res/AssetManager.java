package android.content.res;

import java.io.IOException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/** Canonical three-second PCM fixture metadata, with explicit fault injection. */
public final class AssetManager {
    public final Set<String> missing = new HashSet<>();
    public final Map<String, Long> lengths = new HashMap<>();
    public final Map<String, Integer> opens = new HashMap<>();
    public AssetFileDescriptor openFd(String path) throws IOException {
        opens.merge(path, 1, Integer::sum);
        if (missing.contains(path)) throw new IOException("missing fixture: " + path);
        return new AssetFileDescriptor(path, lengths.getOrDefault(path, 44L + 44_100L * 3L));
    }
    public int opensFor(String path) { return opens.getOrDefault(path, 0); }
    public int totalOpens() { return opens.values().stream().mapToInt(Integer::intValue).sum(); }
}
