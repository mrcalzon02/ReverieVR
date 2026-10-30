package io.github.mrcalzon02.reverievr;

import dev.mrcalzon.lighting.LightModel;
import dev.mrcalzon.lighting.LightingFrameProfiler;
import dev.mrcalzon.lighting.LightingModule;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Properties;

/** Offline Java17 smoke for imported Beckenburg lighting + real editor persistence. */
public final class LightingAuthoringPanelTest {
    public static void main(String[] args) throws Exception {
        System.setProperty("java.awt.headless","true");
        LightingModule.Settings balanced=
            LightingModule.preset(LightingModule.Preset.BALANCED);
        LightingModule.Settings performance=
            LightingModule.preset(LightingModule.Preset.PERFORMANCE);
        if(balanced.sourcePercent()!=performance.sourcePercent() ||
           balanced.ambientPercent()!=performance.ambientPercent() ||
           balanced.reachPercent()!=performance.reachPercent())
            throw new AssertionError("Performance preset altered gameplay-visible lighting");
        if(LightModel.angular(LightModel.Beam.OMNIDIRECTIONAL,.25)!=1.0)
            throw new AssertionError("Imported beam math not functional");
        if(LightModel.diffusionFade(10,5,1)!=0.0)
            throw new AssertionError("Light reach falloff invalid");
        if(!(LightModel.diffusionFade(1,10,.5)>0))
            throw new AssertionError("Light diffusion unexpectedly black");
        LightingFrameProfiler profile=new LightingFrameProfiler(8);
        profile.record(16_000_000,2_000_000,8_000_000,2_000_000,1_000_000,
            135,14,2,false);
        if(profile.snapshot().samples()!=1 ||
           profile.snapshot().frameP95Ms()<15.9)
            throw new AssertionError("Imported frame profiler broken");
        Path dir=Files.createTempDirectory("reverie-lighting-");
        Path file=dir.resolve("lighting/sitka.properties");
        LightingAuthoringPanel panel=new LightingAuthoringPanel(file);
        if(panel.dirty())throw new AssertionError("Initial lighting incorrectly dirty");
        panel.save();
        if(!Files.isRegularFile(file)||panel.dirty())
            throw new AssertionError("Lighting profile did not save atomically");
        Properties properties=new Properties();
        try(var input=Files.newInputStream(file)){properties.load(input);}
        if(!"BALANCED".equals(properties.getProperty("preset")) ||
           !"100".equals(properties.getProperty("diffusion")))
            throw new AssertionError("Lighting settings not preserved");
        LightingAuthoringPanel restored=new LightingAuthoringPanel(file);
        if(restored.dirty()||!restored.documentPath().equals(file))
            throw new AssertionError("Lighting reload lost file identity");
        Properties broken=new Properties();
        broken.putAll(properties);
        broken.setProperty("reach","5000");
        try(var output=Files.newOutputStream(file)){broken.store(output,"invalid fixture");}
        try{
            restored.load();
            throw new AssertionError("Invalid authoring profile was silently accepted");
        }catch(java.io.IOException expected){}
        System.out.println("Reverie lighting: PASS Beckenburg presets/falloff/perf and editor save/reload/validation");
    }
}
