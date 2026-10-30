package io.github.mrcalzon02.reverievr;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.AtomicMoveNotSupportedException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

/** Platform-independent desktop Home/navigation state. No Android or browser dependency. */
final class DesktopHubState {
    static final int PAGE_SIZE = 4;
    static final String PONG = "classic-pong";
    static final String SNAKE = "classic-snake";
    static final String BREAKOUT = "classic-breakout";
    static final String REVERSI = "classic-reversi";
    static final List<String> PORTED_IDS = List.of(PONG, SNAKE, BREAKOUT, REVERSI);
    enum Panel { NONE, GAMES, SETTINGS, WARDROBE, HELP }
    HomeEnvironment environment = HomeEnvironment.WHITE_ROOM;
    Panel panel = Panel.NONE;
    String runningGame = "";
    String query = "";
    int page;
    int outfit = 0;
    double x = 0, z = 1.3, yaw = 0;
    boolean showHelp = true;
    final Path settingsFile;

    DesktopHubState(Path workspace) {
        settingsFile = workspace.resolve("desktop-home.properties");
    }

    static boolean ported(String id) {
        return PORTED_IDS.contains(id) && GameModuleRegistry.find(id) != null;
    }

    List<GameModuleRegistry.Entry> availableGames() {
        List<GameModuleRegistry.Entry> result = new ArrayList<>();
        for (GameModuleRegistry.Entry entry : GameModuleRegistry.GAMES)
            if (ported(entry.id) && entry.displayName.toLowerCase(java.util.Locale.ROOT)
                    .contains(query.toLowerCase(java.util.Locale.ROOT))) result.add(entry);
        return result;
    }

    int pageCount() {
        return Math.max(1, (availableGames().size() + PAGE_SIZE - 1) / PAGE_SIZE);
    }

    void open(Panel to) {
        if (!runningGame.isEmpty()) return;
        panel = to;
        page = 0;
    }

    void close() { panel = Panel.NONE; page = 0; }

    void nextPage(int direction) {
        int count = pageCount();
        page = ((page + direction) % count + count) % count;
    }

    boolean launch(String id) {
        if (!ported(id) || runningGame.length() != 0) return false;
        runningGame = id;
        panel = Panel.NONE;
        return true;
    }

    void back() {
        if (!runningGame.isEmpty()) { runningGame = ""; panel = Panel.GAMES; }
        else if (panel != Panel.NONE) close();
    }

    void search(char c) {
        if(panel != Panel.GAMES || !runningGame.isEmpty()) return;
        if(c == '\b') {
            if (!query.isEmpty()) query = query.substring(0, query.length() - 1);
        } else if (c >= 32 && c <= 126 && query.length() < 36) {
            query += c;
        } else return;
        page = 0;
    }

    void move(double strafe, double forward, double seconds) {
        if (!runningGame.isEmpty()) return;
        double step = Math.max(0, Math.min(.1, seconds)) * 2.15;
        double dx = Math.cos(yaw) * strafe + Math.sin(yaw) * forward;
        double dz = -Math.sin(yaw) * strafe + Math.cos(yaw) * forward;
        x = clamp(x + dx * step, -1.4, 1.4);
        z = clamp(z - dz * step, -.15, 2.0);
    }

    void turn(double radians) {
        if (!runningGame.isEmpty()) return;
        yaw = clamp(yaw + radians, -1.16, 1.16);
    }

    void recenter() { x = 0; z = 1.3; yaw = 0; }
    private static double clamp(double x, double lo, double hi) {
        return Math.max(lo, Math.min(hi, x));
    }

    void load() throws IOException {
        if (!Files.exists(settingsFile)) return;
        if (Files.size(settingsFile) > 8192) throw new IOException("Desktop settings size invalid");
        Properties p = new Properties();
        try (InputStream in = Files.newInputStream(settingsFile)) {p.load(in);}
        environment = HomeEnvironment.fromPreference(p.getProperty("environment"));
        String outfitValue = p.getProperty("outfit", "0");
        try {outfit = Math.max(0, Math.min(5, Integer.parseInt(outfitValue)));}
        catch(NumberFormatException ex) {outfit = 0;}
        showHelp = Boolean.parseBoolean(p.getProperty("showHelp", "true"));
    }

    void save() throws IOException {
        Files.createDirectories(settingsFile.getParent());
        Properties p = new Properties();
        p.setProperty("environment", environment.preferenceValue);
        p.setProperty("outfit", Integer.toString(outfit));
        p.setProperty("showHelp", Boolean.toString(showHelp));
        Path temp = Files.createTempFile(settingsFile.getParent(),".rvdesktop-",".tmp");
        try {
            try(OutputStream out = Files.newOutputStream(temp)) {
                p.store(out, "ReverieVR desktop Home settings v1");
            }
            try {Files.move(temp,settingsFile,StandardCopyOption.ATOMIC_MOVE,
                StandardCopyOption.REPLACE_EXISTING);}
            catch(AtomicMoveNotSupportedException ex) {
                Files.move(temp,settingsFile,StandardCopyOption.REPLACE_EXISTING);
            }
        } finally {Files.deleteIfExists(temp);}
    }
}
