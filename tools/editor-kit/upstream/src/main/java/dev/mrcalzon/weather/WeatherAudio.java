package dev.mrcalzon.weather;
import java.util.*;

/** Frame-rate-independent ambient loop crossfades and one-shot sound dispatch.
 * Host owns actual music/sound driver; this core emits named audio gains.
 * Loops are real provided PCM WAVs in audio/; no platform API is assumed.
 */
public final class WeatherAudio {
    public record Loop(String id,double gain){}
    public record Cue(String id,double volume,double x,double y,double z){}
    private final Map<String,Double> gains=new LinkedHashMap<>();
    private final Deque<Cue> oneShots=new ArrayDeque<>();
    private double fadeSeconds=8,master=1,music=.42;
    public synchronized void configure(double fadeSeconds,double masterVolume,double musicVolume){
        if(!Double.isFinite(fadeSeconds)||fadeSeconds<.1||fadeSeconds>600||
           !Double.isFinite(masterVolume)||masterVolume<0||masterVolume>1||
           !Double.isFinite(musicVolume)||musicVolume<0||musicVolume>1)
            throw new IllegalArgumentException("Invalid audio mix");
        this.fadeSeconds=fadeSeconds;master=masterVolume;music=musicVolume;
    }
    private static double approach(double current,double wanted,double amount){
        return current+Math.signum(wanted-current)*Math.min(Math.abs(wanted-current),amount);
    }
    public synchronized void advance(double seconds,WeatherSystem.State state,double daylight){
        if(state==null||seconds<0||seconds>2||!Double.isFinite(seconds)||
           !Double.isFinite(daylight)||daylight<0||daylight>1)
            throw new IllegalArgumentException("Invalid audio update");
        Map<String,Double> wanted=new HashMap<>();
        wanted.put("ambient_rain",state.rain()*master);
        wanted.put("ambient_wind",Math.max(.08,state.windSpeed()/40)*master);
        wanted.put("ambient_snow",state.snow()*.55*master);
        wanted.put("ambient_fog",state.fog()*.45*master);
        wanted.put("ambient_storm",state.rain()>.65?state.rain()*.55*master:0.0);
        wanted.put("music_day",daylight*music*master*(1-state.rain()*.65));
        wanted.put("music_night",(1-daylight)*music*master);
        wanted.put("music_storm",state.rain()>.7?music*master*.85:0.0);
        wanted.put("music_snow",state.snow()>.35?music*master*.60:0.0);
        wanted.put("music_fog",state.fog()>.50?music*master*.48:0.0);
        for(String key:wanted.keySet())gains.put(key,approach(gains.getOrDefault(key,0.),wanted.get(key),seconds/fadeSeconds));
    }
    public synchronized void accept(List<WeatherSystem.Event> events,double observerX,double observerY,double observerZ){
        for(var e:events){
            if(!"SOUND".equals(e.kind()))continue;
            double distance=Math.sqrt(Math.pow(e.x()-observerX,2)+
                Math.pow(e.y()-observerY,2)+Math.pow(e.z()-observerZ,2));
            double attenuation=1/(1+distance/70.);
            oneShots.add(new Cue(distance>500?"thunder_distant":"thunder_near",
                Math.max(0,Math.min(1,master*attenuation)),e.x(),e.y(),e.z()));
        }
    }
    public synchronized List<Loop> loops(){return gains.entrySet().stream().map(e->new Loop(e.getKey(),e.getValue())).toList();}
    public synchronized List<Cue> drainCues(){var out=List.copyOf(oneShots);oneShots.clear();return out;}
}
