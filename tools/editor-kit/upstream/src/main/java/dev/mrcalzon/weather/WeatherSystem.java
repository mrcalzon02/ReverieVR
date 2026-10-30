package dev.mrcalzon.weather;

import java.awt.*;
import java.awt.geom.*;
import java.util.*;
import java.util.List;

/**
 * Portable Java 17 procedural weather controller for PSX/software-rendered games.
 * No threads, external dependencies, per-project gameplay assumptions or
 * remote services. Call once per fixed simulation tick and once per render.
 * Mesh emissions are OPTIONAL: the host maps IDs to validated shared GLBs.
 * Gameplay (lightning damage, cold, wet ground, city air-denial) is NEVER
 * client-controlled by this visual module; host consumes state explicitly.
 */
public final class WeatherSystem {
    public record State(WeatherType type,double rain,double snow,double fog,double dust,
                        double ash,double visibility,double wetness,double windSpeed,
                        double windHeading,double flash,long elapsedMillis) {}
    public record Event(String kind,String soundId,double x,double y,double z,long atMillis) {}
    public enum StrikeMode { FULL, MESH_ONLY, FLASH_ONLY, THUNDER_ONLY }
    private record Strike(String family,double x,double y,double z,double yaw,
                          long startedMillis,StrikeMode mode) {}
    private record DelayedThunder(long atMillis,double x,double y,double z) {}
    private final long seed;
    private WeatherType current=WeatherType.CLEAR,target=WeatherType.CLEAR;
    private double currentRain,currentSnow,currentFog,currentDust,currentAsh,currentVisibility=1;
    private double startRain,startSnow,startFog,startDust,startAsh,startVisibility=1;
    private double windSpeed=.28,windHeading=1.8,wetness;
    private double observerX,observerY,observerZ;
    private double timeMillis,transitionStart,transitionDuration;
    private final List<Strike> strikes=new ArrayList<>();
    private final List<DelayedThunder> delayed=new ArrayList<>();
    private final ArrayDeque<Event> events=new ArrayDeque<>();
    private long nextStormStrike=0,sequence=0;
    private boolean autoLightning=true,flashesEnabled=true;
    private double maxFlash=.12;
    private static final int[] STRIKE_SEQUENCE={0,2,1,3,2};
    private static final int[] STRIKE_FRAME_MS={33,61,42,34,68};
    private static final int TOTAL_STRIKE_MS=238;
    private static final int SOFT_ILLUMINATION_MS=900;
    public WeatherSystem(long seed){this.seed=seed;}
    public WeatherType weather(){return target;}
    public void setWeather(WeatherType type,double seconds){
        if(type==null||!Double.isFinite(seconds)||seconds<0||seconds>3600)
            throw new IllegalArgumentException("Invalid weather transition");
        startRain=currentRain;startSnow=currentSnow;startFog=currentFog;
        startDust=currentDust;startAsh=currentAsh;startVisibility=currentVisibility;
        target=type;transitionStart=timeMillis;transitionDuration=seconds*1000;
        if(seconds==0)blend(1);
    }
    public void setWind(double speed,double heading){
        if(!Double.isFinite(speed)||speed<0||speed>60||!Double.isFinite(heading))
            throw new IllegalArgumentException("Invalid wind");
        windSpeed=speed;windHeading=heading;
    }
    public void setObserver(double x,double y,double z){
        if(!Double.isFinite(x)||!Double.isFinite(y)||!Double.isFinite(z))
            throw new IllegalArgumentException("Invalid observer position");
        observerX=x;observerY=y;observerZ=z;
    }
    public void strikeAt(double x,double y,double z,String family,StrikeMode mode){
        double distance=Math.sqrt(Math.pow(x-observerX,2)+Math.pow(y-observerY,2)+
            Math.pow(z-observerZ,2));
        strike(x,y,z,distance,family,mode);
    }
    public void strikeRelative(double distance,double bearing,double height,String family,
                               StrikeMode mode){
        if(!Double.isFinite(distance)||distance<0||distance>40000||
           !Double.isFinite(bearing)||!Double.isFinite(height))
            throw new IllegalArgumentException("Invalid relative strike");
        strikeAt(observerX+distance*Math.cos(bearing),
            observerY+height,observerZ+distance*Math.sin(bearing),family,mode);
    }
    public void setAutoLightning(boolean enabled){autoLightning=enabled;}
    public void setFlashesEnabled(boolean enabled){flashesEnabled=enabled;}
    public void setMaxFlash(double intensity){
        if(!Double.isFinite(intensity)||intensity<0||intensity>.3)
            throw new IllegalArgumentException("Flash cap 0..0.3");
        maxFlash=intensity;
    }
    public void advance(double deltaSeconds){
        if(!Double.isFinite(deltaSeconds)||deltaSeconds<0||deltaSeconds>1)
            throw new IllegalArgumentException("Use bounded fixed ticks <= 1s");
        timeMillis+=deltaSeconds*1000;
        double portion=transitionDuration==0?1:Math.min(1,Math.max(0,
            (timeMillis-transitionStart)/transitionDuration));
        blend(portion*portion*(3-2*portion));
        if(portion>=1)current=target;
        wetness=Math.max(0,Math.min(1,wetness+deltaSeconds*(currentRain*.013-currentRain*.001-.003)));
        long now=(long)timeMillis;
        strikes.removeIf(s->now-s.startedMillis>SOFT_ILLUMINATION_MS);
        for(Iterator<DelayedThunder> it=delayed.iterator();it.hasNext();){
            DelayedThunder scheduled=it.next();
            if(now>=scheduled.atMillis()){
                events.add(new Event("SOUND","thunder",scheduled.x(),scheduled.y(),scheduled.z(),now));
                it.remove();
            }
        }
        if(autoLightning&&target.storm()&&currentRain>.5&&now>=nextStormStrike){
            long r=hash(seed^++sequence);double bearing=(r&65535)/65535.0*Math.PI*2;
            double dist=18+((r>>>16)&65535)/65535.0*75;
            strikeRelative(dist,bearing,18+((r>>>32)&7),
                WeatherMeshes.LIGHTNING_FAMILIES[(int)((r>>>39)&3)],StrikeMode.FULL);
            nextStormStrike=now+3500+((r>>>44)&8191);
        }
    }
    private void blend(double t){
        currentRain=mix(startRain,target.rain,t);currentSnow=mix(startSnow,target.snow,t);
        currentFog=mix(startFog,target.fog,t);currentDust=mix(startDust,target.dust,t);
        currentAsh=mix(startAsh,target.ash,t);
        currentVisibility=mix(startVisibility,target.visibility,t);
    }
    private static double mix(double a,double b,double t){return a+(b-a)*t;}
    public void strike(double x,double y,double z,double observerDistance,String family){
        strike(x,y,z,observerDistance,family,StrikeMode.FULL);
    }
    public void strike(double x,double y,double z,double observerDistance,String family,
                       StrikeMode mode){
        if(mode==null)throw new IllegalArgumentException("Missing lightning mode");
        if(!Double.isFinite(x)||!Double.isFinite(y)||!Double.isFinite(z)||
           !Double.isFinite(observerDistance)||observerDistance<0||observerDistance>40000)
            throw new IllegalArgumentException("Invalid strike coordinate or distance");
        WeatherMeshes.lightningFrame(family,0);
        long now=(long)timeMillis;
        long h=hash(seed^++sequence^Double.doubleToRawLongBits(x));
        strikes.add(new Strike(family,x,y,z,(h&65535)*Math.PI*2/65535.,now,mode));
        if(mode==StrikeMode.FULL||mode==StrikeMode.THUNDER_ONLY)
            delayed.add(new DelayedThunder(now+Math.round(observerDistance/343.*1000),x,y,z));
        if(mode!=StrikeMode.THUNDER_ONLY)
            events.add(new Event("VISUAL","lightning",x,y,z,now));
    }
    public State state(){
        double flash=0;long now=(long)timeMillis;
        for(Strike s:strikes){long age=now-s.startedMillis;
            if((s.mode()==StrikeMode.FULL||s.mode()==StrikeMode.FLASH_ONLY)&&
               age>=0&&age<SOFT_ILLUMINATION_MS){
                double v=age<200?age/200.:1-(age-200)/700.;
                flash=Math.max(flash,v*v*(3-2*v));
            }
        }
        return new State(target,currentRain,currentSnow,currentFog,currentDust,currentAsh,
            currentVisibility,wetness,windSpeed,windHeading,
            flashesEnabled?Math.min(maxFlash,flash*maxFlash):0,(long)timeMillis);
    }
    public List<Event> drainEvents(){
        var copy=List.copyOf(events);events.clear();return copy;
    }
    public void emitMeshes(WeatherView view,WeatherMeshes.Sink sink){
        Objects.requireNonNull(view);Objects.requireNonNull(sink);
        if(currentRain>.05){
            String id=currentRain>.69?"weather_heavy_rain_cell":"weather_rain_cell";
            for(int i=0;i<(currentRain>.7?4:2);i++){
                long h=hash(seed+i*91L+(long)timeMillis/120);
                double x=view.x()+((h&255)-128)/10.;
                double y=view.y()+(((h>>>8)&255)-128)/10.;
                sink.emit(new WeatherMeshes.Instance(id,x,y,view.z()+2.5,
                    windHeading,1.0,currentRain*view.exposed()*.33));
            }
        }
        // Fog is a feature of the MAP, not a billboard tied to the player.
        // Stable, seeded world cells remain stationary while the observer moves.
        // Only nearby cells are emitted to bound cost. Z is world height.
        if(currentFog>.12){
            String id=currentFog>.70?"weather_dense_fog_bank":"weather_ground_fog_bank";
            final double spacing=28.0;
            long cx=(long)Math.floor(view.x()/spacing);
            long cy=(long)Math.floor(view.y()/spacing);
            for(long dx=-1;dx<=1;dx++)for(long dy=-1;dy<=1;dy++){
                long gx=cx+dx,gy=cy+dy;
                long h=hash(seed^(gx*0x9e3779b97f4a7c15L)^(gy*0xbf58476d1ce4e5b9L));
                double x=(gx+.5)*spacing+(((h&255)/255.0)-.5)*8;
                double y=(gy+.5)*spacing+((((h>>>8)&255)/255.0)-.5)*8;
                double yaw=windHeading+(((h>>>16)&255)/255.0-.5)*.25;
                sink.emit(new WeatherMeshes.Instance(id,x,y,1.0,
                    yaw,2.5,currentFog*.24*view.exposed()));
            }
        }
        // Cloud banks share the same stable world reference; they do not
        // translate with the camera as the player crosses the investigation.
        if(currentRain>.72||target==WeatherType.OVERCAST){
            final double spacing=85.0;
            long cx=(long)Math.floor(view.x()/spacing);
            long cy=(long)Math.floor(view.y()/spacing);
            for(long dx=-1;dx<=1;dx++)for(long dy=-1;dy<=1;dy++)
                sink.emit(new WeatherMeshes.Instance(
                    currentRain>.72?"weather_storm_cloud_bank":"weather_cloud_bank",
                    (cx+dx+.5)*spacing,(cy+dy+.5)*spacing,24,
                    windHeading,3.5,.45));
        }
        for(Strike s:strikes){
            if(s.mode()==StrikeMode.THUNDER_ONLY||s.mode()==StrikeMode.FLASH_ONLY)continue;
            long age=(long)timeMillis-s.startedMillis();int frame=frameIndex(age);
            if(frame<0)continue;
            String model=WeatherMeshes.lightningFrame(s.family(),STRIKE_SEQUENCE[frame]);
            sink.emit(new WeatherMeshes.Instance(model,s.x(),s.y(),s.z(),s.yaw(),1,1));
        }
    }
    private static int frameIndex(long age){
        if(age<0||age>=TOTAL_STRIKE_MS)return -1;
        for(int i=0;i<STRIKE_FRAME_MS.length;i++){
            age-=STRIKE_FRAME_MS[i];if(age<0)return i;
        }
        return -1;
    }
    public void renderOverlay(Graphics2D targetG,int width,int height,WeatherView view){
        renderOverlay(targetG,width,height,view,(x,y)->true);
    }
    public void renderOverlay(Graphics2D targetG,int width,int height,WeatherView view,
                              java.util.function.BiPredicate<Integer,Integer> outdoorPixel){
        if(outdoorPixel==null)throw new IllegalArgumentException("Null sky visibility mask");
        if(targetG==null||view==null||width<=0||height<=0||width>8192||height>8192)
            throw new IllegalArgumentException("Invalid graphics viewport");
        Graphics2D g=(Graphics2D)targetG.create();
        try{
            g.setClip(0,0,width,height);
            long tick=(long)(timeMillis/24);
            double exposure=view.exposed();
            if(currentFog>0.01){
                int opacity=(int)Math.min(180,35+currentFog*135);
                g.setColor(target==WeatherType.MIASMA?new Color(72,38,105,opacity):
                    target==WeatherType.DUST_STORM?new Color(127,104,64,opacity):
                    new Color(106,114,124,opacity));
                g.fillRect(0,0,width,height);
            }
            int count=(int)Math.min(420,Math.max(0,(currentRain*250+currentSnow*180+
                currentDust*130+currentAsh*130)*exposure));
            for(int i=0;i<count;i++){
                long h=hash(seed+i*0x9e3779b97f4a7c15L);
                double speed=target==WeatherType.BLIZZARD?2.2:(currentRain>.4?6.2:2.0);
                int x=(int)Math.floorMod((long)((h&65535)+tick*windSpeed*.95),width);
                int y=(int)Math.floorMod((long)(((h>>>16)&65535)+tick*speed),height);
                if(!outdoorPixel.test(x,y))continue;
                if(currentRain>=currentSnow&&currentRain>0){
                    g.setColor(new Color(160,187,206,(int)(55+currentRain*110)));
                    g.drawLine(x,y,(int)(x+Math.cos(windHeading)*3),y+(currentRain>.75?12:6));
                }else if(currentSnow>0){
                    g.setColor(new Color(216,220,222,(int)(75+currentSnow*135)));
                    g.fillRect(x,y,1+(int)((h>>>32)&1),2);
                }else{
                    g.setColor(target==WeatherType.ASHFALL?
                        new Color(108,105,108,125):new Color(176,136,88,90));
                    g.fillRect(x,y,2,1);
                }
            }
            if(state().flash()>0){
                g.setColor(new Color(210,224,240,(int)(state().flash()*255)));
                g.fillRect(0,0,width,height);
            }
        }finally{g.dispose();}
    }
    private static long hash(long z){
        z=(z^(z>>>30))*0xbf58476d1ce4e5b9L;
        z=(z^(z>>>27))*0x94d049bb133111ebL;
        return z^(z>>>31);
    }
}
