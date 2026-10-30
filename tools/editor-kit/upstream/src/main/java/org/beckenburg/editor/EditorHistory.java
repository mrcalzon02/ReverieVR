package org.beckenburg.editor;

import java.util.ArrayDeque;
import java.util.Objects;

/** Project-independent bounded transactional undo/redo.
 * Failed commands restore their before-state instead of leaving half an edit.
 */
public final class EditorHistory<S> {
    private final EditorProject<S> project;
    private final ArrayDeque<S> undo=new ArrayDeque<>(),redo=new ArrayDeque<>();
    private final int capacity;
    public EditorHistory(EditorProject<S> project,int capacity){
        this.project=Objects.requireNonNull(project);
        if(capacity<1)throw new IllegalArgumentException("History must be positive");
        this.capacity=capacity;
    }
    public void execute(Runnable command){
        Objects.requireNonNull(command);
        S before=project.snapshot();
        try{command.run();}
        catch(RuntimeException|Error failure){
            project.restore(before);
            throw failure;
        }
        undo.push(before);
        if(undo.size()>capacity)undo.removeLast();
        redo.clear();
    }
    public boolean canUndo(){return !undo.isEmpty();}
    public boolean canRedo(){return !redo.isEmpty();}
    public boolean undo(){
        if(undo.isEmpty())return false;
        redo.push(project.snapshot());project.restore(undo.pop());return true;
    }
    public boolean redo(){
        if(redo.isEmpty())return false;
        undo.push(project.snapshot());project.restore(redo.pop());return true;
    }
}
