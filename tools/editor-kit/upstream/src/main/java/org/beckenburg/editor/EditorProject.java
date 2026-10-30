package org.beckenburg.editor;

import java.io.IOException;
import java.nio.file.Path;
import java.util.List;

/** Engine-independent editing contract. No Beckenburg game or Swing dependency.
 * Other games implement this adapter to reuse scene navigation, tools, history,
 * palette and asset browser without importing castle geometry.
 */
public interface EditorProject<S> {
    record Node(String id,String kind,String label,double x,double y,double z) {
        public Node {
            if(id==null||id.isBlank()||kind==null||kind.isBlank()||label==null)
                throw new IllegalArgumentException("Invalid scene node");
        }
    }
    record Asset(String uri,String category,String label) {
        public Asset {
            EditorAssets.validateUri(uri);
            if(category==null||category.isBlank()||label==null||label.isBlank())
                throw new IllegalArgumentException("Invalid palette asset");
        }
    }
    String id();
    String title();
    Path documentPath();
    List<Node> hierarchy();
    List<Asset> assets();
    S snapshot();
    void restore(S snapshot);
    void save() throws IOException;
}
