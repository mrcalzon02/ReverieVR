package dev.mrcalzon.weather;

import java.awt.Graphics2D;
import java.util.*;

/** One-call standalone Java 17 weather facade. Independent of game engine.
 * A host may render sky + overlay with Graphics2D and place optional GLB
 * model instances through its own triangle renderer. No network coupling.
 */
public final class WeatherModule {
    private final long seed;
    private final WeatherSystem atmosphere;
    private final WeatherSurfaceGrid surfaces=new WeatherSurfaceGrid();
    private final WeatherAudio audio=new WeatherAudio();
    private WeatherView viewer=WeatherView.outdoors(0,0,1.5,0);
    private double timeOfDay=.48,daySeconds=1200,temperatureC=12;
    private double autoInterval=240,autoVariance=.55,autoTransition=75;
    private boolean autoWeather=false,clockRunning=true;
    private long weatherSequence=0;
    private double secondsUntilChange=autoInterval;
    private final List<WeatherType> repertoire=new ArrayList<>(List.of(
        WeatherType.CLEAR,WeatherType.OVERCAST,WeatherType.DRIZZLE,
        WeatherType.RAIN,WeatherType.HEAVY_RAIN,WeatherType.THUNDERSTORM,
        WeatherType.FOG,WeatherType.SNOW));
    private double lightningHeading=0;
    public WeatherModule(long seed){this.seed=seed;this.atmosphere=new WeatherSystem(seed);}
    public WeatherSystem weather(){return atmosphere;}
    public WeatherSurfaceGrid surfaces(){return surfaces;}
    public WeatherAudio audio(){return audio;}
    public WeatherSky.State sky(){return WeatherSky.state(timeOfDay,atmosphere.state());}
    public double timeOfDay(){return timeOfDay;}
    public void setTimeOfDay(double fraction){
        if(!Double.isFinite(fraction)||fraction<0||fraction>=1)
            throw new IllegalArgumentException("Day fraction 0..1 exclusive");
        timeOfDay=fraction;
    }
    public void setDayLength(double seconds){
        if(!Double.isFinite(seconds)||seconds<60||seconds>864000)
            throw new IllegalArgumentException("Day length 60..864000 seconds");
        daySeconds=seconds;
    }
    public void pauseClock(boolean pause){clockRunning=!pause;}
    public void setTemperature(double celsius){
        if(!Double.isFinite(celsius)||celsius< -100||celsius>100)
            throw new IllegalArgumentException("Invalid temperature");
        temperatureC=celsius;
    }
    public void setWeather(WeatherType type,double fadeSeconds){atmosphere.setWeather(type,fadeSeconds);}
    public void setAutoWeather(boolean enabled,double secondsBetween,double randomness,double transitionSeconds){
        if(!Double.isFinite(secondsBetween)||secondsBetween<5||secondsBetween>86400||
           !Double.isFinite(randomness)||randomness<0||randomness>1||
           !Double.isFinite(transitionSeconds)||transitionSeconds<0||transitionSeconds>3600)
            throw new IllegalArgumentException("Invalid auto-weather controls");
        autoWeather=enabled;autoInterval=secondsBetween;autoVariance=randomness;
        autoTransition=transitionSeconds;secondsUntilChange=secondsBetween;
    }
    public void allowWeather(Collection<WeatherType> types){
        if(types==null||types.isEmpty()||types.size()>WeatherType.values().length||
           types.stream().anyMatch(Objects::isNull))
            throw new IllegalArgumentException("Invalid weather selection");
        repertoire.clear();repertoire.addAll(new LinkedHashSet<>(types));
    }
    private long random(long input){
        long z=input+0x9e3779b97f4a7c15L;
        z=(z^(z>>>30))*0xbf58476d1ce4e5b9L;
        z=(z^(z>>>27))*0x94d049bb133111ebL;
        return z^(z>>>31);
    }
    public void advance(double seconds,WeatherView camera){
        if(!Double.isFinite(seconds)||seconds<0||seconds>1||camera==null)
            throw new IllegalArgumentException("Call once per fixed tick <= 1 second");
        viewer=camera;atmosphere.setObserver(camera.x(),camera.y(),camera.z());
        if(clockRunning)timeOfDay=(timeOfDay+seconds/daySeconds)%1;
        if(autoWeather){
            secondsUntilChange-=seconds;
            if(secondsUntilChange<=0){
                long n=random(seed^++weatherSequence);
                int index=(int)Math.floorMod(n,repertoire.size());
                atmosphere.setWeather(repertoire.get(index),autoTransition);
                double change=(n>>>16&0xffff)/65535.0*2-1;
                secondsUntilChange=autoInterval*(1+change*autoVariance);
            }
        }
        atmosphere.advance(seconds);
        surfaces.advance(seconds,atmosphere.state(),temperatureC,sky().daylight());
        audio.advance(seconds,atmosphere.state(),sky().daylight());
        var events=atmosphere.drainEvents();
        audio.accept(events,camera.x(),camera.y(),camera.z());
        for(var ev:events)if("VISUAL".equals(ev.kind()))
            lightningHeading=Math.atan2(ev.y()-camera.y(),ev.x()-camera.x());
    }
    public void renderSky(Graphics2D g,int w,int h){
        WeatherSky.paint(g,w,h,sky(),seed,atmosphere.state().windHeading(),
            atmosphere.state().elapsedMillis()/1000.,viewer.heading(),lightningHeading);
    }
    /** Camera-aware sky painting; geometry stays anchored to world directions. */
    public void renderSky(Graphics2D g,int w,int h,double pitch,double focal){
        WeatherSky.paint(g,w,h,sky(),seed,atmosphere.state().windHeading(),
            atmosphere.state().elapsedMillis()/1000.,viewer.heading(),lightningHeading,
            pitch,focal);
    }
    public void renderOverlay(Graphics2D g,int w,int h){atmosphere.renderOverlay(g,w,h,viewer);}
    public void renderOverlay(Graphics2D g,int w,int h,java.util.function.BiPredicate<Integer,Integer> outdoorPixel){
        atmosphere.renderOverlay(g,w,h,viewer,outdoorPixel);
    }
    public void emitMeshes(WeatherMeshes.Sink sink){atmosphere.emitMeshes(viewer,sink);}
    public record Save(double timeOfDay,WeatherType type,double temperatureC,List<WeatherSurfaceGrid.Surface> surfaces){}
    public Save snapshot(){return new Save(timeOfDay,atmosphere.weather(),temperatureC,surfaces.snapshot());}
    public void restore(Save save){
        if(save==null)throw new IllegalArgumentException("Null weather save");
        setTimeOfDay(save.timeOfDay());setTemperature(save.temperatureC());
        surfaces.restore(save.surfaces());setWeather(save.type(),0);
    }
}
