package dev.mrcalzon.lighting;

import java.util.Objects;
import java.util.prefs.Preferences;

/**
 * Standalone, Java-17 lighting configuration module. It owns no gameplay
 * authority, map state, graphics context or Swing objects. The game uses an
 * immutable snapshot per renderer; changing settings rebuilds only the local
 * renderer's lighting caches at the next scene frame.
 */
public final class LightingModule {
    private LightingModule() {}
    public enum Preset {
        PERFORMANCE("Performance"), BALANCED("Balanced"), QUALITY("Quality"),
        CUSTOM("Custom");
        private final String label;
        Preset(String label){this.label=label;}
        @Override public String toString(){return label;}
    }

    public record Settings(Preset preset, int ambientPercent, int sourcePercent,
            int reachPercent, int diffusionPercent, int flickerPercent, int cacheBudgetPercent,
            int cellMetres, boolean distantOutdoorLod,
            boolean changedOnlyVertices, boolean dependencyReuse) {
        /** Retain binary/source compatibility with the first release's
         * ten-field settings; default diffusion reproduces its linear falloff. */
        public Settings(Preset preset,int ambientPercent,int sourcePercent,
                int reachPercent,int flickerPercent,int cacheBudgetPercent,
                int cellMetres,boolean distantOutdoorLod,
                boolean changedOnlyVertices,boolean dependencyReuse){
            this(preset,ambientPercent,sourcePercent,reachPercent,100,
                flickerPercent,cacheBudgetPercent,cellMetres,distantOutdoorLod,
                changedOnlyVertices,dependencyReuse);
        }
        public Settings {
            Objects.requireNonNull(preset,"lighting preset");
            if(ambientPercent<50||ambientPercent>150
                    ||sourcePercent<50||sourcePercent>150
                    ||reachPercent<65||reachPercent>140
                    ||diffusionPercent<5||diffusionPercent>100
                    ||flickerPercent<0||flickerPercent>200
                    ||cacheBudgetPercent<50||cacheBudgetPercent>150
                    ||(cellMetres!=2&&cellMetres!=4&&cellMetres!=8))
                throw new IllegalArgumentException("Lighting options outside safe ranges");
        }
        public double ambientScale(){return ambientPercent/100.0;}
        public double sourceScale(){return sourcePercent/100.0;}
        public double reachScale(){return reachPercent/100.0;}
        /** Width of the fading edge as a fraction of source reach. */
        public double diffusionFraction(){return diffusionPercent/100.0;}
        public double flickerScale(){return flickerPercent/100.0;}
        public int cacheLimit(int baseline){
            if(baseline<1)throw new IllegalArgumentException("cache baseline");
            return Math.max(128,(int)Math.min(Integer.MAX_VALUE,
                    (long)baseline*cacheBudgetPercent/100));
        }
    }
    public static Settings preset(Preset preset) {
        return switch(Objects.requireNonNull(preset,"preset")) {
            // Presets tune GPU/CPU workload, not the visibility of evidence.
            // Brightness, radius and cosmetic flicker remain explicit custom controls.
            case PERFORMANCE -> new Settings(preset,100,100,100,100,100,70,4,true,true,true);
            case BALANCED -> new Settings(preset,100,100,100,100,100,100,4,true,true,true);
            case QUALITY -> new Settings(preset,100,100,100,100,100,140,2,false,true,true);
            case CUSTOM -> throw new IllegalArgumentException("Custom requires selected values");
        };
    }

    private static final Preferences PREF=Preferences.userNodeForPackage(LightingModule.class);
    private static volatile Settings active=load();

    private static int safeInt(String key,int fallback,int low,int high){
        return Math.max(low,Math.min(high,PREF.getInt(key,fallback)));
    }
    private static Settings load(){
        Preset selected;
        try {selected=Preset.valueOf(PREF.get("preset","BALANCED"));}
        catch(IllegalArgumentException ex){selected=Preset.BALANCED;}
        // Named presets are canonical: an upgrade must not silently keep
        // an obsolete darkness/reach value behind a "Performance" label.
        // Only Custom restores individually persisted fields.
        if(selected!=Preset.CUSTOM)return preset(selected);
        Settings defaults=preset(Preset.BALANCED);
        int cells=PREF.getInt("cellMetres",defaults.cellMetres());
        if(cells!=2&&cells!=4&&cells!=8)cells=defaults.cellMetres();
        return new Settings(selected,
                safeInt("ambient",defaults.ambientPercent(),50,150),
                safeInt("sources",defaults.sourcePercent(),50,150),
                safeInt("reach",defaults.reachPercent(),65,140),
                safeInt("diffusion",defaults.diffusionPercent(),5,100),
                safeInt("flicker",defaults.flickerPercent(),0,200),
                safeInt("cacheBudget",defaults.cacheBudgetPercent(),50,150),
                cells,PREF.getBoolean("distantLod",defaults.distantOutdoorLod()),
                PREF.getBoolean("changedOnly",defaults.changedOnlyVertices()),
                PREF.getBoolean("dependencies",defaults.dependencyReuse()));
    }
    public static Settings current(){return active;}
    /** Applied locally; never replicated to LAN host or serialized into a save. */
    public static synchronized void apply(Settings options){
        Objects.requireNonNull(options,"lighting options");
        if(options.preset()!=Preset.CUSTOM&&!options.equals(preset(options.preset())))
            throw new IllegalArgumentException("Named lighting presets require their canonical values");
        if(options.equals(active))return;
        PREF.put("preset",options.preset().name());
        PREF.putInt("ambient",options.ambientPercent());
        PREF.putInt("sources",options.sourcePercent());
        PREF.putInt("reach",options.reachPercent());
        PREF.putInt("diffusion",options.diffusionPercent());
        PREF.putInt("flicker",options.flickerPercent());
        PREF.putInt("cacheBudget",options.cacheBudgetPercent());
        PREF.putInt("cellMetres",options.cellMetres());
        PREF.putBoolean("distantLod",options.distantOutdoorLod());
        PREF.putBoolean("changedOnly",options.changedOnlyVertices());
        PREF.putBoolean("dependencies",options.dependencyReuse());
        active=options;
    }
}
