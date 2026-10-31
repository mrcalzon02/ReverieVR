package org.beckenburg.editor;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Portable, authoring-only campaign state simulation.
 * No user-supplied code is interpreted or executed. This class NEVER controls
 * the actual host/multiplayer campaign lifecycle.
 */
public final class EditorCampaignFlow {
    public enum State { PENDING, ACTIVE, PAUSED, COMPLETE }
    public enum Command { START, PAUSE, RESUME, COMPLETE, STOP, RESET }
    public record Result(State before,State after,boolean changed,String reason) {}
    public record Reference(String id,String asset,String consumer) {
        public Reference {
            if(!key(id)||!key(asset)||!key(consumer))
                throw new IllegalArgumentException("Invalid reference");
        }
    }
    public static final int MAX_REFERENCES=1024,MAX_FLAGS=256,MAX_BYTES=500_000;
    private State state=State.PENDING;
    private long revisions;
    private final Map<String,Integer> flags=new LinkedHashMap<>();
    private final Map<String,Reference> refs=new LinkedHashMap<>();
    public State state(){return state;}
    public long revisions(){return revisions;}
    public Map<String,Integer> flags(){return Map.copyOf(flags);}
    public Map<String,Reference> references(){return Map.copyOf(refs);}
    public Result apply(Command command){
        if(command==null)throw new IllegalArgumentException("Missing lifecycle command");
        State previous=state;
        String reason;
        switch(command){
            case START -> {
                if(state==State.PENDING){state=State.ACTIVE;reason="Started a single authoring session";}
                else reason="Duplicate/invalid START ignored; no new session created";
            }
            case PAUSE -> {
                if(state==State.ACTIVE){state=State.PAUSED;reason="Simulation paused";}
                else reason="PAUSE has no effect outside ACTIVE";
            }
            case RESUME -> {
                if(state==State.PAUSED){state=State.ACTIVE;reason="Simulation resumed";}
                else reason="RESUME has no effect outside PAUSED";
            }
            case COMPLETE -> {
                if(state==State.ACTIVE){state=State.COMPLETE;reason="Authoring simulation complete";}
                else reason="COMPLETE requires ACTIVE";
            }
            case STOP -> {
                if(state==State.ACTIVE||state==State.PAUSED){state=State.PENDING;reason="Stopped simulated session; persistent flags retained";}
                else reason="STOP is idempotent; nothing is active";
            }
            case RESET -> {
                if(state!=State.PENDING||!flags.isEmpty()){
                    state=State.PENDING;flags.clear();revisions++;
                    return new Result(previous,state,true,"Explicit authoring RESET cleared simulation flags");
                }
                reason="RESET had nothing to reset";
            }
            default -> throw new IllegalStateException("Unknown lifecycle command");
        }
        if(state!=previous)revisions++;
        return new Result(previous,state,state!=previous,reason);
    }
    private static boolean key(String k){return k!=null&&k.matches("[A-Za-z][A-Za-z0-9_.-]{0,63}");}
    public void setFlag(String name,int value){
        if(!key(name))throw new IllegalArgumentException("Invalid persistent flag name");
        if(!flags.containsKey(name)&&flags.size()>=MAX_FLAGS)
            throw new IllegalArgumentException("Too many flags");
        flags.put(name,value);revisions++;
    }
    public void reference(Reference r){
        if(!refs.containsKey(r.id())&&refs.size()>=MAX_REFERENCES)
            throw new IllegalArgumentException("Too many referenced entities");
        refs.put(r.id(),r);revisions++;
    }
    public List<Reference> dependents(String asset){
        List<Reference> out=new ArrayList<>();
        for(Reference r:refs.values())if(r.asset().equals(asset))out.add(r);
        return List.copyOf(out);
    }
    /** Never delete a referenced asset without explicit retargeting. */
    public void deleteAsset(String asset){
        if(!dependents(asset).isEmpty())
            throw new IllegalStateException("Asset still used by "+dependents(asset).size()+" event/menu references");
        // Caller owns the actual asset. This guard only verifies the authoring graph.
    }
    public String encode(){
        StringBuilder out=new StringBuilder("BECKENBURG_CAMPAIGN_FLOW_V1\n");
        out.append("state\t").append(state).append("\n");
        for(var f:flags.entrySet())out.append("flag\t").append(f.getKey()).append("\t").append(f.getValue()).append("\n");
        for(var r:refs.values())out.append("ref\t").append(r.id()).append("\t").append(r.asset())
            .append("\t").append(r.consumer()).append("\n");
        return out.toString();
    }
    public static EditorCampaignFlow decode(String raw)throws IOException{
        if(raw==null||raw.length()>MAX_BYTES||!raw.startsWith("BECKENBURG_CAMPAIGN_FLOW_V1\n"))
            throw new IOException("Invalid or oversized authoring flow");
        EditorCampaignFlow m=new EditorCampaignFlow();
        String[] lines=raw.split("\\R");
        try{
            for(int i=1;i<lines.length;i++){
                if(lines[i].isBlank())continue;
                String[] t=lines[i].split("\t",-1);
                switch(t[0]){
                    case "state" -> {
                        if(t.length!=2||i!=1)throw new IOException("Invalid state line");
                        m.state=State.valueOf(t[1]);
                    }
                    case "flag" -> {
                        if(t.length!=3||m.flags.containsKey(t[1]))throw new IOException("Duplicate/invalid flag");
                        m.setFlag(t[1],Integer.parseInt(t[2]));
                    }
                    case "ref" -> {
                        if(t.length!=4||m.refs.containsKey(t[1]))throw new IOException("Duplicate/invalid reference");
                        m.reference(new Reference(t[1],t[2],t[3]));
                    }
                    default -> throw new IOException("Invalid line "+(i+1));
                }
            }
        }catch(IllegalArgumentException failure){throw new IOException("Malformed campaign flow",failure);}
        return m;
    }
    public void save(Path path)throws IOException{
        Path parent=path.toAbsolutePath().getParent();
        Files.createDirectories(parent);
        Path pending=Files.createTempFile(parent,"campaign-", ".pending");
        try{
            Files.writeString(pending,encode(),StandardCharsets.UTF_8);
            try{Files.move(pending,path,java.nio.file.StandardCopyOption.ATOMIC_MOVE,
                java.nio.file.StandardCopyOption.REPLACE_EXISTING);}
            catch(java.nio.file.AtomicMoveNotSupportedException e){
                Files.move(pending,path,java.nio.file.StandardCopyOption.REPLACE_EXISTING);
            }
        }finally{Files.deleteIfExists(pending);}
    }
    public static EditorCampaignFlow load(Path path)throws IOException {
        if(Files.size(path)>MAX_BYTES)throw new IOException("Campaign file exceeds limit");
        return decode(Files.readString(path,StandardCharsets.UTF_8));
    }
}
