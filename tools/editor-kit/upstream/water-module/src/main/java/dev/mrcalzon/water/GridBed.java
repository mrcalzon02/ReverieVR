package dev.mrcalzon.water;
import java.io.*;
import java.util.*;

/** Shared optical bed geometry and depth queries. Uses the exact same diagonal
 * and planar interpolation for triangles and samples; never player collision. */
public final class GridBed implements WaterModule.Bathymetry {
 private final double x,z,step;private final int width,height;private final double[] y;
 public GridBed(double x,double z,double step,int width,int height,double[] values){
  if(!Double.isFinite(x+z+step)||step<=0||width<2||height<2||(long)width*height>200000||values.length!=width*height)throw new IllegalArgumentException("Bed budget");
  for(double v:values)if(!Double.isFinite(v)||Math.abs(v)>10000)throw new IllegalArgumentException("Bed elevation");
  this.x=x;this.z=z;this.step=step;this.width=width;this.height=height;y=values.clone();
 }
 public record Vertex(double x,double y,double z){}
 public record Triangle(Vertex a,Vertex b,Vertex c){}
 private Vertex vertex(int i,int j){return new Vertex(x+i*step,y[j*width+i],z+j*step);}
 public List<Triangle> triangles(){var out=new ArrayList<Triangle>();for(int j=0;j<height-1;j++)for(int i=0;i<width-1;i++){var a=vertex(i,j);var b=vertex(i+1,j);var c=vertex(i+1,j+1);var d=vertex(i,j+1);out.add(new Triangle(a,c,b));out.add(new Triangle(a,d,c));}return List.copyOf(out);}
 @Override public double bottomY(double wx,double wz){double u=(wx-x)/step,v=(wz-z)/step;if(u<0||v<0||u>width-1||v>height-1)return Double.NaN;int i=Math.min(width-2,(int)u),j=Math.min(height-2,(int)v);u-=i;v-=j;double a=y[j*width+i],b=y[j*width+i+1],c=y[(j+1)*width+i+1],d=y[(j+1)*width+i];return a+(v<=u?(b-a)*u+(c-b)*v:(c-d)*u+(d-a)*v);}
 public static GridBed read(InputStream input)throws IOException{
  if(input==null)throw new IOException("Missing optical bed");try(var in=new DataInputStream(input)){
   if(in.readInt()!=0x57424431)throw new IOException("Bed version");double x=in.readDouble(),z=in.readDouble(),step=in.readDouble();int w=in.readInt(),h=in.readInt();if(w<2||h<2||(long)w*h>200000)throw new IOException("Bed dimensions");double[] a=new double[w*h];for(int i=0;i<a.length;i++)a[i]=in.readDouble();if(in.read()!=-1)throw new IOException("Trailing bed data");try{return new GridBed(x,z,step,w,h,a);}catch(IllegalArgumentException e){throw new IOException(e);}
  }
 }
}
