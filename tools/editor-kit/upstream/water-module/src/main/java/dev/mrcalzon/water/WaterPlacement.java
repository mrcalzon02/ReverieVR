package dev.mrcalzon.water;
import java.util.*;
import java.util.function.Function;
import static dev.mrcalzon.water.WaterModule.*;

/** A self-contained, movable/resizable module instance. Y is the mean surface. */
public record WaterPlacement(String id,double x,double y,double z,double width,double depth,double yaw,WaterSettings settings,Shape shape){
 public enum Shape {SQUARE,CIRCLE,OBLONG,OCTAGON,PILL}
 public WaterPlacement(String id,double x,double y,double z,double width,double depth,double yaw,WaterSettings settings){this(id,x,y,z,width,depth,yaw,settings,Shape.SQUARE);}
 public WaterPlacement{
  if(id==null||!id.matches("[A-Za-z0-9_-]{1,64}")||settings==null||shape==null||!Double.isFinite(x+y+z+width+depth+yaw)||Math.abs(x)>10000||Math.abs(y)>10000||Math.abs(z)>10000||width<.1||depth<.1||width>2000||depth>2000)throw new IllegalArgumentException("Invalid water placement");yaw=Math.IEEEremainder(yaw,Math.PI*2);if(shape==Shape.CIRCLE)depth=width;
 }
 public Polygon polygon(){
  var local=new ArrayList<Point>();
  switch(shape){
   case SQUARE->{for(double[] q:new double[][]{{-width/2,-depth/2},{width/2,-depth/2},{width/2,depth/2},{-width/2,depth/2}})local.add(new Point(q[0],q[1]));}
   case CIRCLE,OBLONG,OCTAGON->{int count=shape==Shape.OCTAGON?8:64;double rx=shape==Shape.CIRCLE?Math.min(width,depth)/2:width/2,rz=shape==Shape.CIRCLE?Math.min(width,depth)/2:depth/2;if(shape==Shape.OCTAGON){rx/=Math.cos(Math.PI/8);rz/=Math.cos(Math.PI/8);}for(int i=0;i<count;i++){double angle=Math.PI*2*i/count+(shape==Shape.OCTAGON?Math.PI/8:0);local.add(new Point(rx*Math.cos(angle),rz*Math.sin(angle)));}}
   case PILL->{boolean alongX=width>=depth;double radius=Math.min(width,depth)/2,half=(Math.max(width,depth)-2*radius)/2;for(int end=0;end<2;end++)for(int i=0;i<=32;i++){double angle=-Math.PI/2+end*Math.PI+i*Math.PI/32,u=(end==0?half:-half)+radius*Math.cos(angle),v=radius*Math.sin(angle);local.add(alongX?new Point(u,v):new Point(v,u));}}
  }
  var points=new ArrayList<Point>();double c=Math.cos(yaw),sin=Math.sin(yaw);for(var q:local)points.add(new Point(x+q.x()*c-q.z()*sin,z+q.x()*sin+q.z()*c));return new Polygon(points);
 }
 public WaterPlacement moved(double dx,double dy,double dz){return new WaterPlacement(id,x+dx,y+dy,z+dz,width,depth,yaw,settings,shape);}
 public WaterPlacement resized(double width,double depth){if(shape==Shape.CIRCLE){double diameter=width!=this.width?width:depth;width=diameter;depth=diameter;}return new WaterPlacement(id,x,y,z,width,depth,yaw,settings,shape);}
 public WaterPlacement configured(WaterSettings next){return new WaterPlacement(id,x,y,z,width,depth,yaw,next,shape);}
 public WaterPlacement shaped(Shape shape){double w=shape==Shape.CIRCLE?Math.min(width,depth):width;return new WaterPlacement(id,x,y,z,w,shape==Shape.CIRCLE?w:depth,yaw,settings,shape);}
 public Body body(Function<String,Texture> textures,Function<WaterPlacement,Bathymetry> beds,List<? extends Exclusion> dry){var bed=beds.apply(this);return new Body(id,polygon(),y,bed,textures.apply(settings.text("layer1.texture")),settings.style(),dry,settings.effects(textures,bed,y),settings.revision());}
}
