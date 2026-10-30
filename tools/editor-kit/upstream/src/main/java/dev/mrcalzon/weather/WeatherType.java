package dev.mrcalzon.weather;

/** Stable cross-game weather IDs. Engine-neutral: never implies gameplay damage. */
public enum WeatherType {
    CLEAR(0, 0, 0, 0, 0, 1),
    OVERCAST(0, 0, .12, 0, 0, .86),
    DRIZZLE(.17, 0, .20, 0, 0, .83),
    RAIN(.52, 0, .22, 0, 0, .71),
    HEAVY_RAIN(1, 0, .44, 0, 0, .49),
    THUNDERSTORM(.94, 0, .53, 0, 0, .44),
    FOG(0, 0, .59, 0, 0, .44),
    DENSE_FOG(0, 0, .89, 0, 0, .19),
    SNOW(0, .50, .30, 0, 0, .70),
    BLIZZARD(0, 1, .76, 0, 0, .24),
    SLEET(.44, .54, .31, 0, 0, .55),
    HAIL(.73, .16, .26, 0, 0, .58),
    DUST_STORM(0, 0, .77, .91, 0, .26),
    ASHFALL(0, .26, .48, 0, 1, .50),
    MIASMA(0, 0, .92, 0, .72, .17);

    public final double rain, snow, fog, dust, ash, visibility;
    WeatherType(double rain, double snow, double fog, double dust, double ash, double visibility) {
        this.rain=rain;this.snow=snow;this.fog=fog;this.dust=dust;this.ash=ash;this.visibility=visibility;
    }
    public boolean storm(){return this==THUNDERSTORM;}
}
