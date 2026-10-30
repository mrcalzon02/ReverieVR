package dev.mrcalzon.water;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.security.*;
import java.util.*;

/** Bounded deterministic UTF-8 authoring/runtime file. Unknown fields fail
 * instead of silently losing settings. No game, project or rendering imports. */
public record WaterDocument(String level,List<WaterPlacement> placements){
 public WaterDocument{
  if(level==null||!level.matches("[A-Za-z0-9_-]{1,80}")||placements==null||placements.size()>64)throw new IllegalArgumentException("Water document budget");placements=List.copyOf(placements);var ids=new HashSet<String>();for(var p:placements)if(!ids.add(p.id()))throw new IllegalArgumentException("Duplicate water ID");
 }
 private static void line(StringBuilder b,String key,Object value){b.append(key).append('=').append(value).append('\n');}
 public String encode(){var b=new StringBuilder();line(b,"version",1);line(b,"level",level);line(b,"count",placements.size());int index=0;
  for(var p:placements){String k="water."+(index++)+".";line(b,k+"id",p.id());line(b,k+"shape",p.shape().name());line(b,k+"x",p.x());line(b,k+"y",p.y());line(b,k+"z",p.z());line(b,k+"width",p.width());line(b,k+"depth",p.depth());line(b,k+"yaw",p.yaw());p.settings().values().forEach((name,value)->line(b,k+"setting."+name,value));}return b.toString();
 }
 public String revision(){try{return HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(encode().getBytes(StandardCharsets.UTF_8)));}catch(NoSuchAlgorithmException e){throw new IllegalStateException(e);}}
 public static WaterDocument read(InputStream input)throws IOException{
  if(input==null)throw new IOException("Missing water document");byte[] bytes;try(input){bytes=input.readNBytes(2_000_001);}if(bytes.length>2_000_000)throw new IOException("Water document exceeds2MB");var p=new Properties();try{p.load(new StringReader(new String(bytes,StandardCharsets.UTF_8)));}catch(IllegalArgumentException e){throw new IOException("Invalid property encoding",e);}
  try{if(!"1".equals(p.getProperty("version")))throw new IllegalArgumentException("Water document version");int n=Integer.parseInt(p.getProperty("count"));if(n<0||n>64)throw new IllegalArgumentException("Water count");var values=new ArrayList<WaterPlacement>();var keys=new HashSet<String>(List.of("version","level","count"));
   for(int i=0;i<n;i++){String k="water."+i+".";for(String name:List.of("id","x","y","z","width","depth","yaw"))keys.add(k+name);keys.add(k+"shape");var settings=new TreeMap<String,String>();for(String key:p.stringPropertyNames())if(key.startsWith(k+"setting.")){settings.put(key.substring((k+"setting.").length()),p.getProperty(key));keys.add(key);}values.add(new WaterPlacement(p.getProperty(k+"id"),number(p,k+"x"),number(p,k+"y"),number(p,k+"z"),number(p,k+"width"),number(p,k+"depth"),number(p,k+"yaw"),new WaterSettings(settings),WaterPlacement.Shape.valueOf(p.getProperty(k+"shape","SQUARE"))));}
   if(!keys.containsAll(p.stringPropertyNames()))throw new IllegalArgumentException("Unknown water document field");return new WaterDocument(p.getProperty("level"),values);
  }catch(IllegalArgumentException e){throw new IOException("Invalid water document: "+e.getMessage(),e);}
 }
 private static double number(Properties p,String key){String value=p.getProperty(key);if(value==null)throw new IllegalArgumentException("Missing "+key);return Double.parseDouble(value);}
 public void save(Path path)throws IOException{Path target=path.toAbsolutePath().normalize(),parent=target.getParent();Files.createDirectories(parent);Path temp=Files.createTempFile(parent,"water-",".tmp");try{Files.writeString(temp,encode(),StandardCharsets.UTF_8);try{Files.move(temp,target,StandardCopyOption.REPLACE_EXISTING,StandardCopyOption.ATOMIC_MOVE);}catch(AtomicMoveNotSupportedException e){Files.move(temp,target,StandardCopyOption.REPLACE_EXISTING);}}finally{Files.deleteIfExists(temp);}}
}
