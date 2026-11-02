package io.github.mrcalzon02.reverievr;

import dev.mrcalzon.water.WaterDocument;
import dev.mrcalzon.weather.WeatherType;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import java.util.Set;

/**
 * Strict Sitka region-to-environment links. V1 remains readable and points at
 * the historical shared profile; new V2 links own three per-region documents.
 * REVSCENE itself is unchanged. These are desktop authoring files, not a
 * native game/environment runtime.
 */
final class SitkaRegionLinks {
    private static final String LEGACY_ROOT = "sitka-environment";
    private static final String REGION_ROOT = "sitka-environment/regions/";
    private static final String WATER = "water/sitka.water";
    private static final String WEATHER = "weather/sitka.properties";
    private static final String LIGHTING = "lighting/sitka.properties";
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

    private static String regional(Path scene, String document) {
        return REGION_ROOT + region(scene) + "/" + document;
    }

    private static String links(Path scene, int version) {
        String prefix = version == 1 ? LEGACY_ROOT + "/" : REGION_ROOT + region(scene) + "/";
        return "SITKA-REGION-LINKS\t" + version + "\nregion\t" + region(scene)
            + "\nwater\t" + prefix + WATER
            + "\nweather\t" + prefix + WEATHER
            + "\nlighting\t" + prefix + LIGHTING + "\n";
    }

    static boolean linked(Path scene) { return Files.exists(sidecar(scene)); }

    private static int linkVersion(Path scene) throws IOException {
        Path link = sidecar(scene);
        if (!Files.exists(link)) return 0;
        if (!Files.isRegularFile(link) || Files.isSymbolicLink(link)
            || Files.size(link) > 1024)
            throw new IOException("Invalid Sitka region profile links: " + link);
        String contents = Files.readString(link, StandardCharsets.UTF_8);
        if (contents.equals(links(scene, 1))) return 1;
        if (contents.equals(links(scene, 2))) return 2;
        throw new IOException("Invalid Sitka region profile links: " + link);
    }

    /** Editor location is regional unless opening an explicitly linked V1 scene. */
    static Path editorEnvironment(Path scene) throws IOException {
        Path base = workspace(scene);
        return base.resolve(linkVersion(scene) == 1
            ? LEGACY_ROOT : REGION_ROOT + region(scene));
    }

    private static Path profile(Path workspace, String relative) throws IOException {
        Path root = workspace.toAbsolutePath().normalize();
        Path target = root.resolve(relative).normalize();
        if (!target.startsWith(root))
            throw new IOException("Sitka profile escapes workspace: " + relative);
        for (Path item = target; !item.equals(root); item = item.getParent()) {
            if (Files.isSymbolicLink(item))
                throw new IOException("Symlinked Sitka profile path: " + relative);
        }
        if (!Files.isRegularFile(target))
            throw new IOException("Missing Sitka profile: " + relative);
        if (!target.toRealPath().startsWith(root.toRealPath()))
            throw new IOException("Sitka profile escapes workspace: " + relative);
        return target;
    }

    private static void validateProfiles(Path workspace, String prefix) throws IOException {
        Path water = profile(workspace, prefix + WATER);
        try (var input = Files.newInputStream(water)) {
            if (!"sitka".equals(WaterDocument.read(input).level()))
                throw new IOException("Referenced water document is not for Sitka");
        }
        Path weather = profile(workspace, prefix + WEATHER);
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
        Path lighting = profile(workspace, prefix + LIGHTING);
        if (Files.size(lighting) > 16384)
            throw new IOException("Sitka lighting profile exceeds size limit");
        // Actual pinned Beckenburg lighting settings/schema, not a dummy profile.
        new LightingAuthoringPanel(lighting);
    }

    private static void bounded(Properties values, String key, double min, double max) {
        double value = Double.parseDouble(values.getProperty(key));
        if (!Double.isFinite(value) || value < min || value > max)
            throw new IllegalArgumentException("Out-of-range " + key);
    }

    static void validateOnOpen(Path scene) throws IOException {
        int version = linkVersion(scene);
        if (version == 0) return; // Existing unlinked blockout remains legal.
        String prefix = version == 1
            ? LEGACY_ROOT + "/" : REGION_ROOT + region(scene) + "/";
        validateProfiles(workspace(scene), prefix);
    }

    private static void writeLinks(Path scene, int version) throws IOException {
        Path link = sidecar(scene);
        Files.createDirectories(link.getParent());
        Path pending = Files.createTempFile(link.getParent(), ".sitka-links-", ".tmp");
        try {
            Files.writeString(pending, links(scene, version), StandardCharsets.UTF_8);
            try { Files.move(pending, link, StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING); }
            catch (AtomicMoveNotSupportedException error) {
                Files.move(pending, link, StandardCopyOption.REPLACE_EXISTING);
            }
        } finally { Files.deleteIfExists(pending); }
    }

    private static boolean any(Path root, String prefix) {
        return Files.exists(root.resolve(prefix + WATER))
            || Files.exists(root.resolve(prefix + WEATHER))
            || Files.exists(root.resolve(prefix + LIGHTING));
    }

    private static void seedFromLegacy(Path root, String prefix) throws IOException {
        // Copy-on-write: never change shared source documents or another region.
        // Clean up only files created by this attempt if the copy fails.
        List<Path> created = new ArrayList<>();
        try {
            for (String name : List.of(WATER, WEATHER, LIGHTING)) {
                Path destination = root.resolve(prefix + name);
                Path parent = destination.getParent();
                if (Files.isSymbolicLink(parent) || Files.isSymbolicLink(parent.getParent()))
                    throw new IOException("Unsafe regional profile directory");
                Files.createDirectories(parent);
                if (Files.exists(destination))
                    throw new IOException("Regional profile already exists: " + destination);
                Path pending = Files.createTempFile(parent, ".sitka-profile-", ".tmp");
                try {
                    Files.copy(profile(root, LEGACY_ROOT + "/" + name), pending,
                        StandardCopyOption.REPLACE_EXISTING);
                    try { Files.move(pending, destination, StandardCopyOption.ATOMIC_MOVE); }
                    catch (AtomicMoveNotSupportedException error) {
                        Files.move(pending, destination);
                    }
                    created.add(destination);
                } finally { Files.deleteIfExists(pending); }
            }
        } catch (IOException | RuntimeException failure) {
            for (Path file : created) Files.deleteIfExists(file);
            throw failure;
        }
    }

    static void bindOnSave(Path scene) throws IOException {
        if (linked(scene)) { validateOnOpen(scene); return; }
        Path root = workspace(scene);
        String prefix = REGION_ROOT + region(scene) + "/";
        if (any(root, prefix)) {
            // A partially authored region must be completed in the real editor.
            validateProfiles(root, prefix);
        } else if (any(root, LEGACY_ROOT + "/")) {
            validateProfiles(root, LEGACY_ROOT + "/");
            seedFromLegacy(root, prefix);
            validateProfiles(root, prefix);
        } else {
            return; // Legacy blockout without authored environment.
        }
        writeLinks(scene, 2);
    }
}
