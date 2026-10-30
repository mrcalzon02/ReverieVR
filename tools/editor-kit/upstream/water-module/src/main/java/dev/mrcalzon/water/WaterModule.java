package dev.mrcalzon.water;

import java.util.*;

/** Dependency-free water layer. World metres, Y-up, +Z camera forward at yaw0.
 * Opaque depth is camera-forward distance, not Euclidean range or hardware Z.
 * Draw after opaque world readback and before above-water sprites/weather.
 * Surface topology, bathymetry and dry hull volumes are authored input.
 */
public final class WaterModule {
 public record Point(double x,double z){public Point{if(!Double.isFinite(x+z))throw new IllegalArgumentException("Bad water point");}}
 public static final class Polygon {
  private final List<Point> points;private final double minX,maxX,minZ,maxZ;
  public List<Point> points(){return points;}
  public Polygon(List<Point> input){points=List.copyOf(input);minX=points.stream().mapToDouble(Point::x).min().orElse(0);maxX=points.stream().mapToDouble(Point::x).max().orElse(0);minZ=points.stream().mapToDouble(Point::z).min().orElse(0);maxZ=points.stream().mapToDouble(Point::z).max().orElse(0);if(points.size()<3||points.size()>2048)throw new IllegalArgumentException("Bad water polygon");double area=0;for(int i=0;i<points.size();i++){var a=points.get(i);var b=points.get((i+1)%points.size());area+=a.x*b.z-b.x*a.z;}if(Math.abs(area)<1e-8)throw new IllegalArgumentException("Zero-area water polygon");}
  /** Includes edges: a dry-volume boundary must not leak a one-pixel water seam. */
  public boolean contains(double x,double z){
   if(x<minX-1e-7||x>maxX+1e-7||z<minZ-1e-7||z>maxZ+1e-7)return false;boolean inside=false;
   for(int i=0,j=points.size()-1;i<points.size();j=i++){
    var a=points.get(j);var b=points.get(i);double dx=b.x-a.x,dz=b.z-a.z,length=dx*dx+dz*dz;
    double t=length==0?0:Math.max(0,Math.min(1,((x-a.x)*dx+(z-a.z)*dz)/length));
    if(Math.hypot(x-a.x-t*dx,z-a.z-t*dz)<1e-7)return true;
    if((a.z>z)!=(b.z>z)&&x<(b.x-a.x)*(z-a.z)/(b.z-a.z)+a.x)inside=!inside;
   }return inside;
  }
 }
 /** Dry volume follows the actual hull interior, not its rectangular outer bounds. */
 public interface Exclusion {boolean excludes(double x,double y,double z);}
 public record DryVolume(String id,Polygon interior,double bottom,double top) implements Exclusion {
  public DryVolume{Objects.requireNonNull(id);Objects.requireNonNull(interior);if(!Double.isFinite(bottom+top)||top<=bottom)throw new IllegalArgumentException("Bad dry hull volume");}
  public boolean excludes(double x,double y,double z){return y>=bottom&&y<=top&&interior.contains(x,z);}
 }
 public interface Bathymetry {double bottomY(double x,double z);}
 /** Periodic RGB diffuse samples with explicit repeat; caller can supply a PNG. */
 public record Texture(int width,int height,int[] rgb){
  public Texture{if(width<2||height<2||width>2048||height>2048||rgb.length!=width*height)throw new IllegalArgumentException("Bad water texture");rgb=rgb.clone();}
  @Override public int[] rgb(){return rgb.clone();}
  public int sample(double u,double v){int x=(int)Math.floor((u-Math.floor(u))*width),z=(int)Math.floor((v-Math.floor(v))*height);return rgb[z*width+x]&0xffffff;}
 }
 public record Style(int shallow,int deep,double absorption,double minimumOpacity,double maximumOpacity,double tileMetres,double drift){
  public Style{if(!Double.isFinite(absorption+minimumOpacity+maximumOpacity+tileMetres+drift)||absorption<=0||minimumOpacity<0||maximumOpacity>1||minimumOpacity>maximumOpacity||tileMetres<=0)throw new IllegalArgumentException("Bad water style");}
 }
 public record Body(String id,Polygon surface,double y,Bathymetry bed,Texture texture,Style style,List<? extends Exclusion> dry,WaterEffects effects,String seamKey){
  public Body(String id,Polygon surface,double y,Bathymetry bed,Texture texture,Style style,List<? extends Exclusion> dry,WaterEffects effects){this(id,surface,y,bed,texture,style,dry,effects,null);}
  public Body(String id,Polygon surface,double y,Bathymetry bed,Texture texture,Style style,List<? extends Exclusion> dry){this(id,surface,y,bed,texture,style,dry,null);}
  public Body{Objects.requireNonNull(id);Objects.requireNonNull(surface);Objects.requireNonNull(bed);Objects.requireNonNull(texture);Objects.requireNonNull(style);dry=List.copyOf(dry);if(!Double.isFinite(y))throw new IllegalArgumentException("Bad water elevation");}
  public boolean wet(double x,double z){if(!surface.contains(x,z))return false;double bottom=bed.bottomY(x,z);if(!Double.isFinite(bottom)||bottom>=y)return false;for(var d:dry)if(d.excludes(x,y,z))return false;return true;}
  public double depth(double x,double z){return wet(x,z)?Math.max(0,y-bed.bottomY(x,z)):0;}
 }
 public record Camera(double x,double y,double z,double yaw,double pitch,double focal,int width,int height){
  public Camera{if(!Double.isFinite(x+y+z+yaw+pitch+focal)||focal<=0||width<1||height<1||(long)width*height>2073600)throw new IllegalArgumentException("Bad water camera");}
 }
 public interface Lighting {int shade(int rgb,double x,double y,double z);}
 public record Statistics(int pixels,int excludedHullSamples,int opaqueOccludedSamples,int contactFoamPixels,int sprayPixels){public Statistics(int pixels,int excludedHullSamples,int opaqueOccludedSamples){this(pixels,excludedHullSamples,opaqueOccludedSamples,0,0);}}
 private record Hit(Body body,double depth,double x,double y,double z){}
 private final List<Body> bodies;
 public WaterModule(List<Body> bodies){this.bodies=List.copyOf(bodies);if(bodies.size()>64)throw new IllegalArgumentException("Too many water bodies");}
 public List<Body> bodies(){return bodies;}
 private static int scale(int a,double v){int rgb=0;for(int shift:new int[]{16,8,0})rgb|=(int)Math.max(0,Math.min(255,Math.round(((a>>shift)&255)*v)))<<shift;return rgb;}
 private static int mix(int a,int b,double t){int rgb=0;for(int shift:new int[]{16,8,0})rgb|=(int)Math.round(((a>>shift)&255)*(1-t)+((b>>shift)&255)*t)<<shift;return rgb;}
 /** Depth-aware transparency with far-to-near layering; no terrain/actor mutation. */
 public Statistics composite(Camera c,int[] opaque,double[] depth,double seconds){return composite(c,opaque,depth,seconds,(rgb,x,y,z)->rgb);}
 public Statistics composite(Camera c,int[] opaque,double[] depth,double seconds,Lighting lighting){
  Objects.requireNonNull(lighting);
  if(opaque.length!=c.width*c.height||depth.length!=opaque.length||!Double.isFinite(seconds))throw new IllegalArgumentException("Bad water frame");
  double cy=Math.cos(c.yaw),sy=Math.sin(c.yaw),cp=Math.cos(c.pitch),sp=Math.sin(c.pitch);int drawn=0,excluded=0,occluded=0;
  var hits=new ArrayList<Hit>(bodies.size());
  boolean optical=bodies.stream().anyMatch(b->b.effects!=null);int[] original=optical?opaque.clone():opaque;double[] originalDepth=optical?depth.clone():depth;
  var contacts=optical?new WaterContact(c,originalDepth):null;var spray=new LinkedHashMap<Long,WaterSpray.Emitter>();int contactPixels=0;
  for(int py=0;py<c.height;py++)for(int px=0;px<c.width;px++){
   int index=py*c.width+px;double screenX=(px+.5-c.width*.5)/c.focal,screenY=(c.height*.5-py-.5)/c.focal;
   double dy=screenY*cp+sp,forward=cp-screenY*sp,dx=-screenX*cy+forward*sy,dz=screenX*sy+forward*cy;
   if(Math.abs(dy)<1e-10)continue;hits.clear();
   for(var body:bodies){
    double t=body.effects==null?(body.y-c.y)/dy:body.effects.intersect(c,dx,dy,dz,body.y,seconds,Math.min(180,originalDepth[index]));if(!Double.isFinite(t)||t<.065||t>180)continue;double x=c.x+dx*t,y=c.y+dy*t,z=c.z+dz*t;
    if(!body.surface.contains(x,z))continue;
    if(t>=depth[index]-.002){occluded++;continue;}
    boolean dry=false;for(var volume:body.dry)if(volume.excludes(x,y,z)){dry=true;break;}
    if(dry){excluded++;continue;}double bed=body.bed.bottomY(x,z);if(!Double.isFinite(bed)||bed>=y)continue;hits.add(new Hit(body,t,x,y,z));
   }
   hits.sort(Comparator.comparingDouble(Hit::depth).reversed());
   // Compatible editor tiles form one surface even where footprints overlap.
   // Other bodies preserve the existing independent transparent-layer contract.
   for(int i=hits.size()-1;i>0;i--){var h=hits.get(i);if(h.body.seamKey==null)continue;for(int j=0;j<i;j++){var q=hits.get(j);if(h.body.seamKey.equals(q.body.seamKey)&&Math.abs(h.depth-q.depth)<1e-6){hits.remove(i);break;}}}
   for(var hit:hits){
    var body=hit.body;var style=body.style;double waterDepth=Math.max(0,hit.y-body.bed.bottomY(hit.x,hit.z));
    // Prefer actual submerged opaque geometry when it provides a nearer bed.
    if(Double.isFinite(originalDepth[index])){double bottom=c.y+dy*originalDepth[index];if(bottom<hit.y)waterDepth=Math.min(waterDepth,hit.y-bottom);}
    double absorption=1-Math.exp(-style.absorption*waterDepth),alpha=style.minimumOpacity+(style.maximumOpacity-style.minimumOpacity)*absorption;
    int tint=mix(style.shallow,style.deep,absorption),detail=body.texture.sample(hit.x/style.tileMetres+seconds*style.drift,hit.z/style.tileMetres-seconds*style.drift*.37);
    int underneath=opaque[index];
    if(body.effects==null)tint=mix(tint,detail,.16);
    else{
     var e=body.effects;var normal=e.surface(hit.x,hit.z,seconds);
     int texture=mix(e.first().sample(hit.x,hit.z,seconds),e.second().sample(hit.x,hit.z,seconds),e.blend());
     double modulation=1+(((texture>>16)&255)/127.5-1)*e.detail();tint=scale(tint,modulation);
     double nl=Math.sqrt(1+normal.slopeX()*normal.slopeX()+normal.slopeZ()*normal.slopeZ()),vl=Math.sqrt(dx*dx+dy*dy+dz*dz);
     double cosine=Math.min(1,Math.abs((normal.slopeX()*dx-dy+normal.slopeZ()*dz)/(nl*vl))),reflection=e.fresnel()*(.02+.98*Math.pow(1-cosine,5));
     tint=mix(tint,e.reflection(),reflection);alpha=alpha+(1-alpha)*reflection;
     // Broken, moving shallow/contact foam. Never an emissive white outline.
     double foam=Math.max(0,1-waterDepth/e.foamDepth());foam*=e.foamPattern(hit.x,hit.z,seconds,waterDepth)*e.foamOpacity();
     var contact=e.contactFoamEnabled()?contacts.near(px,py,hit.x,hit.y,hit.z,hit.depth,normal.slopeX(),normal.slopeZ()):null;
     if(contact!=null){
      double breakup=.55+.45*WaterEffects.noise(hit.x*8+seconds*.23,hit.z*8-seconds*.17),edge=contact.foam()*breakup*e.foamOpacity();foam=Math.max(foam,edge);if(edge>.1)contactPixels++;
      if(e.sprayEnabled()&&edge>.25&&spray.size()<256){long key=WaterSpray.cell(contact.x(),contact.z())^body.id.hashCode();double crest=Math.max(0,normal.offset()/Math.max(.001,e.envelope()));spray.putIfAbsent(key,new WaterSpray.Emitter(contact.x(),hit.y,contact.z(),contact.outwardX(),contact.outwardZ(),edge*(.35+.65*crest),key));}
     }
     tint=mix(tint,0xb9c8c4,foam);alpha=alpha+(1-alpha)*foam*.8;
     if(e.refractionPixels()>0&&hits.size()==1){
      int rx=(int)Math.round(px+normal.slopeX()*e.refractionPixels()*Math.min(1,waterDepth)),ry=(int)Math.round(py+normal.slopeZ()*e.refractionPixels()*Math.min(1,waterDepth));
      if(rx>=0&&rx<c.width&&ry>=0&&ry<c.height){int ri=ry*c.width+rx;if(originalDepth[ri]>hit.depth+.01)underneath=original[ri];}
     }
    }
    tint=lighting.shade(tint,hit.x,hit.y,hit.z);opaque[index]=mix(underneath,tint,alpha);drawn++;
   }
   // Surface depth is retained for later above-water effects; RGB already contains the bed.
   if(!hits.isEmpty())depth[index]=hits.get(hits.size()-1).depth;
  }int sprayPixels=optical?WaterSpray.draw(c,opaque,depth,seconds,spray.values(),lighting):0;return new Statistics(drawn,excluded,occluded,contactPixels,sprayPixels);
 }
}
