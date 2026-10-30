package org.beckenburg.editor;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

/** Module registry does not instantiate unavailable optional integrations. */
public final class EnvironmentModules {
    private final Map<String,EnvironmentModule> modules=new LinkedHashMap<>();
    private RuntimeScene activeScene;

    public synchronized void register(EnvironmentModule module){
        Objects.requireNonNull(module);
        if(module.id()==null||module.id().isBlank())
            throw new IllegalArgumentException("Module ID required");
        if(modules.containsKey(module.id()))
            throw new IllegalArgumentException("Duplicate module: "+module.id());
        if(activeScene!=null&&module.ready())module.attach(activeScene);
        modules.put(module.id(),module);
    }

    public synchronized List<EnvironmentModule> available(){
        return List.copyOf(modules.values());
    }

    public synchronized List<EnvironmentModule> ready(){
        List<EnvironmentModule> result=new ArrayList<>();
        for(EnvironmentModule m:modules.values())if(m.ready())result.add(m);
        return List.copyOf(result);
    }

    public synchronized void attach(RuntimeScene scene){
        Objects.requireNonNull(scene);
        if(activeScene!=null)detach();
        List<EnvironmentModule> attached=new ArrayList<>();
        try{
            for(EnvironmentModule m:modules.values())if(m.ready()){
                m.attach(scene);
                attached.add(m);
            }
            activeScene=scene;
        }catch(RuntimeException failure){
            for(int i=attached.size()-1;i>=0;i--){
                try{attached.get(i).detach();}catch(RuntimeException cleanup){failure.addSuppressed(cleanup);}
            }
            throw failure;
        }
    }

    public synchronized void detach(){
        RuntimeException error=null;
        List<EnvironmentModule> all=ready();
        for(int i=all.size()-1;i>=0;i--)try{all.get(i).detach();}
        catch(RuntimeException problem){
            if(error==null)error=problem;else error.addSuppressed(problem);
        }
        activeScene=null;
        if(error!=null)throw error;
    }
}
