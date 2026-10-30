package org.beckenburg.editor;

import java.util.List;
import java.util.Map;
import java.util.Objects;

/**
 * Engine-agnostic scene boundary shared by authoring tools and live runtimes.
 * Implementations own storage and simulation: this interface never duplicates a preview scene.
 * Coordinates are world-space metres, rotations are degrees and scale is unitless.
 */
public interface RuntimeScene {
    record Transform(double x,double y,double z,
                     double pitch,double yaw,double roll,
                     double scaleX,double scaleY,double scaleZ) {
        public Transform {
            for(double value:new double[]{x,y,z,pitch,yaw,roll,scaleX,scaleY,scaleZ})
                if(!Double.isFinite(value))throw new IllegalArgumentException("Non-finite transform");
            if(scaleX<=0||scaleY<=0||scaleZ<=0)
                throw new IllegalArgumentException("Scale must be positive");
        }
        public static Transform identity(){
            return new Transform(0,0,0,0,0,0,1,1,1);
        }
    }

    /** Components retain type-specific property values without binding the core to a game. */
    record Component(String type,Map<String,String> properties) {
        public Component {
            if(type==null||type.isBlank())throw new IllegalArgumentException("Component type required");
            properties=Map.copyOf(Objects.requireNonNull(properties));
        }
    }

    record Node(String id,String name,String parentId,String layer,
                Transform transform,List<Component> components) {
        public Node {
            if(id==null||id.isBlank()||name==null||name.isBlank()||
               layer==null||layer.isBlank())throw new IllegalArgumentException("Node identity required");
            if(id.equals(parentId))throw new IllegalArgumentException("Node cannot parent itself");
            transform=Objects.requireNonNull(transform);
            components=List.copyOf(Objects.requireNonNull(components));
        }
    }

    /** Stable scene ID, including across save/load and edit/play transitions. */
    String sceneId();
    List<Node> nodes();

    default Node find(String id){
        Objects.requireNonNull(id);
        for(Node node:nodes())if(node.id().equals(id))return node;
        return null;
    }

    /** Mutations must pass through the runtime's command/transaction boundary. */
    void replace(Node node);
    void remove(String id);

    /** A snapshot must be independent of subsequent mutations. */
    Snapshot snapshot();
    void restore(Snapshot snapshot);

    interface Snapshot {}
}
