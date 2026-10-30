package dev.mrcalzon.weather;

/** Coordinates are host world metres, not a teleport/dimension-specific system. */
public record WeatherView(double x,double y,double z,double heading,double exposed) {
    public WeatherView {
        if(!Double.isFinite(x)||!Double.isFinite(y)||!Double.isFinite(z)||
           !Double.isFinite(heading)||!Double.isFinite(exposed)||exposed<0||exposed>1)
            throw new IllegalArgumentException("Invalid world camera or exposure");
    }
    public static WeatherView outdoors(double x,double y,double z,double heading) {
        return new WeatherView(x,y,z,heading,1);
    }
}
