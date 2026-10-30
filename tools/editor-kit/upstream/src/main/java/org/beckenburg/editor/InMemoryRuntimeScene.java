package org.beckenburg.editor;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

/** Portable in-memory scene suitable for standalone tools and small games.
 * Production games can instead adapt their authoritative scene storage.
 */
public final class InMemoryRuntimeScene implements RuntimeScene {
    private final String id;
    private final Map<String,Node> entries=new LinkedHashMap<>();
    public InMemoryRuntimeScene(String id){
        if(id==null||id.isBlank())throw new IllegalArgumentException("Scene ID required");
        this.id=id;
    }
    public String sceneId(){return id;}
    public synchronized List<Node> nodes(){return List.copyOf(entries.values());}
    public synchronized void replace(Node node){
        Objects.requireNonNull(node);
        if(node.parentId()!=null&&!entries.containsKey(node.parentId()))
            throw new IllegalArgumentException("Unknown parent: "+node.parentId());
        String parent=node.parentId();
        while(parent!=null){
            if(parent.equals(node.id()))throw new IllegalArgumentException("Parent cycle");
            Node ancestor=entries.get(parent);
            parent=ancestor==null?null:ancestor.parentId();
        }
        entries.put(node.id(),node);
    }
    public synchronized void remove(String nodeId){
        Objects.requireNonNull(nodeId);
        if(!entries.containsKey(nodeId))return;
        // Prevent references to deleted parents: remove descendants as a unit.
        List<String> pending=new ArrayList<>();
        pending.add(nodeId);
        for(int i=0;i<pending.size();i++){
            String parent=pending.get(i);
            for(Node node:entries.values())
                if(parent.equals(node.parentId()))pending.add(node.id());
        }
        for(String child:pending)entries.remove(child);
    }
    public synchronized Snapshot snapshot(){
        return new SceneState(id,List.copyOf(entries.values()));
    }
    public synchronized void restore(Snapshot snapshot){
        if(!(snapshot instanceof SceneState state)||!id.equals(state.sceneId()))
            throw new IllegalArgumentException("Snapshot belongs to another scene");
        entries.clear();
        for(Node node:state.nodes())entries.put(node.id(),node);
    }
    private record SceneState(String sceneId,List<Node> nodes) implements Snapshot {}
}
