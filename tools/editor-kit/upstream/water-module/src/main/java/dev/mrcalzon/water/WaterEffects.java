package dev.mrcalzon.water;

import static dev.mrcalzon.water.WaterModule.*;

/** World-space, deterministic PSX-inspired layers. No game or graphics dependency.
 * Scrolling/pixelation and noise-phase waves follow the credited MIT Godot shader;
 * optics/foam and the bounded ray traversal are original Java implementations. */
public record WaterEffects(Layer first,Layer second,double blend,double detail,
                           double amplitude,double grid,double speed,double noiseScale,
                           double foamDepth,double foamOpacity,double refractionPixels,
                           double fresnel,int reflection,ShoreWave shoreline,boolean contactFoamEnabled,boolean sprayEnabled) {
 public WaterEffects(Layer first,Layer second,double blend,double detail,double amplitude,double grid,double speed,double noiseScale,double foamDepth,double foamOpacity,double refractionPixels,double fresnel,int reflection,ShoreWave shoreline){this(first,second,blend,detail,amplitude,grid,speed,noiseScale,foamDepth,foamOpacity,refractionPixels,fresnel,reflection,shoreline,true,true);}
 public WaterEffects(Layer first,Layer second,double blend,double detail,double amplitude,double grid,double speed,double noiseScale,double foamDepth,double foamOpacity,double refractionPixels,double fresnel,int reflection){this(first,second,blend,detail,amplitude,grid,speed,noiseScale,foamDepth,foamOpacity,refractionPixels,fresnel,reflection,null);}
 public record ShoreWave(Bathymetry bed,double level,double amplitude,double period,double wavelength,double slope){
  public ShoreWave{if(bed==null||!Double.isFinite(level+amplitude+period+wavelength+slope)||amplitude<0||amplitude>.08||period<2||wavelength<.5||slope<=0)throw new IllegalArgumentException("Shore waves");}
  double depth(double x,double z){return Math.max(0,level-bed.bottomY(x,z));}
  double phase(double x,double z,double time){return Math.PI*2*(depth(x,z)/slope/wavelength-time/period);}
  public double height(double x,double z,double time){double d=depth(x,z);return Double.isFinite(d)?amplitude*Math.sin(phase(x,z,time))*Math.exp(-d/.75):0;}
 }
 public double envelope(){return amplitude+(shoreline==null?0:shoreline.amplitude());}
 public double foamPattern(double x,double z,double time,double depth){double breakup=Math.max(0,(noise(x*3+time*.08,z*3-time*.11)-.3)/.7);if(shoreline==null)return breakup;double crest=Math.pow(Math.max(0,Math.sin(shoreline.phase(x,z,time))),3);return breakup*(.25+.75*crest);}

 public record Layer(Texture texture,double scaleX,double scaleZ,double scrollX,double scrollZ,int pixels){
  public Layer{if(texture==null||!Double.isFinite(scaleX+scaleZ+scrollX+scrollZ)||scaleX<=0||scaleZ<=0||pixels<2||pixels>4096)throw new IllegalArgumentException("Bad water layer");}
  public int sample(double x,double z,double time){double u=x*scaleX+time*scrollX,v=z*scaleZ+time*scrollZ;return texture.sample(Math.floor(u*pixels)/pixels,Math.floor(v*pixels)/pixels);}
 }
 public WaterEffects{
  if(first==null||second==null||!Double.isFinite(blend+detail+amplitude+grid+speed+noiseScale+foamDepth+foamOpacity+refractionPixels+fresnel)||blend<0||blend>1||detail<0||detail>1||amplitude<0||amplitude>.5||grid<.25||grid>8||noiseScale<=0||foamDepth<=0||foamOpacity<0||foamOpacity>1||refractionPixels<0||refractionPixels>12||fresnel<0||fresnel>1)throw new IllegalArgumentException("Bad water effects");
 }
 /** Repeatable coherent value noise; equivalent purpose to the source noise map,
  * deliberately not advertised as a FastNoiseLite-compatible implementation. */
 public static double noise(double x,double z){int ix=(int)Math.floor(x),iz=(int)Math.floor(z);double u=x-ix,v=z-iz;u=u*u*(3-2*u);v=v*v*(3-2*v);return lerp(lerp(hash(ix,iz),hash(ix+1,iz),u),lerp(hash(ix,iz+1),hash(ix+1,iz+1),u),v);}
 private static double hash(int x,int z){int h=x*374761393+z*668265263+1274126177;h=(h^(h>>>13))*1274126177;return ((h^(h>>>16))&0xffffff)/16777215.0;}
 private static double lerp(double a,double b,double t){return a+(b-a)*t;}
 public double vertex(double x,double z,double time){return (.72*Math.sin(x*.2+z*.2+time*speed+noise(x*noiseScale,z*noiseScale)*10)+.28*Math.sin(x*1.1+z*.75-time*speed*1.3+noise(x*.14,z*.14)*3))*amplitude+(shoreline==null?0:shoreline.height(x,z,time));}
 public record Surface(double offset,double slopeX,double slopeZ){}
 public Surface surface(double x,double z,double time){
  double x0=Math.floor(x/grid)*grid,z0=Math.floor(z/grid)*grid,u=(x-x0)/grid,v=(z-z0)/grid;
  double a=vertex(x0,z0,time),b=vertex(x0+grid,z0,time),c=vertex(x0+grid,z0+grid,time),d=vertex(x0,z0+grid,time);
  double sx=v<=u?(b-a)/grid:(c-d)/grid,sz=v<=u?(c-b)/grid:(d-a)/grid;
  return new Surface(a+sx*(x-x0)+sz*(z-z0),sx,sz);
 }
 /** First intersection with piecewise planar wave triangles. Traverse grid cells
  * only inside the finite displacement envelope; no ambiguous grazing root guess. */
 public double intersect(Camera c,double dx,double dy,double dz,double level,double time,double limit){
  if(Math.abs(dy)<1e-10)return Double.NaN;
  double a=(level-envelope()-c.y())/dy,b=(level+envelope()-c.y())/dy,lo=Math.max(.065,Math.min(a,b)-1e-7),hi=Math.min(limit,Math.max(a,b)+1e-7);
  if(lo>hi)return Double.NaN;
  for(int cell=0;cell<2048&&lo<=hi;cell++){
   double x=c.x()+dx*(lo+1e-8),z=c.z()+dz*(lo+1e-8),x0=Math.floor(x/grid)*grid,z0=Math.floor(z/grid)*grid;
   double tx=Math.abs(dx)<1e-12?Double.POSITIVE_INFINITY:((dx>0?x0+grid:x0)-c.x())/dx;
   double tz=Math.abs(dz)<1e-12?Double.POSITIVE_INFINITY:((dz>0?z0+grid:z0)-c.z())/dz,end=Math.min(hi,Math.min(tx,tz));
   double va=vertex(x0,z0,time),vb=vertex(x0+grid,z0,time),vc=vertex(x0+grid,z0+grid,time),vd=vertex(x0,z0+grid,time),best=Double.POSITIVE_INFINITY;
   for(int tri=0;tri<2;tri++){
    double sx=tri==0?(vb-va)/grid:(vc-vd)/grid,sz=tri==0?(vc-vb)/grid:(vd-va)/grid,den=dy-sx*dx-sz*dz;
    if(Math.abs(den)<1e-12)continue;
    double t=(level+va+sx*(c.x()-x0)+sz*(c.z()-z0)-c.y())/den;
    double u=c.x()+dx*t-x0,v=c.z()+dz*t-z0;
    if(t>=lo-1e-7&&t<=end+1e-7&&u>=-1e-6&&u<=grid+1e-6&&v>=-1e-6&&v<=grid+1e-6&&(tri==0?v<=u+1e-7:v>=u-1e-7))best=Math.min(best,t);
   }
   if(Double.isFinite(best))return best;
   if(end>=hi)break;lo=end+1e-7;
  }return Double.NaN;
 }
}
