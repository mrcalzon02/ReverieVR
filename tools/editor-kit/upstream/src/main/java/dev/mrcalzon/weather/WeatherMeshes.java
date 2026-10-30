package dev.mrcalzon.weather;

import java.util.Set;

/** Optional adapter into the game's actual GLB instance renderer. */
public final class WeatherMeshes {
    private WeatherMeshes() {}
    public record Instance(String meshId,double x,double y,double z,double yaw,double scale,double opacity) {
        public Instance {
            if(meshId==null||meshId.isBlank()||!Double.isFinite(x)||!Double.isFinite(y)||
               !Double.isFinite(z)||!Double.isFinite(yaw)||!Double.isFinite(scale)||
               !Double.isFinite(opacity)||scale<=0||opacity<0||opacity>1)
                throw new IllegalArgumentException("Invalid weather mesh instance");
        }
    }
    @FunctionalInterface public interface Sink {void emit(Instance instance);}
    public static final String[] LIGHTNING_FAMILIES={
        "cloud_to_ground","fork","sheet","horizontal_arc"};
    public static final Set<String> VOLUMES=Set.of("weather_rain_cell","weather_heavy_rain_cell",
        "weather_ground_fog_bank","weather_dense_fog_bank","weather_cloud_bank",
        "weather_storm_cloud_bank");
    public static String lightningFrame(String family,int frame){
        boolean known=false;
        for(String f:LIGHTNING_FAMILIES)if(f.equals(family)){known=true;break;}
        if(!known||frame<0||frame>3)throw new IllegalArgumentException("Unknown lightning mesh frame");
        return "lightning_"+family+"_frame_0"+frame;
    }
}
