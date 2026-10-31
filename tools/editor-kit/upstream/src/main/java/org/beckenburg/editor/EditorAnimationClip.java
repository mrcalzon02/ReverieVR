package org.beckenburg.editor;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** Portable animation authoring model. Not the authoritative simulation/physics driver.
 * Separate rigid-part poses from the optional ROOT displacement track.
 * No Blender, renderer, networking, native PS1 hardware or game dependencies.
 */
public final class EditorAnimationClip {
    public static final int MAX_KEYS=8192;
    public static final int MAX_PARTS=128;
    public enum Playback { STEPPED, LINEAR }
    public record Pose(String part,int frame,double x,double y,double z,
                       double pitch,double yaw,double roll,double sx,double sy,double sz) {
        public Pose {
            if(part==null||!part.matches("[A-Za-z][A-Za-z0-9_\\-]{0,63}")
                ||part.equalsIgnoreCase("ROOT")||frame<0
                ||!valid(x,y,z,pitch,yaw,roll,sx,sy,sz)
                ||sx<=0||sy<=0||sz<=0)throw new IllegalArgumentException("Invalid rigid-part pose");
        }
    }
    public record Root(int frame,double x,double y,double z) {
        public Root {
            if(frame<0||!valid(x,y,z))throw new IllegalArgumentException("Invalid root motion");
        }
    }
    public record Sample(Map<String,Pose> parts,Root root) {
        public Sample {parts=Map.copyOf(parts);}
    }
    private String name;
    private int fps;
    private int frames;
    private final Map<String,java.util.TreeMap<Integer,Pose>> tracks=new LinkedHashMap<>();
    private final java.util.TreeMap<Integer,Root> roots=new java.util.TreeMap<>();
    public EditorAnimationClip(String name,int fps,int frames){
        rename(name);timing(fps,frames);
    }
    private static boolean valid(double... values){
        for(double value:values)if(!Double.isFinite(value)||Math.abs(value)>100000)return false;
        return true;
    }
    public String name(){return name;}
    public int fps(){return fps;}
    public int frames(){return frames;}
    public int keyCount(){
        int n=roots.size();for(var t:tracks.values())n+=t.size();return n;
    }
    public Set<String> parts(){return Set.copyOf(tracks.keySet());}
    public List<Pose> poses(){
        List<Pose> all=new ArrayList<>();
        for(var t:tracks.values())all.addAll(t.values());
        all.sort(Comparator.comparing(Pose::part).thenComparingInt(Pose::frame));
        return List.copyOf(all);
    }
    public List<Root> rootKeys(){return List.copyOf(roots.values());}
    public void rename(String newName){
        if(newName==null||!newName.matches("[A-Za-z][A-Za-z0-9_\\-]{0,63}"))
            throw new IllegalArgumentException("Use a short ASCII animation name");
        this.name=newName;
    }
    public void timing(int newFps,int newFrames){
        if(newFps!=15&&newFps!=30||newFrames<1||newFrames>7200)
            throw new IllegalArgumentException("Animation needs 15/30 fps and 1..7200 frames");
        if(!roots.isEmpty()&&roots.lastKey()>=newFrames)
            throw new IllegalArgumentException("Root key extends beyond requested frame count");
        for(var track:tracks.values())if(!track.isEmpty()&&track.lastKey()>=newFrames)
            throw new IllegalArgumentException("Part key extends beyond requested frame count");
        fps=newFps;frames=newFrames;
    }
    public void put(Pose pose){
        if(pose.frame()>=frames)throw new IllegalArgumentException("Pose is outside clip");
        if(!tracks.containsKey(pose.part())&&tracks.size()>=MAX_PARTS)
            throw new IllegalArgumentException("Too many rigid parts");
        boolean replace=tracks.containsKey(pose.part())&&tracks.get(pose.part()).containsKey(pose.frame());
        if(!replace&&keyCount()>=MAX_KEYS)throw new IllegalArgumentException("Too many animation keys");
        tracks.computeIfAbsent(pose.part(),k->new java.util.TreeMap<>()).put(pose.frame(),pose);
    }
    public void remove(String part,int frame){
        var t=tracks.get(part);if(t==null)return;
        t.remove(frame);if(t.isEmpty())tracks.remove(part);
    }
    public void put(Root root){
        if(root.frame()>=frames)throw new IllegalArgumentException("Root key outside clip");
        if(!roots.containsKey(root.frame())&&keyCount()>=MAX_KEYS)
            throw new IllegalArgumentException("Too many animation keys");
        roots.put(root.frame(),root);
    }
    public void removeRoot(int frame){roots.remove(frame);}
    public Sample sample(int frame,Playback playback){
        if(frame<0||frame>=frames)throw new IllegalArgumentException("Frame out of bounds");
        Map<String,Pose> result=new LinkedHashMap<>();
        for(var entry:tracks.entrySet()){
            var t=entry.getValue();
            if(t.isEmpty())continue;
            var lo=t.floorEntry(frame);var hi=t.ceilingEntry(frame);
            if(lo==null)lo=t.firstEntry();
            if(hi==null)hi=t.lastEntry();
            result.put(entry.getKey(),
                playback==Playback.LINEAR&&lo.getKey()<hi.getKey()&&frame>lo.getKey()
                    ?mix(lo.getValue(),hi.getValue(),(double)(frame-lo.getKey())/(hi.getKey()-lo.getKey()),frame)
                    :lo.getValue());
        }
        Root root=new Root(frame,0,0,0);
        if(!roots.isEmpty()){
            var lo=roots.floorEntry(frame);var hi=roots.ceilingEntry(frame);
            if(lo==null)lo=roots.firstEntry();
            if(hi==null)hi=roots.lastEntry();
            root=playback==Playback.LINEAR&&lo.getKey()<hi.getKey()&&frame>lo.getKey()
                ?mix(lo.getValue(),hi.getValue(),(double)(frame-lo.getKey())/(hi.getKey()-lo.getKey()),frame)
                :lo.getValue();
        }
        return new Sample(result,root);
    }
    private static double lerp(double a,double b,double t){return a+(b-a)*t;}
    private static Pose mix(Pose a,Pose b,double t,int frame){
        return new Pose(a.part(),frame,lerp(a.x(),b.x(),t),lerp(a.y(),b.y(),t),lerp(a.z(),b.z(),t),
            lerp(a.pitch(),b.pitch(),t),lerp(a.yaw(),b.yaw(),t),lerp(a.roll(),b.roll(),t),
            lerp(a.sx(),b.sx(),t),lerp(a.sy(),b.sy(),t),lerp(a.sz(),b.sz(),t));
    }
    private static Root mix(Root a,Root b,double t,int frame){
        return new Root(frame,lerp(a.x(),b.x(),t),lerp(a.y(),b.y(),t),lerp(a.z(),b.z(),t));
    }
    private static boolean close(Pose a,Pose b,double epsilon){
        return Math.abs(a.x()-b.x())<=epsilon&&Math.abs(a.y()-b.y())<=epsilon
            &&Math.abs(a.z()-b.z())<=epsilon&&Math.abs(a.pitch()-b.pitch())<=epsilon
            &&Math.abs(a.yaw()-b.yaw())<=epsilon&&Math.abs(a.roll()-b.roll())<=epsilon
            &&Math.abs(a.sx()-b.sx())<=epsilon&&Math.abs(a.sy()-b.sy())<=epsilon
            &&Math.abs(a.sz()-b.sz())<=epsilon;
    }
    /** Safe for STEPPED playback only: removes intermediate keys that are
     * identical to the preceding pose; never removes first/last pose of any part.
     * Does not modify root motion, retime a clip, or alter required impact poses.
     */
    public int compact(double epsilon){
        if(!Double.isFinite(epsilon)||epsilon<0||epsilon>1)
            throw new IllegalArgumentException("Invalid pose reduction tolerance");
        int removed=0;
        for(var t:tracks.values()){
            var keys=new ArrayList<>(t.values());
            if(keys.size()<3)continue;
            Pose prior=keys.get(0);
            for(int i=1;i<keys.size()-1;i++){
                Pose next=keys.get(i);
                if(close(prior,next,epsilon)){t.remove(next.frame());removed++;}
                else prior=next;
            }
        }
        return removed;
    }
    /** Stable text, one key per line, versioned and diffable. Safe bounded read. */
    public String encode(){
        StringBuilder b=new StringBuilder("BECKENBURG_ANIM_V1\n");
        b.append("name\t").append(name).append("\n");
        b.append("timing\t").append(fps).append("\t").append(frames).append("\n");
        for(Pose p:poses())b.append("pose\t").append(p.part()).append("\t").append(p.frame())
            .append("\t").append(p.x()).append("\t").append(p.y()).append("\t").append(p.z())
            .append("\t").append(p.pitch()).append("\t").append(p.yaw()).append("\t").append(p.roll())
            .append("\t").append(p.sx()).append("\t").append(p.sy()).append("\t").append(p.sz()).append("\n");
        for(Root r:roots.values())b.append("root\t").append(r.frame()).append("\t")
            .append(r.x()).append("\t").append(r.y()).append("\t").append(r.z()).append("\n");
        return b.toString();
    }
    public static EditorAnimationClip decode(String text)throws IOException{
        if(text==null||text.length()>1_500_000)throw new IOException("Animation too large");
        String[] lines=text.split("\\R");
        if(lines.length<3||!lines[0].equals("BECKENBURG_ANIM_V1"))
            throw new IOException("Unsupported animation document");
        try{
            String[] name=lines[1].split("\t",-1),timing=lines[2].split("\t",-1);
            if(name.length!=2||!name[0].equals("name")||timing.length!=3||!timing[0].equals("timing"))
                throw new IOException("Invalid animation header");
            EditorAnimationClip clip=new EditorAnimationClip(name[1],
                Integer.parseInt(timing[1]),Integer.parseInt(timing[2]));
            for(int i=3;i<lines.length;i++){
                if(lines[i].isBlank())continue;
                String[] t=lines[i].split("\t",-1);
                if(t[0].equals("pose")&&t.length==12)
                    clip.put(new Pose(t[1],Integer.parseInt(t[2]),Double.parseDouble(t[3]),
                        Double.parseDouble(t[4]),Double.parseDouble(t[5]),Double.parseDouble(t[6]),
                        Double.parseDouble(t[7]),Double.parseDouble(t[8]),Double.parseDouble(t[9]),
                        Double.parseDouble(t[10]),Double.parseDouble(t[11])));
                else if(t[0].equals("root")&&t.length==5)
                    clip.put(new Root(Integer.parseInt(t[1]),Double.parseDouble(t[2]),
                        Double.parseDouble(t[3]),Double.parseDouble(t[4])));
                else throw new IOException("Unknown record on line "+(i+1));
            }
            return clip;
        }catch(IllegalArgumentException e){throw new IOException("Invalid animation document",e);}
    }
    public void save(Path path)throws IOException {
        Path parent=path.toAbsolutePath().getParent();
        Files.createDirectories(parent);
        Path pending=Files.createTempFile(parent,"anim-", ".pending");
        try{
            Files.writeString(pending,encode(),StandardCharsets.UTF_8);
            try{Files.move(pending,path,java.nio.file.StandardCopyOption.ATOMIC_MOVE,
                java.nio.file.StandardCopyOption.REPLACE_EXISTING);}
            catch(java.nio.file.AtomicMoveNotSupportedException ex){
                Files.move(pending,path,java.nio.file.StandardCopyOption.REPLACE_EXISTING);
            }
        }finally{Files.deleteIfExists(pending);}
    }
    public static EditorAnimationClip load(Path path)throws IOException {
        if(Files.size(path)>1_500_000)throw new IOException("Animation file exceeds budget");
        return decode(Files.readString(path,StandardCharsets.UTF_8));
    }
}
