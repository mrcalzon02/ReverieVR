package org.beckenburg.editor;

import java.nio.file.Files;
import java.nio.file.Path;

public final class EditorAnimationClipTest {
    private static void check(boolean yes,String why){if(!yes)throw new AssertionError(why);}
    public static void main(String[] args)throws Exception {
        var clip=new EditorAnimationClip("walk_cycle",15,30);
        clip.put(new EditorAnimationClip.Pose("Head",0,0,1,0,0,0,0,1,1,1));
        clip.put(new EditorAnimationClip.Pose("Head",10,0,1,0,0,0,0,1,1,1));
        clip.put(new EditorAnimationClip.Pose("Head",20,0,2,0,0,30,0,1,1,1));
        clip.put(new EditorAnimationClip.Pose("Head",29,0,2,0,0,30,0,1,1,1));
        clip.put(new EditorAnimationClip.Pose("Torso",0,0,0,0,0,0,0,1,1,1));
        clip.put(new EditorAnimationClip.Root(0,0,0,0));
        clip.put(new EditorAnimationClip.Root(29,2,0,0));
        check(clip.sample(15,EditorAnimationClip.Playback.STEPPED).parts().get("Head").y()==1,
            "Stepped sampling stays at previous pose");
        check(clip.sample(15,EditorAnimationClip.Playback.LINEAR).parts().get("Head").y()==1.5,
            "Linear preview interpolates between keys");
        check(clip.sample(15,EditorAnimationClip.Playback.STEPPED).root().x()==0,
            "Root track separate from pose");
        check(clip.sample(15,EditorAnimationClip.Playback.LINEAR).root().x()>0,
            "Root interpolation works in preview");
        check(clip.compact(0.001)==1,"Only redundant middle pose pruned");
        check(clip.rootKeys().size()==2,"Root keys preserved by pruning");
        check(clip.poses().stream().filter(x->x.part().equals("Head")).count()==3,
            "First and last pose protected");
        String encoded=clip.encode();
        check(encoded.equals(EditorAnimationClip.decode(encoded).encode()),"Deterministic text round trip");
        boolean bad=false;
        try{EditorAnimationClip.decode(encoded.replace("BECKENBURG_ANIM_V1","NO_VERSION"));}
        catch(java.io.IOException expected){bad=true;}
        check(bad,"Bad version rejected");
        bad=false;try{clip.timing(60,30);}catch(IllegalArgumentException expected){bad=true;}
        check(bad,"60 fps not a valid current authoring preset");
        var tooMany=new EditorAnimationClip("short",30,1);
        bad=false;
        try{tooMany.put(new EditorAnimationClip.Pose("Arm",1,0,0,0,0,0,0,1,1,1));}
        catch(IllegalArgumentException expected){bad=true;}
        check(bad,"No out-of-clip keys");
        Path dir=Files.createTempDirectory("anim-test-");
        try{
            Path file=dir.resolve("clip.anim");
            clip.save(file);
            check(EditorAnimationClip.load(file).encode().equals(encoded),"Disk save/load");
        }finally{
            try(var s=Files.list(dir)){s.forEach(p->{try{Files.delete(p);}catch(Exception ignored){}});}
            Files.deleteIfExists(dir);
        }
        System.out.println("PASS animation stepped/linear sampling, root separation, pruning, bounds, atomic save/load");
    }
}
