package io.github.mrcalzon02.reverievr;

import dev.mrcalzon.water.WaterDocument;
import dev.mrcalzon.weather.WeatherType;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.Properties;
import java.util.Set;

/**
 * Sitka named-region sidecar linking the REAL shared environment documents.
 * REVSCENE remains unchanged and usable by existing native/desktop readers.
 * A missing sidecar means an older, explicitly unlinked blockout, not a
 * fabricated water/weather/lighting profile.
 */
final class SitkaRegionLinks {
    private static final String WATER = "sitka-environment/water/sitka.water";
    private static final String WEATHER = "sitka-environment/weather/sitka.properties";
    private static final String LIGHTING = "sitka-environment/lighting/sitka.properties";
    private static final Set<String> WEATHER_KEYS =
        Set.of("version", "condition", "hour", "wind_mps", "wind_heading_deg");
    private static final Set<WeatherType> AUTHORABLE_WEATHER = Set.of(
        WeatherType.CLEAR, WeatherType.OVERCAST, WeatherType.DRIZZLE,
        WeatherType.RAIN, WeatherType.HEAVY_RAIN, WeatherType.FOG,
        WeatherType.DENSE_FOG, WeatherType.SNOW, WeatherType.THUNDERSTORM);

    private SitkaRegionLinks() {}

    private static String region(Path scene) {
        String name = scene.getFileName().toString();
        if (!name.matches("[a-z0-9][a-z0-9-]{0,47}\\.rvscene"))
            throw new IllegalArgumentException("Not a safe Sitka region scene: " + name);
        return name.substring(0, name.length() - ".rvscene".length());
    }

    static Path sidecar(Path scene) {
        return scene.resolveSibling(region(scene) + ".rvlinks");
    }

    private static Path workspace(Path scene) throws IOException {
        Path normalized = scene.toAbsolutePath().normalize();
        Path sitka = normalized.getParent();
        if (sitka == null || !"sitka".equals(sitka.getFileName().toString())
            || sitka.getParent() == null
            || !"scenes".equals(sitka.getParent().getFileName().toString()))
            throw new IOException("Sitka region must be under workspace/scenes/sitka");
        return sitka.getParent().getParent();
    }

    private static Path profile(Path workspace, String relative) throws IOException {
        Path target = workspace.resolve(relative);
        if (!Files.isRegularFile(target) || Files.isSymbolicLink(target))
            throw new IOException("Missing or unsafe Sitka profile: " + relative);
        Path realRoot = workspace.toRealPath();
        if (!target.toRealPath().startsWith(realRoot))
            throw new IOException("Sitka profile escapes workspace: " + relative);
        return target;
    }

    private static void validateProfiles(Path workspace) throws IOException {
        Path water = profile(workspace, WATER);
        try (var input = Files.newInputStream(water)) {
            if (!"sitka".equals(WaterDocument.read(input).level()))
                throw new IOException("Referenced water document is not for Sitka");
        }
        Path weather = profile(workspace, WEATHER);
        if (Files.size(weather) > 4096)
            throw new IOException("Sitka weather profile exceeds size limit");
        Properties values = new Properties();
        try (var input = Files.newInputStream(weather)) { values.load(input); }
        try {
            if (!values.stringPropertyNames().equals(WEATHER_KEYS)
                || !"1".equals(values.getProperty("version")))
                throw new IllegalArgumentException("Unsupported weather fields/version");
            WeatherType condition = WeatherType.valueOf(values.getProperty("condition"));
            if (!AUTHORABLE_WEATHER.contains(condition))
                throw new IllegalArgumentException("Weather mode is not authorable by Sitka");
            bounded(values, "hour", 0, 23.9);
            bounded(values, "wind_mps", 0, 60);
            bounded(values, "wind_heading_deg", 0, 360);
        } catch (RuntimeException error) {
            throw new IOException("Invalid referenced Sitka weather profile", error);
        }
        // The actual Beckenburg-backed panel owns lighting schema validation.
        new LightingAuthoringPanel(profile(workspace, LIGHTING));
    }

    private static void bounded(Properties values, String key, double min, double max) {
        double value = Double.parseDouble(values.getProperty(key));
        if (!Double.isFinite(value) || value < min || value > max)
            throw new IllegalArgumentException("Out-of-range " + key);
    }

    private static String expected(Path scene) {
        return "SITKA-REGION-LINKS\t1\nregion\t" + region(scene)
            + "\nwater\t" + WATER + "\nweather\t" + WEATHER
            + "\nlighting\t" + LIGHTING + "\n";
    }

    static boolean linked(Path scene) { return Files.exists(sidecar(scene)); }

    static void validateOnOpen(Path scene) throws IOException {
        Path link = sidecar(scene);
        if (!Files.exists(link)) return; // Existing unlinked authoring blockout.
        if (!Files.isRegularFile(link) || Files.isSymbolicLink(link)
            || Files.size(link) > 1024
            || !Files.readString(link, StandardCharsets.UTF_8).equals(expected(scene)))
            throw new IOException("Invalid Sitka region profile links: " + link);
        validateProfiles(workspace(scene));
    }

    static void bindOnSave(Path scene) throws IOException {
        if (linked(scene)) { validateOnOpen(scene); return; }
        Path base = workspace(scene);
        boolean water = Files.exists(base.resolve(WATER));
        boolean weather = Files.exists(base.resolve(WEATHER));
        boolean lighting = Files.exists(base.resolve(LIGHTING));
        if (!water && !weather && !lighting) return; // Legacy blockout, no invented links.
        if (!water || !weather || !lighting)
            throw new IOException("Sitka environment profiles are incomplete; save water, weather and lighting before linking this region");
        validateProfiles(base);
        Path link = sidecar(scene);
        Files.createDirectories(link.getParent());
        Path pending = Files.createTempFile(link.getParent(), ".sitka-links-", ".tmp");
        try {
            Files.writeString(pending, expected(scene), StandardCharsets.UTF_8);
            try { Files.move(pending, link, StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING); }
            catch (AtomicMoveNotSupportedException error) {
                Files.move(pending, link, StandardCopyOption.REPLACE_EXISTING);
            }
        } finally { Files.deleteIfExists(pending); }
    }
}
