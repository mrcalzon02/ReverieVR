package org.beckenburg.editor.ai;

import java.util.List;
import java.util.Map;
import java.util.Set;

public final class NpcRuleWorkbenchTest {
    private static void ok(boolean expected,String name){if(!expected)throw new AssertionError(name);}
    private static NpcRuleBook.Rule sample(String name,String base,String favored){
        return new NpcRuleBook.Rule(name,base,"STANDARD","CLEANSING","LURK",
            "FLOAT","DOORS","CAMERA_ONLY","ISOLATION",3,90,true,true,
            favored,35,"Dark shape prefers empty rooms","Unattended camera outline",
            78,255,Set.of("DREAD"),Map.of("EMF","NONE","WRITING","RITUAL_CIRCLES"));
    }
    public static void main(String[] args){
        NpcRuleBook book=new NpcRuleBook(List.of(sample("SHADE","SHADE","MANIFEST"),
            sample("ANCIENT_SHADE","SHADE","MANIFEST")));
        ok(book.all().size()==2,"Book holds all variants");
        String machine=NpcRuleExport.toJson(book);
        ok(machine.contains("\"schema\":\"beckenburg.npc-canonical-rule-export/v1\""),
            "Machine export has stable schema");
        ok(machine.contains("\"id\":\"ANCIENT_SHADE\"")&&machine.contains("\"CAMERA_ONLY\""),
            "Machine export reads all rule entries");
        ok(machine.indexOf("\"id\":\"ANCIENT_SHADE\"") <
            machine.indexOf("\"id\":\"SHADE\""),
            "JSON species order deterministic");
        try{
            java.nio.file.Path folder=java.nio.file.Files.createTempDirectory("npc-rule-test-");
            try{
                java.nio.file.Path export=folder.resolve("npc.json");
                NpcRuleExport.save(book,export);
                ok(java.nio.file.Files.readString(export).equals(machine),
                    "Atomic rule export matches in-memory source snapshot");
            }finally{
                try(var files=java.nio.file.Files.list(folder)){
                    files.forEach(path->{try{java.nio.file.Files.deleteIfExists(path);}catch(Exception ignored){}});
                }
                java.nio.file.Files.deleteIfExists(folder);
            }
        }catch(java.io.IOException problem){throw new AssertionError("Rulebook save failed",problem);}
        ok(book.explain("SHADE").contains("CAMERA_ONLY"),"Can explain actual visibility channel");
        ok(book.explain("ANCIENT_SHADE").contains("inherits mechanical base SHADE"),
            "Can explain inheritance");
        ok(book.inspect().stream().anyMatch(f->f.code().equals("VISIBILITY_RULE")),
            "Warn about camera-only rendering");
        var observation=new NpcDecisionWorkbench.Observation(200,2,100,true,true,
            List.of(1,3),Set.of("STRIKE","MANIFEST","DRIFT","DISTURB"));
        var victimDenied=NpcDecisionWorkbench.inspect(book.get("SHADE"),
            observation,"STRIKE",2,false);
        ok(!victimDenied.eligible()&&victimDenied.reason().contains("spatial victim"),
            "Never infer victim contact from hearing");
        ok(NpcDecisionWorkbench.inspect(book.get("SHADE"),observation,"DRIFT",4,false)
            .eligible()==false,"Never traverse unlisted portal");
        ok(NpcDecisionWorkbench.inspect(book.get("SHADE"),observation,"DRIFT",3,false)
            .eligible(),"Legal portal can be previewed");
        ok(!NpcDecisionWorkbench.inspect(book.get("SHADE"),observation,"MANIFEST",3,false)
            .eligible(),"Non-movement cannot target a distant room");
        var plan=NpcDecisionWorkbench.propose(book.get("SHADE"),observation,false);
        ok(plan.eligible()&&!plan.action().equals("STRIKE"),"No unsupported strike proposal");
        ok(NpcDecisionWorkbench.propose(book.get("SHADE"),
            new NpcDecisionWorkbench.Observation(1,2,10,false,false,List.of(),Set.of()),false)
            .action().equals("NONE"),"No fabricated legal action");
        var trace=NpcDecisionWorkbench.replay(book.get("SHADE"),
            List.of(new NpcDecisionWorkbench.Observation(1,2,10,false,false,List.of(),Set.of()),
                observation),false);
        ok(trace.size()==2&&trace.get(1).tick()==200,"Bounded deterministic replay");
        boolean thrown=false;
        try{NpcDecisionWorkbench.replay(book.get("SHADE"),List.of(observation,observation),false);}
        catch(IllegalArgumentException expected){thrown=true;}
        ok(thrown,"Replay rejects out-of-order snapshots");
        thrown=false;
        try{new NpcRuleBook(List.of(sample("SHADE","SHADE","MANIFEST"),
            sample("SHADE","SHADE","MANIFEST")));}
        catch(IllegalArgumentException expected){thrown=true;}
        ok(thrown,"Duplicate ghost IDs rejected");
        thrown=false;
        try{new NpcDecisionWorkbench.Observation(1,0,100,false,false,List.of(1,1),Set.of());}
        catch(IllegalArgumentException expected){thrown=true;}
        ok(thrown,"Invalid nav exits rejected");
        System.out.println("PASS portable NPC rulebook, inheritance, observation legality and bounded decision replay");
    }
}
