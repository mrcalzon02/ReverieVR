import dev.mrcalzon.water.WaterDocument;
import dev.mrcalzon.water.WaterEditorModel;
import dev.mrcalzon.weather.WeatherModule;
import dev.mrcalzon.weather.WeatherType;
import java.nio.file.Files;
import java.util.List;

/** Real headless Sitka water/weather editor smoke; no map imagery or fake geometry. */
public final class SitkaEditorCoreTest {
    public static void main(String[] args) throws Exception {
        WaterEditorModel water = new WaterEditorModel(new WaterDocument("sitka",List.of()));
        String id = water.add(4,0,7,8,12);
        if(water.document().placements().size()!=1||!water.document().placements().get(0).id().equals(id))
            throw new AssertionError("Added water not persisted in editor model");
        java.nio.file.Path folder = Files.createTempDirectory("sitka-editor-test-");
        java.nio.file.Path file = folder.resolve("water/sitka.water");
        water.document().save(file);
        try(var input=Files.newInputStream(file)){
            if(WaterDocument.read(input).placements().size()!=1)
                throw new AssertionError("Water round-trip lost shoreline placement");
        }
        WeatherModule weather=new WeatherModule(0x517CA1L);
        weather.setTimeOfDay(.5);
        weather.setWeather(WeatherType.RAIN,0);
        var state=weather.snapshot();
        weather.setWeather(WeatherType.SNOW,0);
        weather.restore(state);
        if(weather.snapshot().type()!=WeatherType.RAIN)
            throw new AssertionError("Weather mode/snapshot restoration failed");
        System.out.println("Sitka editor: PASS pinned Beckenburg water, save, weather and restore");
    }
}
