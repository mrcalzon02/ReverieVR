package io.github.mrcalzon02.reverievr;

import java.awt.Component;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import org.beckenburg.editor.EditorAnimationLab;
import org.beckenburg.editor.EditorAuthoringReference;
import org.beckenburg.editor.EditorBillboardLab;
import org.beckenburg.editor.EditorCampaignLab;
import org.beckenburg.editor.EditorGlbWriter;
import org.beckenburg.editor.EditorMeshPipeline;
import org.beckenburg.editor.EditorSoundLab;

/** Portable Beckenburg authoring utilities, never an Android runtime dependency. */
public final class EditorToolLauncher {
    private EditorToolLauncher() {}
    public static void install(JFrame frame) {
        JMenuBar menuBar=frame.getJMenuBar();
        if(menuBar==null){menuBar=new JMenuBar();frame.setJMenuBar(menuBar);}
        JMenu tools=new JMenu("PSX authoring");
        item(tools,"Animation clips...",()->EditorAnimationLab.show(frame));
        item(tools,"Sound conversion...",()->EditorSoundLab.show(frame));
        item(tools,"Billboard art...",()->EditorBillboardLab.show(frame));
        item(tools,"Campaign flow...",()->EditorCampaignLab.show(frame));
        item(tools,"Mesh reduction / GLB...",()->mesh(frame));
        tools.addSeparator();
        item(tools,"Editor reference...",()->EditorAuthoringReference.show(frame));
        menuBar.add(tools);
    }
    private static void item(JMenu menu,String title,Runnable action) {
        JMenuItem item=new JMenuItem(title);
        item.addActionListener(e->action.run());
        menu.add(item);
    }
    private static void mesh(Component parent) {
        JFileChooser chooser=new JFileChooser();
        chooser.setDialogTitle("Open licensed local OBJ or STL mesh");
        if(chooser.showOpenDialog(parent)!=JFileChooser.APPROVE_OPTION)return;
        Path input=chooser.getSelectedFile().toPath().toAbsolutePath().normalize();
        try {
            EditorMeshPipeline.Mesh original=EditorMeshPipeline.read(input);
            JComboBox<Integer> level=new JComboBox<>();
            for(int i=0;i<=12;i++)level.addItem(i);
            level.setSelectedItem(0);
            if(JOptionPane.showConfirmDialog(parent,level,
                    "Mesh reduction level (0 preserves geometry)",
                    JOptionPane.OK_CANCEL_OPTION,JOptionPane.QUESTION_MESSAGE)!=JOptionPane.OK_OPTION)return;
            EditorMeshPipeline.Result result=EditorMeshPipeline.regress(
                original,(Integer)level.getSelectedItem());
            if(result.optimized().triangles().isEmpty())
                throw new IOException("No drawable triangles remain; source has not been changed");
            String message="Source: "+original.triangles().size()+" triangles, "+result.sourceMemoryBytes()+
                " estimated bytes\nExport: "+result.optimized().triangles().size()+
                " triangles, "+result.optimizedMemoryBytes()+" estimated bytes"+
                "\nParts: "+result.parts().size()+
                "\nWarnings: "+String.join("; ",result.warnings())+
                "\n\nExport a new GLB file? Source remains untouched.";
            if(JOptionPane.showConfirmDialog(parent,message,"Review mesh export",
                    JOptionPane.YES_NO_OPTION,JOptionPane.WARNING_MESSAGE)!=JOptionPane.YES_OPTION)return;
            chooser.setSelectedFile(new java.io.File(input.getFileName().toString()+".glb"));
            if(chooser.showSaveDialog(parent)!=JFileChooser.APPROVE_OPTION)return;
            Path output=chooser.getSelectedFile().toPath().toAbsolutePath().normalize();
            if(!output.getFileName().toString().toLowerCase(java.util.Locale.ROOT).endsWith(".glb"))
                throw new IOException("Output must use .glb extension");
            if(Files.exists(output)&&Files.isSameFile(input,output))
                throw new IOException("Refusing to overwrite input source");
            if(Files.exists(output)&&JOptionPane.showConfirmDialog(parent,
                "Replace existing GLB file?", "Confirm replace",JOptionPane.YES_NO_OPTION)
                !=JOptionPane.YES_OPTION)return;
            byte[] glb=EditorGlbWriter.encode(result.optimized());
            saveAtomically(output,glb);
            JOptionPane.showMessageDialog(parent,"GLB saved: "+output+"\n"+glb.length+
                " bytes. This does not install a runtime model.","Export complete",
                JOptionPane.INFORMATION_MESSAGE);
        }catch(Exception error){
            JOptionPane.showMessageDialog(parent,error.getMessage(),
                "Mesh import/export failed",JOptionPane.ERROR_MESSAGE);
        }
    }
    private static void saveAtomically(Path output,byte[] data)throws IOException {
        Path folder=output.toAbsolutePath().getParent();
        Files.createDirectories(folder);
        Path pending=Files.createTempFile(folder,"mesh-export-", ".pending");
        try{
            Files.write(pending,data);
            try{Files.move(pending,output,StandardCopyOption.ATOMIC_MOVE,
                        StandardCopyOption.REPLACE_EXISTING);}
            catch(java.nio.file.AtomicMoveNotSupportedException error){
                Files.move(pending,output,StandardCopyOption.REPLACE_EXISTING);
            }
        }finally{Files.deleteIfExists(pending);}
    }
}
