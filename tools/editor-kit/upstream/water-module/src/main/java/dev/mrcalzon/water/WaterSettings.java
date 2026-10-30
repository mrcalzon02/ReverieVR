package dev.mrcalzon.water;
import java.util.*;
import java.nio.charset.StandardCharsets;
import java.security.*;
import java.util.function.Function;
import static dev.mrcalzon.water.WaterModule.*;

/** Portable, immutable editor/runtime settings. No Beckenburg classes or paths. */
public final class WaterSettings {
 public record NumberField(String key,String label,double minimum,double maximum,double step,double initial){}
 public static final List<NumberField> NUMBERS=List.of(
  new NumberField("wave.height","Wave height (m)",0,.5,.01,.16),
  new NumberField("wave.grid","Wave grid (m)",.25,8,.25,.75),
  new NumberField("wave.speed","Wave time speed",0,5,.05,.8),
  new NumberField("wave.noise","Wave noise scale",.001,2,.005,.045),
  new NumberField("shore.height","Shore run-up (m)",0,.08,.005,.06),
  new NumberField("shore.period","Shore period (s)",2,60,.5,5.5),
  new NumberField("shore.length","Shore wavelength (m)",.5,30,.1,2.8),
  new NumberField("foam.depth","Shallow foam depth (m)",.01,3,.02,.28),
  new NumberField("foam.opacity","Foam strength",0,1,.05,.65),
  new NumberField("alpha.minimum","Shallow opacity",0,1,.01,.48),
  new NumberField("alpha.maximum","Deep opacity",0,1,.01,.97),
  new NumberField("alpha.absorption","Depth absorption /m",.01,20,.1,1.8),
  new NumberField("bed.depth","Fallback bed depth (m)",.05,100,.1,3.5),
  new NumberField("blend","Layer blend",0,1,.01,.27),
  new NumberField("detail","Texture contrast",0,1,.01,.35),
  new NumberField("refraction","Refraction (px)",0,12,.5,5),
  new NumberField("fresnel","Grazing reflection",0,1,.05,.5),
  new NumberField("layer1.scaleX","Layer 1 U repeats/m",.001,32,.01,.28),
  new NumberField("layer1.scaleZ","Layer 1 V repeats/m",.001,32,.01,.28),
  new NumberField("layer1.direction","Layer 1 travel degrees",0,360,5,Math.toDegrees(Math.atan2(-.015/.28,-.001/.28))+360),
  new NumberField("layer1.speed","Layer 1 travel m/s",0,4,.005,Math.hypot(.015/.28,.001/.28)),
  new NumberField("layer1.pixels","Layer 1 pixel steps",2,4096,1,128),
  new NumberField("layer2.scaleX","Layer 2 U repeats/m",.001,32,.01,.19),
  new NumberField("layer2.scaleZ","Layer 2 V repeats/m",.001,32,.01,.23),
  new NumberField("layer2.direction","Layer 2 travel degrees",0,360,5,Math.toDegrees(Math.atan2(.015/.19,.02/.23))),
  new NumberField("layer2.speed","Layer 2 travel m/s",0,4,.005,Math.hypot(.015/.19,.02/.23)),
  new NumberField("layer2.pixels","Layer 2 pixel steps",2,4096,1,192));
 private final Map<String,String> values;
 public WaterSettings(){this(Map.of());}
 public WaterSettings(Map<String,String> changes){
  var all=new TreeMap<String,String>();for(var n:NUMBERS)all.put(n.key,Double.toString(n.initial));
  all.put("layer1.texture","water://ripple-a");all.put("layer2.texture","water://ripple-b");all.put("colour.shallow","365859");all.put("colour.deep","132e39");all.put("colour.reflection","303b4c");all.put("bed.profile","flat");all.put("contact.enabled","true");all.put("spray.enabled","true");
  for(var entry:changes.entrySet()){if(!all.containsKey(entry.getKey())||entry.getValue()==null)throw new IllegalArgumentException("Unknown water setting "+entry.getKey());all.put(entry.getKey(),entry.getValue());}
  for(var n:NUMBERS){double v=Double.parseDouble(all.get(n.key));if(!Double.isFinite(v)||v<n.minimum||v>n.maximum||n.key.endsWith("pixels")&&v!=Math.rint(v))throw new IllegalArgumentException(n.label+" must be "+n.minimum+".."+n.maximum);all.put(n.key,Double.toString(n.key.endsWith("direction")?v%360:v));}
  if(Double.parseDouble(all.get("alpha.minimum"))>Double.parseDouble(all.get("alpha.maximum")))throw new IllegalArgumentException("Shallow opacity cannot exceed deep opacity");
  for(String key:List.of("layer1.texture","layer2.texture")){String uri=all.get(key);if(!uri.matches("[a-z][a-z0-9+.-]*://[A-Za-z0-9_./-]{1,180}")||uri.contains(".."))throw new IllegalArgumentException("Unsafe water texture URI");}
  for(String key:List.of("colour.shallow","colour.deep","colour.reflection")){String v=all.get(key).replace("#","").toLowerCase(Locale.ROOT);if(!v.matches("[0-9a-f]{6}"))throw new IllegalArgumentException("Water colour must have six hex digits");all.put(key,v);}
  if(!all.get("bed.profile").matches("[a-z][a-z0-9_-]{0,60}"))throw new IllegalArgumentException("Bed profile identifier");
  for(String key:List.of("contact.enabled","spray.enabled"))if(!List.of("true","false").contains(all.get(key)))throw new IllegalArgumentException("Invalid water toggle");
  values=Collections.unmodifiableMap(all);
 }
 public Map<String,String> values(){return values;}
 public String text(String key){if(!values.containsKey(key))throw new IllegalArgumentException(key);return values.get(key);}
 public double number(String key){return Double.parseDouble(text(key));}
 public boolean enabled(String key){return Boolean.parseBoolean(text(key));}
 public WaterSettings with(String key,Object value){var copy=new TreeMap<>(values);copy.put(key,String.valueOf(value));return new WaterSettings(copy);}
 public Style style(){return new Style(Integer.parseInt(text("colour.shallow"),16),Integer.parseInt(text("colour.deep"),16),number("alpha.absorption"),number("alpha.minimum"),number("alpha.maximum"),3.2,.018);}
 private WaterEffects.Layer layer(int index,Function<String,Texture> textures){String p="layer"+index;double angle=Math.toRadians(number(p+".direction")),speed=number(p+".speed"),sx=number(p+".scaleX"),sz=number(p+".scaleZ");return new WaterEffects.Layer(textures.apply(text(p+".texture")),sx,sz,-Math.sin(angle)*speed*sx,-Math.cos(angle)*speed*sz,(int)number(p+".pixels"));}
 public WaterEffects effects(Function<String,Texture> textures,Bathymetry bed,double level){return new WaterEffects(layer(1,textures),layer(2,textures),number("blend"),number("detail"),number("wave.height"),number("wave.grid"),number("wave.speed"),number("wave.noise"),number("foam.depth"),number("foam.opacity"),number("refraction"),number("fresnel"),Integer.parseInt(text("colour.reflection"),16),new WaterEffects.ShoreWave(bed,level,number("shore.height"),number("shore.period"),number("shore.length"),.22),enabled("contact.enabled"),enabled("spray.enabled"));}
 public String revision(){try{var hash=MessageDigest.getInstance("SHA-256");values.forEach((k,v)->hash.update((k+"="+v+"\n").getBytes(StandardCharsets.UTF_8)));return HexFormat.of().formatHex(hash.digest());}catch(NoSuchAlgorithmException e){throw new IllegalStateException(e);}}
 @Override public boolean equals(Object other){return other instanceof WaterSettings s&&values.equals(s.values);}
 @Override public int hashCode(){return values.hashCode();}
}
