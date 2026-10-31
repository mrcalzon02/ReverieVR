package org.beckenburg.editor;

import java.nio.file.Files;
import java.nio.file.Path;

public final class EditorCampaignFlowTest {
    private static void check(boolean yes,String why){if(!yes)throw new AssertionError(why);}
    public static void main(String[] args)throws Exception{
        var flow=new EditorCampaignFlow();
        check(flow.apply(EditorCampaignFlow.Command.START).changed(),"First start");
        long revision=flow.revisions();
        check(!flow.apply(EditorCampaignFlow.Command.START).changed(),"Duplicate start ignored");
        check(revision==flow.revisions(),"Duplicate start does not create session");
        check(flow.apply(EditorCampaignFlow.Command.PAUSE).after()==EditorCampaignFlow.State.PAUSED,"Paused");
        check(!flow.apply(EditorCampaignFlow.Command.PAUSE).changed(),"Duplicate pause ignored");
        check(flow.apply(EditorCampaignFlow.Command.RESUME).after()==EditorCampaignFlow.State.ACTIVE,"Resume");
        check(flow.apply(EditorCampaignFlow.Command.COMPLETE).after()==EditorCampaignFlow.State.COMPLETE,"Complete");
        check(!flow.apply(EditorCampaignFlow.Command.START).changed(),"Completed cannot restart without reset");
        check(!flow.apply(EditorCampaignFlow.Command.STOP).changed(),"Stop complete is harmless");
        flow.setFlag("blue_keycard",1);
        flow.reference(new EditorCampaignFlow.Reference("gate_trigger","gate_model","transition_to_keep"));
        check(flow.dependents("gate_model").size()==1,"Concrete dependent");
        boolean blocked=false;
        try{flow.deleteAsset("gate_model");}catch(IllegalStateException expected){blocked=true;}
        check(blocked,"Referenced asset deletion blocked");
        String encoded=flow.encode();
        var reloaded=EditorCampaignFlow.decode(encoded);
        check(reloaded.encode().equals(encoded),"Text format roundtrip");
        check(reloaded.flags().get("blue_keycard")==1,"Save flags");
        check(reloaded.references().size()==1,"Save refs");
        var reset=reloaded.apply(EditorCampaignFlow.Command.RESET);
        check(reset.changed()&&reloaded.flags().isEmpty(),"Explicit reset clears simulation flags");
        check(reloaded.references().size()==1,"Reset does not erase authored dependency graph");
        var stop=new EditorCampaignFlow();
        stop.setFlag("story_progress",12);
        stop.apply(EditorCampaignFlow.Command.START);
        stop.apply(EditorCampaignFlow.Command.STOP);
        check(stop.flags().get("story_progress")==12,"Stop retains persistent flags");
        check(!stop.apply(EditorCampaignFlow.Command.STOP).changed(),"Duplicate stop no action");
        Path root=Files.createTempDirectory("campaign-lab-test-");
        try{
            Path file=root.resolve("campaign.flow");
            flow.save(file);
            check(EditorCampaignFlow.load(file).encode().equals(flow.encode()),"Atomic disk persistence");
        }finally{
            try(var files=Files.list(root)){files.forEach(x->{try{Files.deleteIfExists(x);}catch(Exception ignored){}});}
            Files.deleteIfExists(root);
        }
        System.out.println("PASS idempotent campaign transitions, persistent flags, dependency guards and save/load");
    }
}
