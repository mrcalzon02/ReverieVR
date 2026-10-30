package io.github.mrcalzon02.reverievr;

import java.nio.file.Files;
import java.nio.file.Path;
import org.beckenburg.editor.EditorHistory;
import org.beckenburg.editor.EditorProject;

/** Headless acceptance of the *real* imported Beckenburg editor contracts. */
public final class ReverieSceneProjectTest {
    public static void main(String[] args) throws Exception {
        Path temp=Files.createTempDirectory("rv-game-editor-");
        final SceneDocument[] live={new SceneDocument()};
        Path harbor=ReverieDesktop.sceneFile(temp,"sitka","harbor");
        Path crescent=ReverieDesktop.sceneFile(temp,"sitka","crescent-bay");
        if(harbor.equals(crescent)||!harbor.toString().contains("sitka"))
            throw new AssertionError("Sitka world regions overwrite each other");
        for(String invalid:new String[]{"../escape","harbor/other","HARBOR","", "a".repeat(49)}){
            try {
                ReverieDesktop.sceneFile(temp,"sitka",invalid);
                throw new AssertionError("Unsafe region accepted: "+invalid);
            }catch(IllegalArgumentException expected){}
        }
        ReverieSceneProject project=new ReverieSceneProject(
            "sitka",harbor,()->live[0],x->live[0]=x);
        if(!project.id().equals("sitka")||project.assets().size()<6)
            throw new AssertionError("Sitka palette not populated");
        if(project.assets().stream().noneMatch(a->a.uri().equals("reverie://sitka/building")))
            throw new AssertionError("Sitka building tool unavailable");
        EditorHistory<String> history=new EditorHistory<>(project,64);
        history.execute(()->live[0].pieces.add(
            new SceneDocument.Piece("harbor-structure","box","-",1,0,3,0,2)));
        if(project.hierarchy().size()!=1||!history.canUndo())
            throw new AssertionError("Authoring mutation was not recorded");
        project.save();
        if(SceneDocument.load(harbor).pieces.size()!=1)
            throw new AssertionError("Save did not write authoritative REVSCENE");
        if(!history.undo()||!project.hierarchy().isEmpty()||!history.canRedo())
            throw new AssertionError("Undo did not revert actual document");
        if(!history.redo()||project.hierarchy().size()!=1)
            throw new AssertionError("Redo did not restore actual document");
        try{
            history.execute(()->{live[0].pieces.clear();throw new IllegalStateException("bad edit");});
            throw new AssertionError("Did not reject failed transaction");
        }catch(IllegalStateException expected){}
        if(project.hierarchy().size()!=1)
            throw new AssertionError("Failed command failed to roll back");
        ReverieSceneProject red=new ReverieSceneProject(
            "between-deliveries-red-ledger",temp.resolve("red.rvscene"),
            ()->live[0],x->live[0]=x);
        if(red.assets().stream().noneMatch(a->a.uri().equals("reverie://ledger/counter")))
            throw new AssertionError("Red Ledger blockout adapter missing");
        if(red.assets().stream().anyMatch(a->a.uri().startsWith("reverie://sitka/")))
            throw new AssertionError("Sitka-only palette leaked into Red Ledger");
        if(!ReverieDesktop.sceneFile(temp,"omega-front","ignored").endsWith("omega-front.rvscene"))
            throw new AssertionError("Other native projects lost their documents");
        System.out.println("Reverie game manager: PASS Beckenburg history/rollback, palette, Sitka regions, native scene saves");
    }
}
