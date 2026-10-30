package dev.mrcalzon.water;
import java.util.*;
import static dev.mrcalzon.water.WaterModule.*;

/** Real footprint edge snapping, including polygonal rims of curved shapes.
 * Compatible facing edges align exactly; no bounding-box-only approximation. */
public final class WaterSnap {
 public record Edge(int index,Point a,Point b,double nx,double nz){public double length(){return Math.hypot(b.x()-a.x(),b.z()-a.z());}}
 public record Match(String movingId,String targetId,Edge moving,Edge target,double dx,double dy,double dz,double rotation,double cost){}
 public static List<Edge> edges(WaterPlacement p){var points=p.polygon().points();var result=new ArrayList<Edge>();for(int i=0;i<points.size();i++){var a=points.get(i);var b=points.get((i+1)%points.size());double dx=b.x()-a.x(),dz=b.z()-a.z(),length=Math.hypot(dx,dz);if(length<1e-8)continue;double nx=dz/length,nz=-dx/length;if(nx*((a.x()+b.x())/2-p.x())+nz*((a.z()+b.z())/2-p.z())<0){nx=-nx;nz=-nz;}result.add(new Edge(i,a,b,nx,nz));}return List.copyOf(result);}
 public static WaterPlacement rotated(WaterPlacement p,double angle){return new WaterPlacement(p.id(),p.x(),p.y(),p.z(),p.width(),p.depth(),p.yaw()+angle,p.settings(),p.shape());}
 public static Match find(WaterPlacement moving,List<WaterPlacement> targets,double tolerance,boolean angleSnap){
  if(!Double.isFinite(tolerance)||tolerance<=0||tolerance>2)throw new IllegalArgumentException("Snap tolerance0..2m");Match best=null;var original=edges(moving);var rotations=new HashMap<Long,List<Edge>>();
  for(var target:targets){if(target.id().equals(moving.id())||Math.abs(target.y()-moving.y())>.03)continue;
   if(Math.hypot(target.x()-moving.x(),target.z()-moving.z())>(Math.hypot(target.width(),target.depth())+Math.hypot(moving.width(),moving.depth()))/2+tolerance)continue;
   for(var t:edges(target))for(var edge:original){
    double turn=Math.IEEEremainder(Math.atan2(-t.nz,-t.nx)-Math.atan2(edge.nz,edge.nx),Math.PI*2);
    if(Math.abs(turn)>(angleSnap?Math.toRadians(12):1e-7))continue;
    double travel=moving.shape()==WaterPlacement.Shape.CIRCLE?0:Math.abs(turn)*Math.max(moving.width(),moving.depth())/2;if(travel>tolerance)continue;
    Edge m=edge;if(Math.abs(turn)>1e-10){long key=Math.round(turn*1e12);final double angle=key*1e-12;turn=angle;var rotated=rotations.computeIfAbsent(key,k->edges(rotated(moving,angle)));m=rotated.stream().filter(e->e.index==edge.index).findFirst().orElseThrow();}
    double tx=(t.b.x()-t.a.x())/t.length(),tz=(t.b.z()-t.a.z())/t.length(),mx=(m.a.x()+m.b.x())/2,mz=(m.a.z()+m.b.z())/2,distance=(mx-t.a.x())*t.nx+(mz-t.a.z())*t.nz;
    double dx=-distance*t.nx,dz=-distance*t.nz;
    double a=(m.a.x()+dx-t.a.x())*tx+(m.a.z()+dz-t.a.z())*tz,b=(m.b.x()+dx-t.a.x())*tx+(m.b.z()+dz-t.a.z())*tz,lo=Math.min(a,b),hi=Math.max(a,b),slide=0;
    if(hi<0)slide=-hi;else if(lo>t.length())slide=t.length()-lo;
    else if(Math.abs(lo)<tolerance/3)slide=-lo;else if(Math.abs(hi-t.length())<tolerance/3)slide=t.length()-hi;
    dx+=slide*tx;dz+=slide*tz;double cost=Math.hypot(dx,dz)+travel;
    if(cost>tolerance||Math.min(hi+slide,t.length())-Math.max(lo+slide,0)<Math.min(.005,Math.min(m.length(),t.length())*.25))continue;
    if(best==null||cost<best.cost-1e-8)best=new Match(moving.id(),target.id(),m,t,dx,target.y()-moving.y(),dz,turn,cost);
   }
  }if(best==null)for(var target:targets)if(!target.id().equals(moving.id())&&Math.abs(target.y()-moving.y())<=.03){var candidate=rim(moving,target,tolerance);if(candidate!=null&&(best==null||candidate.cost<best.cost))best=candidate;}return best;
 }
 private static boolean separated(WaterPlacement a,WaterPlacement b){var pa=a.polygon().points();var pb=b.polygon().points();var axes=new ArrayList<Edge>(edges(a));axes.addAll(edges(b));for(var e:axes){double alo=Double.POSITIVE_INFINITY,ahi=Double.NEGATIVE_INFINITY,blo=alo,bhi=ahi;for(var p:pa){double t=p.x()*e.nx+p.z()*e.nz;alo=Math.min(alo,t);ahi=Math.max(ahi,t);}for(var p:pb){double t=p.x()*e.nx+p.z()*e.nz;blo=Math.min(blo,t);bhi=Math.max(bhi,t);}if(ahi<blo-1e-9||bhi<alo-1e-9)return true;}return false;}
 private static Point nearest(Point p,Edge edge){double dx=edge.b.x()-edge.a.x(),dz=edge.b.z()-edge.a.z(),t=Math.max(0,Math.min(1,((p.x()-edge.a.x())*dx+(p.z()-edge.a.z())*dz)/(dx*dx+dz*dz)));return new Point(edge.a.x()+dx*t,edge.a.z()+dz*t);}
 private static Match rim(WaterPlacement moving,WaterPlacement target,double tolerance){if(!separated(moving,target))return null;Match best=null;for(var m:edges(moving))for(var t:edges(target)){
  for(int i=0;i<4;i++){Point a,b;if(i<2){a=i==0?m.a:m.b;b=nearest(a,t);}else{b=i==2?t.a:t.b;a=nearest(b,m);}double dx=b.x()-a.x(),dz=b.z()-a.z(),cost=Math.hypot(dx,dz);if(cost<=tolerance&&(best==null||cost<best.cost))best=new Match(moving.id(),target.id(),m,t,dx,target.y()-moving.y(),dz,0,cost);}
 }return best;}
 public static WaterPlacement apply(WaterPlacement p,Match match,WaterPlacement pivot){double c=Math.cos(match.rotation),s=Math.sin(match.rotation),x=p.x()-pivot.x(),z=p.z()-pivot.z();return new WaterPlacement(p.id(),pivot.x()+x*c-z*s+match.dx,p.y()+match.dy,pivot.z()+x*s+z*c+match.dz,p.width(),p.depth(),p.yaw()+match.rotation,p.settings(),p.shape());}
 private WaterSnap(){}
}
