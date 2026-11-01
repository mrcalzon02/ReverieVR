package io.github.mrcalzon02.reverievr;

import dev.mrcalzon.water.WaterDocument;
import dev.mrcalzon.water.WaterEditorModel;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Properties;

/** Actual Sitka region -> pinned environment authoring round-trip, no fake runtime. */
public final class SitkaRegionLinksTest {
    private static void rejected(Path scene) throws Exception {
        try {
            SitkaRegionLinks.validateOnOpen(scene);
            throw new AssertionError("Broken linked region was accepted");
        } catch (IOException expected) { }
    }

    public static void main(String[] args) throws Exception {
        System.setProperty("java.awt.headless", "true");
        Path root = Files.createTempDirectory("sitka-linked-region-");
        Path harbor = ReverieDesktop.sceneFile(root, "sitka", "harbor");
        Path bay = ReverieDesktop.sceneFile(root, "sitka", "crescent-bay");
        final SceneDocument[] active = {new SceneDocument()};
        ReverieSceneProject project = new ReverieSceneProject("sitka", harbor,
            () -> active[0], value -> active[0] = value);
        active[0].pieces.add(new SceneDocument.Piece(
            "pier", "box", "-", 4, 0, 7, 20, 2));
        project.save(); // Existing scenes remain legal without invented profiles.
        if (SitkaRegionLinks.linked(harbor))
            throw new AssertionError("Created fictitious environment references");

        Path waterFile = root.resolve("sitka-environment/water/sitka.water");
        WaterEditorModel water = new WaterEditorModel(new WaterDocument("sitka", List.of()));
        water.add(4, 0, 7, 8, 12);
        water.document().save(waterFile);
        Path weatherFile = root.resolve("sitka-environment/weather/sitka.properties");
        Files.createDirectories(weatherFile.getParent());
        String weather = "version=1\\ncondition=RAIN\\nhour=14.5\\nwind_mps=3.0\\nwind_heading_deg=85.0\\n";
        Files.writeString(weatherFile, weather);
        Path lightFile = root.resolve("sitka-environment/lighting/sitka.properties");
        new LightingAuthoringPanel(lightFile).save();
        project.save(); // Bind existing genuine profiles without rewriting REVSCENE.
        if (!SitkaRegionLinks.linked(harbor))
            throw new AssertionError("Existing environment was not linked to region");
        SitkaRegionLinks.validateOnOpen(harbor);
        String originalLink = Files.readString(SitkaRegionLinks.sidecar(harbor));
        if (!originalLink.contains("region\\tharbor\\n")
            || !originalLink.contains("water\\tsitka-environment/water/sitka.water"))
            throw new AssertionError("Wrong saved region or profile references");

        // Simulate close, reopen, edit, save and reopen the REAL scene document.
        active[0] = SceneDocument.load(harbor);
        if (active[0].pieces.size() != 1 || active[0].pieces.get(0).x != 4)
            throw new AssertionError("Region blockout lost on first reopen");
        active[0].pieces.get(0).x = 17;
        project.save();
        SitkaRegionLinks.validateOnOpen(harbor);
        if (SceneDocument.load(harbor).pieces.get(0).x != 17
            || !originalLink.equals(Files.readString(SitkaRegionLinks.sidecar(harbor))))
            throw new AssertionError("Region edit/reopen changed environment references");
        Properties loaded = new Properties();
        try (var input = Files.newInputStream(weatherFile)) { loaded.load(input); }
        if (!"RAIN".equals(loaded.getProperty("condition"))
            || !"14.5".equals(loaded.getProperty("hour"))
            || !"85.0".equals(loaded.getProperty("wind_heading_deg")))
            throw new AssertionError("Linked authored weather values were lost");
        try (var input = Files.newInputStream(waterFile)) {
            if (WaterDocument.read(input).placements().size() != 1)
                throw new AssertionError("Linked water placements were lost");
        }
        new LightingAuthoringPanel(lightFile); // Actual lighting schema reload.

        // A second named region binds to the same explicit profiles, not a
        // fabricated duplicate or a replacement of the first region.
        final SceneDocument[] second = {new SceneDocument()};
        second[0].pieces.add(new SceneDocument.Piece(
            "cliff", "cylinder", "-", -8, 1, 2, 0, 3));
        new ReverieSceneProject("sitka", bay, () -> second[0], value -> second[0] = value).save();
        SitkaRegionLinks.validateOnOpen(bay);
        if (SceneDocument.load(harbor).pieces.get(0).x != 17
            || SceneDocument.load(bay).pieces.get(0).x != -8)
            throw new AssertionError("Regions overwrote each other");

        Files.delete(weatherFile);
        rejected(harbor); // No silent default when a linked profile disappears.
        try {
            project.save();
            throw new AssertionError("Saved a scene with a missing linked profile");
        } catch (IOException expected) { }
        Files.writeString(weatherFile, weather);
        Files.writeString(weatherFile, weather.replace("hour=14.5", "hour=NaN"));
        rejected(harbor);
        Files.writeString(weatherFile, weather);
        Files.writeString(SitkaRegionLinks.sidecar(harbor),
            originalLink.replace("sitka-environment/water/sitka.water", "../escape.water"));
        rejected(harbor);
        Files.writeString(SitkaRegionLinks.sidecar(harbor), originalLink);
        SitkaRegionLinks.validateOnOpen(harbor);

        Files.delete(lightFile);
        Path unlinked = ReverieDesktop.sceneFile(root, "sitka", "unlinked-new");
        final SceneDocument[] third = {new SceneDocument()};
        try {
            new ReverieSceneProject("sitka", unlinked, () -> third[0], value -> third[0] = value).save();
            throw new AssertionError("Incomplete environment silently linked");
        } catch (IOException expected) { }
        if (Files.exists(unlinked) || SitkaRegionLinks.linked(unlinked))
            throw new AssertionError("Failed save mutated an unlinked scene");

        System.out.println("Sitka linked regions: PASS two-region scene edit/save/reopen, real water/weather/lighting readback, missing/corrupt/unsafe reference rejection");
    }
}
