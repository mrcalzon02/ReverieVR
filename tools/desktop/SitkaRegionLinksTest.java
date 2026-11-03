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

    private static void unchanged(Path[] files, byte[][] originals) throws IOException {
        for (int i=0; i<files.length; i++) {
            if (!java.util.Arrays.equals(originals[i], Files.readAllBytes(files[i])))
                throw new AssertionError("Failed region switch modified " + files[i]);
        }
    }
    private static void unchangedExcept(Path[] files, byte[][] originals, Path injected)
            throws IOException {
        for (int i=0; i<files.length; i++) {
            if (!files[i].equals(injected)
                && !java.util.Arrays.equals(originals[i], Files.readAllBytes(files[i])))
                throw new AssertionError("Rejected region switch modified " + files[i]);
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

        // Actual desktop Water and weather button delegates to this resolver.
        // Editing a pending region name must not retarget the loaded project.
        if (!ReverieDesktop.openRegionEnvironment("sitka", project).equals(harborEnv)
            || !ReverieDesktop.openRegionEnvironment("sitka", bayProject).equals(bayEnv))
            throw new AssertionError("Desktop environment launch ignored its open region");
        Path pendingRegion = ReverieDesktop.sceneFile(root, "sitka", "crescent-bay");
        if (!pendingRegion.equals(bay)
            || !ReverieDesktop.openRegionEnvironment("sitka", project).equals(harborEnv))
            throw new AssertionError("Typing a pending region redirected the open harbor editor");
        try {
            ReverieDesktop.openRegionEnvironment("home", project);
            throw new AssertionError("Non-Sitka project could launch a Sitka environment");
        } catch (IOException expected) { }

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

        // The workbench uses prepareScene before changing its live selection.
        // Exercise that exact preparation path with two genuinely distinct
        // authored regions; a failed switch must leave the open harbor editor
        // and every authoritative profile untouched, not silently repair data.
        ReverieDesktop.PreparedScene selected=ReverieDesktop.prepareScene(root,"sitka","harbor");
        Path harborLight=harborEnv.resolve("lighting/sitka.properties");
        Path bayLight=bayEnv.resolve("lighting/sitka.properties");
        Path bayWeather=bayEnv.resolve("weather/sitka.properties");
        if (!selected.path().equals(harbor)
            || !selected.lighting().documentPath().equals(harborLight))
            throw new AssertionError("Initial workbench selection is not harbor");
        Path[] guarded={
            harbor, harborEnv.resolve("water/sitka.water"),
            harborEnv.resolve("weather/sitka.properties"), harborLight,
            bay, bayEnv.resolve("water/sitka.water"), bayWeather, bayLight,
            waterFile, weatherFile, lightFile
        };
        byte[][] original=new byte[guarded.length][];
        for(int i=0;i<guarded.length;i++) original[i]=Files.readAllBytes(guarded[i]);
        String validBayLighting=Files.readString(bayLight);

        // A malformed destination lighting profile must fail during actual
        // lighting-dock construction, before any live region is rebound.
        String invalidLighting="schema=invalid\npreset=CUSTOM\n";
        Files.writeString(bayLight,invalidLighting);
        try {
            selected=ReverieDesktop.prepareScene(root,"sitka","crescent-bay");
            throw new AssertionError("Malformed destination lighting opened");
        } catch (IOException expected) { }
        if (!invalidLighting.equals(Files.readString(bayLight)))
            throw new AssertionError("Rejected lighting open rewrote invalid destination");
        unchangedExcept(guarded,original,bayLight);
        Files.writeString(bayLight,validBayLighting);
        unchanged(guarded,original);

        // A missing destination lighting profile is not an invitation to
        // substitute the unsaved BALANCED default or to modify harbor.
        Files.delete(bayLight);
        try {
            selected=ReverieDesktop.prepareScene(root,"sitka","crescent-bay");
            throw new AssertionError("Missing destination lighting opened");
        } catch (IOException expected) { }
        if (Files.exists(bayLight))
            throw new AssertionError("Rejected missing lighting open created a default profile");
        unchangedExcept(guarded,original,bayLight);
        Files.writeString(bayLight,validBayLighting);
        unchanged(guarded,original);

        // The weather parser must reject a non-finite destination value;
        // the previously opened harbor retains both its dock and scene.
        weather(bayWeather,"SNOW",Double.NaN);
        String invalidWeather=Files.readString(bayWeather);
        try {
            selected=ReverieDesktop.prepareScene(root,"sitka","crescent-bay");
            throw new AssertionError("Malformed destination weather opened");
        } catch (IOException expected) { }
        if (!invalidWeather.equals(Files.readString(bayWeather)))
            throw new AssertionError("Rejected weather open rewrote invalid destination");
        unchangedExcept(guarded,original,bayWeather);
        weather(bayWeather,"SNOW",19.0);
        unchanged(guarded,original);
        if (!selected.path().equals(harbor)
            || !selected.lighting().documentPath().equals(harborLight)
            || !ReverieDesktop.openRegionEnvironment("sitka",project).equals(harborEnv))
            throw new AssertionError("Failed destination switch rebound harbor authoring");
        project.save();
        unchanged(guarded,original);

        // Repair the destination, then complete the same switch, save, and
        // reopen both real scene documents without cross-region profile writes.
        selected=ReverieDesktop.prepareScene(root,"sitka","crescent-bay");
        if (!selected.path().equals(bay)
            || !selected.lighting().documentPath().equals(bayLight))
            throw new AssertionError("Repaired destination did not become selectable");
        second[0].pieces.get(0).x=-9;
        bayProject.save();
        if (SceneDocument.load(bay).pieces.get(0).x!=-9
            || SceneDocument.load(harbor).pieces.get(0).x!=17
            || waterCount(bayEnv.resolve("water/sitka.water"))!=3
            || waterCount(harborEnv.resolve("water/sitka.water"))!=2
            || !"SNOW".equals(properties(bayWeather).getProperty("condition"))
            || !"FOG".equals(properties(harborEnv.resolve("weather/sitka.properties")).getProperty("condition"))
            || !"120".equals(properties(bayLight).getProperty("ambient"))
            || !"80".equals(properties(harborLight).getProperty("ambient")))
            throw new AssertionError("Successful switch/save/reopen mixed region state");
        for(int i=1;i<guarded.length;i++)
            if(i!=4 && !java.util.Arrays.equals(original[i],Files.readAllBytes(guarded[i])))
                throw new AssertionError("Successful scene save overwrote environment profile: "+guarded[i]);

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
        ReverieSceneProject legacyProject = new ReverieSceneProject(
            "sitka", legacy, () -> new SceneDocument(), ignored -> {});
        if (!ReverieDesktop.openRegionEnvironment("sitka", legacyProject).equals(shared))
            throw new AssertionError("Desktop editor launch lost V1 shared-profile compatibility");
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
        try {
            ReverieDesktop.openRegionEnvironment("sitka", bayProject);
            throw new AssertionError("Desktop launched an invalid linked weather profile");
        } catch (IOException expected) { }
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

        System.out.println("Sitka independent regions: PASS atomic failed lighting/weather switch, restored successful save/reopen, independent water/weather/lighting, V1, unsafe/partial refusal");
    }
}
