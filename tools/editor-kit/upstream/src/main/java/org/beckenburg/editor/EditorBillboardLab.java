package org.beckenburg.editor;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.FlowLayout;
import java.awt.Graphics;
import java.awt.GridLayout;
import java.awt.image.BufferedImage;
import java.nio.file.Files;
import java.nio.file.Path;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.SwingWorker;

/** Side-by-side 2D source/derived atlas inspection; NO 3D game renderer parity yet. */
public final class EditorBillboardLab {
    private final JDialog dialog;
    private final JComboBox<String> dimensions=new JComboBox<>(new String[]{"256 x 256","512 x 256","512 x 512"});
    private final JComboBox<EditorBillboardProfile.Palette> palette=
        new JComboBox<>(EditorBillboardProfile.Palette.values());
    private final JCheckBox dither=new JCheckBox("4x4 ordered dither");
    private final JTextArea details=new JTextArea(7,44);
    private final JButton bake=new JButton("BAKE PNG DERIVATIVE");
    private final JLabel filename=new JLabel("Choose PNG or JPEG source");
    private Path selected;
    private EditorBillboardProfile.Source source;
    private EditorBillboardProfile.Output preview;
    private int revision;
    private final JPanel pictures=new JPanel(new GridLayout(1,2,6,3)){
        @Override protected void paintComponent(Graphics g){super.paintComponent(g);}
    };
    private static final class Picture extends JPanel{
        private BufferedImage current;
        private final String title;
        Picture(String title){this.title=title;setBackground(new Color(26,31,39));}
        void show(BufferedImage image){current=image;repaint();}
        @Override protected void paintComponent(Graphics g){
            super.paintComponent(g);
            g.setColor(Color.WHITE);g.drawString(title,7,15);
            if(current==null)return;
            double scale=Math.min((getWidth()-22.)/current.getWidth(),(getHeight()-36.)/current.getHeight());
            int w=Math.max(1,(int)(current.getWidth()*scale)),h=Math.max(1,(int)(current.getHeight()*scale));
            g.drawImage(current,(getWidth()-w)/2,(getHeight()-h)/2,w,h,null);
        }
    }
    private final Picture original=new Picture("UNMODIFIED SOURCE");
    private final Picture derived=new Picture("2D BAKE PREVIEW (NOT LIVE 3D GAME RENDERER)");
    private EditorBillboardLab(JFrame parent){
        dialog=new JDialog(parent,"PSX Billboard / Background Asset Preparation",false);
        JPanel content=new JPanel(new BorderLayout(6,6));
        JPanel top=new JPanel(new BorderLayout(5,5));
        JButton choose=new JButton("IMPORT IMAGE");choose.addActionListener(e->open());
        top.add(filename,BorderLayout.CENTER);top.add(choose,BorderLayout.EAST);
        content.add(top,BorderLayout.NORTH);
        pictures.add(original);pictures.add(derived);
        pictures.setPreferredSize(new java.awt.Dimension(800,300));
        content.add(pictures,BorderLayout.CENTER);
        JPanel options=new JPanel(new FlowLayout(FlowLayout.LEFT,4,3));
        options.add(new JLabel("Canvas"));options.add(dimensions);
        options.add(new JLabel("Palette"));options.add(palette);
        options.add(dither);
        JButton refresh=new JButton("PREVIEW");refresh.addActionListener(e->convert());
        options.add(refresh);
        dimensions.addActionListener(e->convert());
        palette.addActionListener(e->convert());dither.addActionListener(e->convert());
        JPanel bottom=new JPanel(new BorderLayout(3,4));
        bottom.add(options,BorderLayout.NORTH);
        details.setEditable(false);details.setFont(new java.awt.Font("Monospaced",java.awt.Font.PLAIN,12));
        details.setText("Preview source beside a non-destructive retro PNG derivative.\n"+
            "No live billboard mapping or SceneRenderer is wired to this panel yet.");
        bottom.add(new JScrollPane(details),BorderLayout.CENTER);
        JPanel actions=new JPanel(new FlowLayout(FlowLayout.RIGHT,4,3));
        bake.setEnabled(false);bake.addActionListener(e->save());
        JButton close=new JButton("CLOSE");close.addActionListener(e->dialog.dispose());
        actions.add(new JLabel("Expensive scene relight/3D bake: NOT performed"));actions.add(bake);actions.add(close);
        bottom.add(actions,BorderLayout.SOUTH);
        content.add(bottom,BorderLayout.SOUTH);
        content.setBorder(javax.swing.BorderFactory.createEmptyBorder(6,6,6,6));
        dialog.setContentPane(content);dialog.setSize(930,620);dialog.setLocationRelativeTo(parent);
    }
    public static void show(JFrame parent){new EditorBillboardLab(parent).dialog.setVisible(true);}
    private void open(){
        JFileChooser f=new JFileChooser();
        f.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter("PNG / JPEG","png","jpg","jpeg"));
        if(f.showOpenDialog(dialog)!=JFileChooser.APPROVE_OPTION)return;
        Path input=f.getSelectedFile().toPath();
        bake.setEnabled(false);filename.setText("Reading "+input.getFileName()+"...");
        new SwingWorker<EditorBillboardProfile.Source,Void>(){
            @Override protected EditorBillboardProfile.Source doInBackground()throws Exception{
                return EditorBillboardProfile.read(input);
            }
            @Override protected void done(){
                try{
                    source=get();selected=input;filename.setText(source.name());
                    original.show(source.image());convert();
                }catch(Exception error){
                    source=null;preview=null;filename.setText("Import failed");
                    details.setText("SOURCE ERROR: "+reason(error));bake.setEnabled(false);
                }
            }
        }.execute();
    }
    private void convert(){
        if(source==null)return;
        final int seq=++revision;
        int[] w={256,512,512},h={256,256,512};
        final int width=w[dimensions.getSelectedIndex()],height=h[dimensions.getSelectedIndex()];
        final var mode=(EditorBillboardProfile.Palette)palette.getSelectedItem();
        final boolean ordered=dither.isSelected();
        final var input=source;
        bake.setEnabled(false);
        details.setText("Preparing 2D asset-only preview. Original preserved...");
        new SwingWorker<EditorBillboardProfile.Output,Void>(){
            @Override protected EditorBillboardProfile.Output doInBackground(){
                return EditorBillboardProfile.convert(input,width,height,mode,ordered);
            }
            @Override protected void done(){
                if(seq!=revision||input!=source)return;
                try{
                    preview=get();derived.show(preview.image());
                    details.setText("Source: "+input.width()+"x"+input.height()+" / "+input.fileBytes()+" compressed bytes\n"+
                        "Source decoded RGBA estimate: "+input.decodedRgbaBytes()/1024+" KiB\n"+
                        "Derivative: "+preview.width()+"x"+preview.height()+" / "+preview.palette()+
                        " / ordered dither "+preview.dither()+"\n"+
                        "Derivative decoded RGBA estimate: "+preview.decodedRgbaBytes()/1024+" KiB\n"+
                        "These are CPU image estimates, NOT measured GPU VRAM allocations.\n"+
                        "Original pixels are untouched; BAKE creates a new PNG.\n"+
                        "The 2D derivative is not yet installed as a live 3D billboard/skybox.");
                    bake.setEnabled(true);
                }catch(Exception error){details.setText("PREVIEW ERROR: "+reason(error));}
            }
        }.execute();
    }
    private void save(){
        if(source==null||preview==null||selected==null)return;
        JFileChooser f=new JFileChooser();
        f.setSelectedFile(new java.io.File("billboard-"+preview.width()+"x"+preview.height()+"-retro.png"));
        if(f.showSaveDialog(dialog)!=JFileChooser.APPROVE_OPTION)return;
        Path destination=f.getSelectedFile().toPath();
        if(destination.toAbsolutePath().normalize().equals(selected.toAbsolutePath().normalize())){
            JOptionPane.showMessageDialog(dialog,"Choose a different output path. Source is protected.",
                "Source protection",JOptionPane.WARNING_MESSAGE);return;
        }
        final var output=preview;
        bake.setEnabled(false);
        new SwingWorker<Long,Void>(){
            @Override protected Long doInBackground()throws Exception{
                EditorBillboardProfile.save(destination,output);return Files.size(destination);
            }
            @Override protected void done(){
                bake.setEnabled(true);
                try{details.append("\nBAKED "+destination+" / compressed "+get()+" bytes\n");}
                catch(Exception error){details.append("\nBAKE FAILED: "+reason(error));}
            }
        }.execute();
    }
    private static String reason(Throwable error){
        while(error.getCause()!=null)error=error.getCause();
        return String.valueOf(error.getMessage());
    }
}
