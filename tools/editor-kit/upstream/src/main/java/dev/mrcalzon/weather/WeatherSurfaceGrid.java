package dev.mrcalzon.weather;

import java.util.*;

/** Small, host-independent wet-ground and snow-deposition simulation.
 * Register ONLY surfaces inside loaded chunks. The engine owns geometry and
 * sky visibility; covers/roofs MUST return zero exposure when overhead.
 */
public final class WeatherSurfaceGrid {
    public enum Material { SOIL, GRASS, STONE, CONCRETE, METAL, WOOD, ROOF, WATER }
    public record Surface(int id,Material material,double exposure,double wet,double snowMetres) {}
    public record Sample(double wet,double snowMetres,double specular,double traction,int rgb) {}
    private final Map<Integer,Surface> surfaces=new TreeMap<>();
    public void register(int id,Material material,double exposed){
        if(id<0||material==null||!Double.isFinite(exposed)||exposed<0||exposed>1)
            throw new IllegalArgumentException("Invalid surface or sky exposure");
        Surface s=surfaces.get(id);
        surfaces.put(id,new Surface(id,material,exposed,s==null?0:s.wet(),s==null?0:s.snowMetres()));
    }
    public void setExposure(int id,double fraction){
        Surface s=surfaces.get(id);
        if(s==null)throw new IllegalArgumentException("Unknown surface");
        register(id,s.material(),fraction);
    }
    public Surface surface(int id){return surfaces.get(id);}
    public void unregister(int id){surfaces.remove(id);}
    public List<Surface> snapshot(){return List.copyOf(surfaces.values());}
    public void restore(List<Surface> saved){
        if(saved==null||saved.size()>250000)throw new IllegalArgumentException("Invalid save");
        Map<Integer,Surface> checked=new TreeMap<>();
        for(Surface s:saved){
            if(s==null||s.id()<0||s.material()==null||!valid01(s.exposure())||
               !valid01(s.wet())||!Double.isFinite(s.snowMetres())||
               s.snowMetres()<0||s.snowMetres()>4||checked.putIfAbsent(s.id(),s)!=null)
                throw new IllegalArgumentException("Invalid surface snapshot");
        }
        surfaces.clear();surfaces.putAll(checked);
    }
    private static boolean valid01(double d){return Double.isFinite(d)&&d>=0&&d<=1;}
    private static double clamp(double d,double a,double b){return Math.max(a,Math.min(b,d));}
    public void advance(double seconds,WeatherSystem.State weather,double temperatureC,double daylight){
        if(!Double.isFinite(seconds)||seconds<0||seconds>30||weather==null||
           !Double.isFinite(temperatureC)||!valid01(daylight))
            throw new IllegalArgumentException("Invalid surface update");
        for(var s:List.copyOf(surfaces.values())){
            double e=s.exposure();
            double rain=e*weather.rain();
            double snow=e*weather.snow();
            double evap=(temperatureC>0?.00010+.000009*temperatureC:0)*(.3+.7*daylight);
            double wet=clamp(s.wet()+seconds*(rain*.0025-evap),0,1);
            double melt=temperatureC>0?(temperatureC*.000006+rain*.00004)*seconds:0;
            double snowMetres=clamp(s.snowMetres()+seconds*(snow*.000095-melt),0,4);
            if(s.material()==Material.WATER)snowMetres=0;
            surfaces.put(s.id(),new Surface(s.id(),s.material(),e,wet,snowMetres));
        }
    }
    public Sample shade(int id,int baseRgb){
        Surface s=surfaces.get(id);
        if(s==null)return new Sample(0,0,0,1,baseRgb&0xffffff);
        double wet=s.wet(),snow=clamp(s.snowMetres()/.08,0,1);
        double specular=wet*(s.material()==Material.METAL?.80:.38);
        double grip=s.material()==Material.WATER?1:clamp(1-wet*.25-snow*.48,.18,1);
        int r=(baseRgb>>16)&255,g=(baseRgb>>8)&255,b=baseRgb&255;
        double dark=1-.36*wet;
        r=(int)clamp(r*dark*(1-snow)+222*snow,0,255);
        g=(int)clamp(g*dark*(1-snow)+228*snow,0,255);
        b=(int)clamp(b*(dark+.03*wet)*(1-snow)+231*snow,0,255);
        return new Sample(wet,s.snowMetres(),specular,grip,(r<<16)|(g<<8)|b);
    }
}
