package org.beckenburg.editor;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** Non-blocking scene diagnostics used by the editor and game tooling. */
public final class RuntimeSceneDiagnostics {
    private RuntimeSceneDiagnostics(){}
    public record Finding(String code,String nodeId,String message) {}
    public static List<Finding> inspect(RuntimeScene scene){
        if(scene==null)throw new IllegalArgumentException("Scene required");
        List<Finding> findings=new ArrayList<>();
        Map<String,RuntimeScene.Node> ids=new HashMap<>();
        for(RuntimeScene.Node node:scene.nodes()){
            if(node==null){
                findings.add(new Finding("NULL_NODE",null,"Scene contains a null node"));
                continue;
            }
            if(ids.putIfAbsent(node.id(),node)!=null)
                findings.add(new Finding("DUPLICATE_ID",node.id(),"Duplicate stable scene ID"));
        }
        for(RuntimeScene.Node node:ids.values()){
            String parent=node.parentId();
            if(parent!=null&&!ids.containsKey(parent))
                findings.add(new Finding("MISSING_PARENT",node.id(),"Unknown parent: "+parent));
            Set<String> visited=new HashSet<>();
            String step=node.id();
            while(step!=null&&visited.add(step)){
                RuntimeScene.Node next=ids.get(step);
                step=next==null?null:next.parentId();
            }
            if(step!=null)
                findings.add(new Finding("PARENT_CYCLE",node.id(),"Scene parenting cycle"));
        }
        return List.copyOf(findings);
    }
}
