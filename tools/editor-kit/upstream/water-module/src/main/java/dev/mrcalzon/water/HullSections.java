package dev.mrcalzon.water;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import static dev.mrcalzon.water.WaterModule.*;

/** Interior sections cut from the actual hull. Adjacent-section union is
 * conservative within the wall thickness; it never masks an outer AABB. */
public final class HullSections implements Exclusion {
 public record Section(double y,Polygon interior){}
 private final List<Section> sections;
 public HullSections(List<Section> sections){
  this.sections=List.copyOf(sections);if(sections.size()<2||sections.size()>256)throw new IllegalArgumentException("Hull section budget");
  double last=Double.NEGATIVE_INFINITY;for(var s:sections){if(!Double.isFinite(s.y)||s.y<=last||s.interior==null)throw new IllegalArgumentException("Unsorted hull sections");last=s.y;}
 }
 public List<Section> sections(){return sections;}
 @Override public boolean excludes(double x,double y,double z){
  if(y<sections.get(0).y||y>sections.get(sections.size()-1).y)return false;
  for(int i=1;i<sections.size();i++)if(y<=sections.get(i).y){return sections.get(i-1).interior.contains(x,z)||sections.get(i).interior.contains(x,z);}return false;
 }
 /** Bounded portable text contract, one Y followed by X/Z pairs per row. */
 public static HullSections read(InputStream input)throws IOException{
  if(input==null)throw new IOException("Missing authored hull sections");
  byte[] bytes;try(input){bytes=input.readNBytes(1_000_001);}if(bytes.length>1_000_000)throw new IOException("Hull resource budget");
  try{
   String[] lines=new String(bytes,StandardCharsets.UTF_8).split("\\R");if(!lines[0].equals("WATER_HULL_SECTIONS_1"))throw new IOException("Hull version");var list=new ArrayList<Section>();
   for(int i=1;i<lines.length;i++){if(lines[i].isBlank())continue;var v=lines[i].split("\\t");if(v.length<7||v.length%2!=1)throw new IOException("Hull row");var p=new ArrayList<Point>();for(int j=1;j<v.length;j+=2)p.add(new Point(Double.parseDouble(v[j]),Double.parseDouble(v[j+1])));list.add(new Section(Double.parseDouble(v[0]),new Polygon(p)));}
   return new HullSections(list);
  }catch(IllegalArgumentException e){throw new IOException("Invalid hull profile",e);}
 }
}
