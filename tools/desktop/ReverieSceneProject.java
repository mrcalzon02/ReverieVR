package io.github.mrcalzon02.reverievr;

import java.io.IOException;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;
import java.util.function.Supplier;
import org.beckenburg.editor.EditorProject;

/**
 * Actual ReverieVR scene-document adapter for Beckenburg's portable editor
 * contracts. No castle or Beckenburg renderer classes cross the boundary.
 *
 * The wireframe is an authoring preview. No game is declared playable just
 * because it has a scene document. Snapshot/restore uses the REAL REVSCENE
 * codec, so editor undo/redo and persistence have one authority.
 */
final class ReverieSceneProject implements EditorProject<String> {
    private final String gameId;
    private final Path scenePath;
    private final Supplier<SceneDocument> current;
    private final Consumer<SceneDocument> restore;
    ReverieSceneProject(String gameId, Path scenePath,
            Supplier<SceneDocument> current,Consumer<SceneDocument> restore){
        if(gameId==null||!gameId.matches("[a-z0-9-]{3,80}"))
            throw new IllegalArgumentException("Invalid editor game id");
        this.gameId=gameId;
        this.scenePath=java.util.Objects.requireNonNull(scenePath);
        this.current=java.util.Objects.requireNonNull(current);
        this.restore=java.util.Objects.requireNonNull(restore);
    }
    @Override public String id(){return gameId;}
    @Override public String title(){return gameId.equals("sitka")?"Sitka":gameId;}
    @Override public Path documentPath(){return scenePath;}
    @Override public List<Node> hierarchy(){
        List<Node> result=new ArrayList<>();
        for(SceneDocument.Piece p:current.get().pieces){
            result.add(new Node(p.id,p.kind,p.id,p.x,p.y,p.z));
        }
        return List.copyOf(result);
    }
    @Override public List<Asset> assets(){
        List<Asset> items=new ArrayList<>();
        items.add(new Asset("reverie://primitive/box","Primitives","Box / building-mass guide"));
        items.add(new Asset("reverie://primitive/cylinder","Primitives","Cylinder / piling guide"));
        items.add(new Asset("reverie://asset/import-obj","Models","Import local OBJ and preview geometry"));
        if("sitka".equals(gameId)){
            items.add(new Asset("reverie://sitka/building","Sitka blockout",
                "Building footprint volume (box)"));
            items.add(new Asset("reverie://sitka/piling","Sitka blockout",
                "Dock piling (cylinder)"));
            items.add(new Asset("reverie://sitka/shore","Sitka blockout",
                "Shore grade reference (box)"));
        } else if("between-deliveries-red-ledger".equals(gameId)){
            items.add(new Asset("reverie://ledger/counter","Red Ledger blockout",
                "Counter volume (box)"));
            items.add(new Asset("reverie://ledger/stool","Red Ledger blockout",
                "Stool / fixture volume (cylinder)"));
        }
        return List.copyOf(items);
    }
    @Override public String snapshot(){return current.get().encode();}
    @Override public void restore(String state){
        try{restore.accept(SceneDocument.decode(state));}
        catch(IOException error){
            throw new IllegalArgumentException("Cannot restore invalid editor snapshot",error);
        }
    }
    @Override public void save() throws IOException{current.get().save(scenePath);}
}
