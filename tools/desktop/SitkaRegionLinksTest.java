package io.github.mrcalzon02.reverievr;

import dev.mrcalzon.water.WaterDocument;
import dev.mrcalzon.water.WaterEditorModel;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Properties;

/** Real per-region Sitka water/weather/lighting save/reopen and legacy link acceptance. */
public final class SitkaRegionLinksTest {
    private static void rejected(Path scene) throws Exception {
        try {
            SitkaRegionLinks.validateOnOpen(scene);
            throw new AssertionError("Broken linked region was accepted");
        } catch (IOException expected) { }
    }

    private static void weather(Path file, String condition, double hour) throws IOException {
        Files.createDirectories(file.getParent());
        Files.writeString(file, "version=1\ncondition=" + condition
            + "\nhour=" + hour + "\nwind_mps=3.0\nwind_heading_deg=85.0\n");
    }

    private static Properties properties(Path file) throws IOException {
        Properties values = new Properties();
        try (var input = Files.newInputStream(file)) { values.load(input); }
        return values;
    }

    private static void lighting(Path file, int ambient) throws IOException {
        Properties values = properties(file);
        values.setProperty("preset", "CUSTOM");
        values.setProperty("ambient", Integer.toString(ambient));
        try (var output = Files.newOutputStream(file)) {
            values.store(output, "Sitka region lighting authoring");
        }
        new LightingAuthoringPanel(file); // The actual Beckenburg schema parser.
    }

    private static int waterCount(Path file) throws IOException {
        try (var input = Files.newInputStream(file)) {
            return WaterDocument.read(input).placements().size();
        }
    }

    public static void main(String[] args) throws Exception {
        System.setProperty("java.awt.headless", "true");
        Path root = Files.createTempDirectory("sitka-independent-regions-");
        Path harbor = ReverieDesktop.sceneFile(root, "sitka", "harbor");
        Path bay = ReverieDesktop.sceneFile(root, "sitka", "crescent-bay");
        final SceneDocument[] active = {new SceneDocument()};
        ReverieSceneProject project = new ReverieSceneProject("sitka", harbor,
            () -> active[0], value -> active[0] = value);
        active[0].pieces.add(new SceneDocument.Piece(
            "pier", "box", "-", 4, 0, 7, 20, 2));
        project.save();
        if (SitkaRegionLinks.linked(harbor))
            throw new AssertionError("Created fictitious environment references");

        // Genuine shared documents are a read-only seed for NEW V2 regions.
        Path shared = root.resolve("sitka-environment");
        Path waterFile = shared.resolve("water/sitka.water");
        WaterEditorModel sourceWater = new WaterEditorModel(new WaterDocument("sitka", List.of()));
        sourceWater.add(4, 0, 7, 8, 12);
        sourceWater.document().save(waterFile);
        Path weatherFile = shared.resolve("weather/sitka.properties");
        weather(weatherFile, "RAIN", 14.5);
        Path lightFile = shared.resolve("lighting/sitka.properties");
        new LightingAuthoringPanel(lightFile).save();

        project.save();
        SitkaRegionLinks.validateOnOpen(harbor);
        Path harborEnv = SitkaRegionLinks.editorEnvironment(harbor);
        if (!harborEnv.equals(root.resolve("sitka-environment/regions/harbor"))
            || !Files.readString(SitkaRegionLinks.sidecar(harbor)).startsWith("SITKA-REGION-LINKS\t2\n"))
            throw new AssertionError("Harbor did not receive independent V2 links");

        final SceneDocument[] second = {new SceneDocument()};
        second[0].pieces.add(new SceneDocument.Piece(
            "cliff", "cylinder", "-", -8, 1, 2, 0, 3));
        ReverieSceneProject bayProject = new ReverieSceneProject(
            "sitka", bay, () -> second[0], value -> second[0] = value);
        bayProject.save();
        SitkaRegionLinks.validateOnOpen(bay);
        Path bayEnv = SitkaRegionLinks.editorEnvironment(bay);
        if (harborEnv.equals(bayEnv) || !Files.isRegularFile(bayEnv.resolve("water/sitka.water")))
            throw new AssertionError("Regions still share environment files");

        // Author deliberately different real documents in both regions.
        WaterEditorModel harborWater;
        try (var input = Files.newInputStream(harborEnv.resolve("water/sitka.water"))) {
            harborWater = new WaterEditorModel(WaterDocument.read(input));
        }
        harborWater.add(20, 0, 12, 6, 9);
        harborWater.document().save(harborEnv.resolve("water/sitka.water"));
        weather(harborEnv.resolve("weather/sitka.properties"), "FOG", 6.5);
        lighting(harborEnv.resolve("lighting/sitka.properties"), 80);

        WaterEditorModel bayWater;
        try (var input = Files.newInputStream(bayEnv.resolve("water/sitka.water"))) {
            bayWater = new WaterEditorModel(WaterDocument.read(input));
        }
        bayWater.add(-10, 0, 2, 4, 5);
        bayWater.add(-12, 0, 4, 3, 4);
        bayWater.document().save(bayEnv.resolve("water/sitka.water"));
        weather(bayEnv.resolve("weather/sitka.properties"), "SNOW", 19.0);
        lighting(bayEnv.resolve("lighting/sitka.properties"), 120);

        // Real scene edits and separate profile edits survive both reopen paths.
        active[0] = SceneDocument.load(harbor);
        active[0].pieces.get(0).x = 17;
        project.save();
        bayProject.save();
        SitkaRegionLinks.validateOnOpen(harbor);
        SitkaRegionLinks.validateOnOpen(bay);
        if (SceneDocument.load(harbor).pieces.get(0).x != 17
            || SceneDocument.load(bay).pieces.get(0).x != -8)
            throw new AssertionError("Region scenes overwrote each other");
        if (waterCount(harborEnv.resolve("water/sitka.water")) != 2
            || waterCount(bayEnv.resolve("water/sitka.water")) != 3
            || waterCount(waterFile) != 1)
            throw new AssertionError("Water placements leaked between regions or shared seed");
        if (!"FOG".equals(properties(harborEnv.resolve("weather/sitka.properties")).getProperty("condition"))
            || !"SNOW".equals(properties(bayEnv.resolve("weather/sitka.properties")).getProperty("condition"))
            || !"RAIN".equals(properties(weatherFile).getProperty("condition")))
            throw new AssertionError("Weather leaked between regions or shared seed");
        if (!"80".equals(properties(harborEnv.resolve("lighting/sitka.properties")).getProperty("ambient"))
            || !"120".equals(properties(bayEnv.resolve("lighting/sitka.properties")).getProperty("ambient"))
            || "80".equals(properties(lightFile).getProperty("ambient")))
            throw new AssertionError("Lighting leaked between regions or shared seed");

        // V1 sidecars continue to open exactly as written, without silent migration.
        Path legacy = ReverieDesktop.sceneFile(root, "sitka", "legacy-dock");
        String legacyLinks = "SITKA-REGION-LINKS\t1\nregion\tlegacy-dock\n"
            + "water\tsitka-environment/water/sitka.water\n"
            + "weather\tsitka-environment/weather/sitka.properties\n"
            + "lighting\tsitka-environment/lighting/sitka.properties\n";
        Files.createDirectories(legacy.getParent());
        Files.writeString(SitkaRegionLinks.sidecar(legacy), legacyLinks);
        SitkaRegionLinks.validateOnOpen(legacy);
        if (!SitkaRegionLinks.editorEnvironment(legacy).equals(shared))
            throw new AssertionError("Legacy V1 editor no longer resolves its shared profiles");
        SitkaRegionLinks.bindOnSave(legacy);
        if (!legacyLinks.equals(Files.readString(SitkaRegionLinks.sidecar(legacy))))
            throw new AssertionError("Legacy link was silently migrated");

        Files.delete(harborEnv.resolve("weather/sitka.properties"));
        rejected(harbor);
        SitkaRegionLinks.validateOnOpen(bay); // Other region stays usable.
        try {
            project.save();
            throw new AssertionError("Saved a region with a missing profile");
        } catch (IOException expected) { }
        weather(harborEnv.resolve("weather/sitka.properties"), "FOG", 6.5);
        weather(bayEnv.resolve("weather/sitka.properties"), "SNOW", Double.NaN);
        rejected(bay);
        weather(bayEnv.resolve("weather/sitka.properties"), "SNOW", 19.0);
        String validLink = Files.readString(SitkaRegionLinks.sidecar(harbor));
        Files.writeString(SitkaRegionLinks.sidecar(harbor),
            validLink.replace("sitka-environment/regions/harbor/water/sitka.water", "../escape.water"));
        rejected(harbor);
        Files.writeString(SitkaRegionLinks.sidecar(harbor), validLink);
        SitkaRegionLinks.validateOnOpen(harbor);

        // Partial regional authoring must fail rather than fall back to shared.
        Path incomplete = ReverieDesktop.sceneFile(root, "sitka", "unlinked-new");
        Path incompleteWater = root.resolve("sitka-environment/regions/unlinked-new/water/sitka.water");
        sourceWater.document().save(incompleteWater);
        final SceneDocument[] third = {new SceneDocument()};
        try {
            new ReverieSceneProject("sitka", incomplete,
                () -> third[0], value -> third[0] = value).save();
            throw new AssertionError("Incomplete region silently linked");
        } catch (IOException expected) { }
        if (Files.exists(incomplete) || SitkaRegionLinks.linked(incomplete))
            throw new AssertionError("Failed save mutated an incomplete scene");

        System.out.println("Sitka independent regions: PASS two-region real water/weather/lighting edit-save-reopen, V1 readback, missing/corrupt/unsafe/partial refusal");
    }
}
