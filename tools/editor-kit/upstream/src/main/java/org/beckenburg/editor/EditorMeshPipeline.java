package org.beckenburg.editor;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

/**
 * Portable bounded mesh import and non-destructive, part-aware vertex clustering.
 * No Blender, GUI, game or third-party dependencies. Source files are never modified.
 */
public final class EditorMeshPipeline {
    public static final long MAX_SOURCE_BYTES=32L*1024*1024;
    public static final int MAX_TRIANGLES=120000;
    public record Vertex(double x,double y,double z,double u,double v) {
        public Vertex {
            if(!Double.isFinite(x+y+z+u+v))throw new IllegalArgumentException("Nonfinite vertex");
        }
    }
    public record Triangle(String part,Vertex a,Vertex b,Vertex c) {
        public Triangle {
            if(part==null||part.isBlank()||a==null||b==null||c==null)
                throw new IllegalArgumentException("Invalid triangle");
        }
    }
    public record Mesh(String name,List<Triangle> triangles,long sourceBytes) {
        public Mesh {
            if(name==null||name.isBlank()||triangles==null
                ||triangles.size()>MAX_TRIANGLES||sourceBytes<0)throw new IllegalArgumentException("Invalid mesh");
            triangles=List.copyOf(triangles);
        }
    }
    public record PartStats(String name,int before,int after,int verticesBefore,int verticesAfter,
                            boolean overRegression) {}
    public record Result(Mesh source,Mesh optimized,int level,List<PartStats> parts,
                         List<String> warnings,long sourceMemoryBytes,long optimizedMemoryBytes) {
        public Result {
            parts=List.copyOf(parts);warnings=List.copyOf(warnings);
        }
        public boolean overRegressed(){return parts.stream().anyMatch(PartStats::overRegression);}
    }
    private EditorMeshPipeline(){}

    public static Mesh read(Path file)throws IOException{
        if(!Files.isRegularFile(file))throw new IOException("Not a regular model file");
        long length=Files.size(file);
        if(length<=0||length>MAX_SOURCE_BYTES)throw new IOException("Model exceeds 32 MiB input ceiling");
        byte[] raw=Files.readAllBytes(file);
        String name=file.getFileName().toString();
        String lower=name.toLowerCase(Locale.ROOT);
        if(lower.endsWith(".obj"))return obj(name,new String(raw,StandardCharsets.UTF_8),length);
        if(lower.endsWith(".stl"))return stl(name,raw,length);
        throw new IOException("Portable mesh reader supports OBJ and STL; GLB uses the game's glTF decoder");
    }

    private static Mesh obj(String name,String raw,long size)throws IOException{
        List<double[]> xyz=new ArrayList<>(),uv=new ArrayList<>();
        List<Triangle> triangles=new ArrayList<>();
        String part="Object";
        int lineNumber=0;
        try{
            for(String line:raw.split("\\R")){
                lineNumber++;String text=line.strip();
                if(text.isEmpty()||text.startsWith("#"))continue;
                String[] tokens=text.split("\\s+");
                switch(tokens[0]){
                    case "o","g" -> {if(tokens.length>1)part=tokens[1];}
                    case "v" -> {if(tokens.length<4)throw new IOException("Invalid OBJ position");
                        xyz.add(new double[]{Double.parseDouble(tokens[1]),Double.parseDouble(tokens[2]),Double.parseDouble(tokens[3])});}
                    case "vt" -> {if(tokens.length<3)throw new IOException("Invalid OBJ UV");
                        uv.add(new double[]{Double.parseDouble(tokens[1]),Double.parseDouble(tokens[2])});}
                    case "f" -> {
                        if(tokens.length<4||tokens.length>257)throw new IOException("Invalid OBJ face degree");
                        Vertex first=faceVertex(tokens[1],xyz,uv),previous=faceVertex(tokens[2],xyz,uv);
                        for(int k=3;k<tokens.length;k++){
                            Vertex next=faceVertex(tokens[k],xyz,uv);
                            triangles.add(new Triangle(part,first,previous,next));previous=next;
                            if(triangles.size()>MAX_TRIANGLES)throw new IOException("OBJ triangle budget exceeded");
                        }
                    }
                }
            }
        }catch(NumberFormatException|IndexOutOfBoundsException err){
            throw new IOException("Invalid OBJ at line "+lineNumber,err);
        }
        if(triangles.isEmpty())throw new IOException("OBJ has no faces");
        return new Mesh(name,triangles,size);
    }
    private static int positionIndex(String raw,int count)throws IOException{
        int value=Integer.parseInt(raw);
        int index=value>0?value-1:count+value;
        if(value==0||index<0||index>=count)throw new IOException("OBJ index out of bounds");
        return index;
    }
    private static Vertex faceVertex(String s,List<double[]> xyz,List<double[]> uv)throws IOException{
        String[] indices=s.split("/",-1);
        if(indices.length>3)throw new IOException("Invalid OBJ face reference");
        double[] p=xyz.get(positionIndex(indices[0],xyz.size()));
        double u=0,v=0;
        if(indices.length>1&&!indices[1].isEmpty()){
            double[] t=uv.get(positionIndex(indices[1],uv.size()));
            u=t[0];v=t[1];
        }
        return new Vertex(p[0],p[1],p[2],u,v);
    }
    private static Mesh stl(String name,byte[] raw,long size)throws IOException{
        List<Triangle> triangles=new ArrayList<>();
        if(raw.length>=84){
            long declared=Integer.toUnsignedLong(ByteBuffer.wrap(raw,80,4).order(ByteOrder.LITTLE_ENDIAN).getInt());
            if(declared<=MAX_TRIANGLES&&84L+declared*50==raw.length){
                ByteBuffer b=ByteBuffer.wrap(raw).order(ByteOrder.LITTLE_ENDIAN);
                for(int i=0;i<declared;i++){
                    int off=84+i*50;Vertex[] p=new Vertex[3];
                    for(int v=0;v<3;v++){
                        int at=off+12+v*12;
                        p[v]=new Vertex(b.getFloat(at),b.getFloat(at+4),b.getFloat(at+8),0,0);
                    }
                    triangles.add(new Triangle("Solid",p[0],p[1],p[2]));
                }
                if(triangles.isEmpty())throw new IOException("Empty STL");
                return new Mesh(name,triangles,size);
            }
        }
        String part="Solid";List<Vertex> points=new ArrayList<>(3);
        int lineNumber=0;
        try{
            for(String line:new String(raw,StandardCharsets.US_ASCII).split("\\R")){
                lineNumber++;String[] v=line.trim().split("\\s+");
                if(v.length>1&&v[0].equalsIgnoreCase("solid"))part=v[1];
                if(v.length>=4&&v[0].equalsIgnoreCase("vertex")){
                    points.add(new Vertex(Double.parseDouble(v[1]),Double.parseDouble(v[2]),Double.parseDouble(v[3]),0,0));
                    if(points.size()==3){
                        triangles.add(new Triangle(part,points.get(0),points.get(1),points.get(2)));points.clear();
                        if(triangles.size()>MAX_TRIANGLES)throw new IOException("STL triangle budget exceeded");
                    }
                }
            }
        }catch(NumberFormatException e){throw new IOException("Invalid STL at line "+lineNumber,e);}
        if(triangles.isEmpty()||!points.isEmpty())throw new IOException("No complete STL triangles");
        return new Mesh(name,triangles,size);
    }

    /** Level 0 is the exact input. Larger levels coarsen a world-space grid; limbs
     * and heads must remain named parts, never merged into an anonymous mesh. */
    public static Result regress(Mesh source,int level){
        if(level<0||level>12)throw new IllegalArgumentException("Regression level 0..12");
        if(level==0){
            Map<String,List<Triangle>> groups=new LinkedHashMap<>();
            for(Triangle t:source.triangles())
                groups.computeIfAbsent(t.part(),k->new ArrayList<>()).add(t);
            List<PartStats> exact=new ArrayList<>();
            for(var entry:groups.entrySet()){
                int vertices=uniqueVertices(entry.getValue());
                exact.add(new PartStats(entry.getKey(),entry.getValue().size(),
                    entry.getValue().size(),vertices,vertices,false));
            }
            long bytes=estimatedBytes(source.triangles());
            List<String> warnings=new ArrayList<>();
            if(source.sourceBytes()>4_000_000)
                warnings.add("SOURCE SIZE: >4 MiB; the current GLB decoder has a 4 MiB per-model ceiling");
            if(bytes>16L*1024*1024)
                warnings.add("MEMORY: estimated CPU triangle data >16 MiB (not full process heap)");
            return new Result(source,source,0,exact,warnings,bytes,bytes);
        }
        double[] bounds={Double.POSITIVE_INFINITY,Double.POSITIVE_INFINITY,Double.POSITIVE_INFINITY,
                         Double.NEGATIVE_INFINITY,Double.NEGATIVE_INFINITY,Double.NEGATIVE_INFINITY};
        Map<String,List<Triangle>> before=new LinkedHashMap<>();
        for(Triangle t:source.triangles()){
            before.computeIfAbsent(t.part(),k->new ArrayList<>()).add(t);
            for(Vertex v:new Vertex[]{t.a(),t.b(),t.c()}){
                bounds[0]=Math.min(bounds[0],v.x());bounds[1]=Math.min(bounds[1],v.y());bounds[2]=Math.min(bounds[2],v.z());
                bounds[3]=Math.max(bounds[3],v.x());bounds[4]=Math.max(bounds[4],v.y());bounds[5]=Math.max(bounds[5],v.z());
            }
        }
        double diagonal=Math.sqrt(Math.pow(bounds[3]-bounds[0],2)+Math.pow(bounds[4]-bounds[1],2)+Math.pow(bounds[5]-bounds[2],2));
        // Levels 1..12 range from fine snapping to deliberate whole-part destruction;
        // the preview always retains its source for immediate rollback.
        double cell=level==0?0:Math.max(1e-10,diagonal*Math.pow(2,level-9));
        Map<String,List<Triangle>> after=new LinkedHashMap<>();
        List<String> warnings=new ArrayList<>();
        for(var entry:before.entrySet()){
            List<Triangle> output=new ArrayList<>();Set<String> seen=new HashSet<>();
            for(Triangle t:entry.getValue()){
                Vertex a=snap(t.a(),cell),b=snap(t.b(),cell),c=snap(t.c(),cell);
                if(area2(a,b,c)<1e-20)continue;
                String key=corner(a)+"|"+corner(b)+"|"+corner(c);
                if(seen.add(key))output.add(new Triangle(entry.getKey(),a,b,c));
            }
            after.put(entry.getKey(),output);
        }
        List<Triangle> combined=new ArrayList<>();
        List<PartStats> stats=new ArrayList<>();
        for(var e:before.entrySet()){
            List<Triangle> original=e.getValue(),reduced=after.get(e.getKey());
            combined.addAll(reduced);
            int b=uniqueVertices(original),a=uniqueVertices(reduced);
            boolean broken=level>0&&(reduced.isEmpty()||(original.size()>=4&&reduced.size()<Math.max(2,original.size()*.08))
                                      ||(b>=4&&a<=2));
            stats.add(new PartStats(e.getKey(),original.size(),reduced.size(),b,a,broken));
            if(broken)warnings.add("OVER-REGRESSION: '"+e.getKey()+"' collapsed from "+original.size()+
                                  " triangles / "+b+" corners to "+reduced.size()+" triangles / "+a+" corners");
        }
        if(source.sourceBytes()>4_000_000)warnings.add("SOURCE SIZE: >4 MiB; existing Beckenburg GLB decoder has a 4 MiB per-model ceiling");
        long beforeMemory=estimatedBytes(source.triangles()),afterMemory=estimatedBytes(combined);
        if(beforeMemory>16L*1024*1024)warnings.add("MEMORY: estimated source triangle data >16 MiB (not full process heap)");
        if(combined.isEmpty())warnings.add("INVALID RESULT: reduction removed all drawable triangles. Publishing is disabled.");
        Mesh result=new Mesh(source.name(),combined,source.sourceBytes());
        // The over-regressed preview is represented by stats even when no drawable triangles survive.
        return new Result(source,result,level,stats,warnings,beforeMemory,afterMemory);
    }
    private static long estimatedBytes(List<Triangle> triangles){return (long)triangles.size()*3*5*8;}
    private static int uniqueVertices(List<Triangle> list){
        Set<String> values=new HashSet<>();
        for(Triangle t:list){values.add(corner(t.a()));values.add(corner(t.b()));values.add(corner(t.c()));}
        return values.size();
    }
    private static String corner(Vertex v){return v.x()+":"+v.y()+":"+v.z();}
    private static Vertex snap(Vertex v,double cell){
        if(cell==0)return v;
        return new Vertex(Math.rint(v.x()/cell)*cell,Math.rint(v.y()/cell)*cell,
            Math.rint(v.z()/cell)*cell,v.u(),v.v());
    }
    private static double area2(Vertex a,Vertex b,Vertex c){
        double x=b.x()-a.x(),y=b.y()-a.y(),z=b.z()-a.z();
        double p=c.x()-a.x(),q=c.y()-a.y(),r=c.z()-a.z();
        return Math.pow(y*r-z*q,2)+Math.pow(z*p-x*r,2)+Math.pow(x*q-y*p,2);
    }
}
