package org.beckenburg.editor;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.GridLayout;
import java.nio.file.Path;
import javax.swing.JButton;
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

/** Authoring-only WAV conversion and sound-size inspection. Not a second mixer. */
public final class EditorSoundLab {
    private final JDialog dialog;
    private final JTextArea report=new JTextArea(13,48);
    private final JComboBox<String> rate=new JComboBox<>(new String[]{"22050 Hz mono","11025 Hz mono"});
    private final JComboBox<String> depth=new JComboBox<>(new String[]{"16-bit","8-bit"});
    private final JButton export=new JButton("EXPORT DERIVATIVE");
    private final JLabel sourceLabel=new JLabel("No source selected");
    private Path selected;
    private EditorSoundProfile.Source source;
    private EditorSoundLab(JFrame parent){
        dialog=new JDialog(parent,"PSX SFX / Audio Asset Authoring",false);
        JPanel content=new JPanel(new BorderLayout(6,6));
        JButton choose=new JButton("OPEN PCM WAV");
        choose.addActionListener(e->choose());
        JPanel header=new JPanel(new BorderLayout(5,5));
        header.add(sourceLabel,BorderLayout.CENTER);header.add(choose,BorderLayout.EAST);
        content.add(header,BorderLayout.NORTH);
        report.setEditable(false);report.setFont(new Font(Font.MONOSPACED,Font.PLAIN,12));
        content.add(new JScrollPane(report),BorderLayout.CENTER);
        JPanel bottom=new JPanel(new BorderLayout(4,4));
        JPanel presets=new JPanel(new FlowLayout(FlowLayout.LEFT,3,4));
        presets.add(new JLabel("Sample rate"));presets.add(rate);
        presets.add(new JLabel("Precision"));presets.add(depth);
        presets.add(new JLabel("Optional PSX-like compactness preset; not a hardware requirement"));
        bottom.add(presets,BorderLayout.NORTH);
        JPanel buttons=new JPanel(new FlowLayout(FlowLayout.RIGHT,4,3));
        export.addActionListener(e->write());export.setEnabled(false);
        JButton close=new JButton("CLOSE");close.addActionListener(e->dialog.dispose());
        buttons.add(export);buttons.add(close);
        bottom.add(buttons,BorderLayout.SOUTH);content.add(bottom,BorderLayout.SOUTH);
        content.setBorder(javax.swing.BorderFactory.createEmptyBorder(8,8,8,8));
        dialog.setContentPane(content);dialog.setSize(790,430);dialog.setLocationRelativeTo(parent);
        report.setText("Inspect original 8/16-bit mono/stereo PCM WAV without altering it.\n"+
            "Save a separate mono WAV at 11.025 or 22.05 kHz, 8 or 16 bit.\n"+
            "This tool makes assets only; it does not register sounds with the game mixer.\n"+
            "Source provenance, license and any in-game trigger routing still require review.");
    }
    public static void show(JFrame parent){new EditorSoundLab(parent).dialog.setVisible(true);}
    private void choose(){
        JFileChooser picker=new JFileChooser();
        picker.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter("PCM WAV (*.wav)","wav"));
        if(picker.showOpenDialog(dialog)!=JFileChooser.APPROVE_OPTION)return;
        Path path=picker.getSelectedFile().toPath();
        selected=null;source=null;export.setEnabled(false);sourceLabel.setText("Reading "+path.getFileName());
        new SwingWorker<EditorSoundProfile.Source,Void>(){
            @Override protected EditorSoundProfile.Source doInBackground()throws Exception{
                return EditorSoundProfile.read(path);
            }
            @Override protected void done(){
                try{
                    source=get();selected=path;export.setEnabled(true);
                    sourceLabel.setText(path.getFileName().toString());
                    String text="SOURCE WAV: "+source.filename()+"\n"+
                        "Channels: "+source.channels()+"  /  Sample rate: "+source.sampleRate()+" Hz\n"+
                        "Length: "+String.format(java.util.Locale.ROOT,"%.2f",(double)source.frames()/source.sampleRate())+" seconds\n"+
                        "File bytes: "+source.sourceBytes()+"  /  Decoded PCM bytes: "+source.decodedBytes()+"\n"+
                        "A 22.05 kHz/16-bit mono derivative uses ~"+
                        Math.round(source.frames()*22050.0/source.sampleRate()*2/1024)+" KiB of PCM.\n"+
                        "A 11.025 kHz/8-bit mono derivative uses ~"+
                        Math.round(source.frames()*11025.0/source.sampleRate()/1024)+" KiB of PCM.\n"+
                        (source.decodedBytes()>512*1024?"RETRO BUDGET WARNING: larger than historical PS1 SPU RAM; modern Java is NOT subject to that fixed cap.\n":"")+
                        "\nSOURCE PRESERVATION: This source is never overwritten. Export a sibling with a new filename.\n"+
                        "Suggested sound design: dry transient, optional pitch variation, separate shared room reverb.\n"+
                        "Do not add ghost/arrow sounds to gameplay until provenance, listener and source mapping are validated.";
                    report.setText(text);
                }catch(Exception ex){sourceLabel.setText("Import failed");report.setText("WAV IMPORT FAILED: "+cause(ex));}
            }
        }.execute();
    }
    private void write(){
        if(source==null||selected==null)return;
        JFileChooser picker=new JFileChooser();
        String name=selected.getFileName().toString().replaceFirst("(?i)\\.wav$","");
        picker.setSelectedFile(new java.io.File(name+"-retro-"+(rate.getSelectedIndex()==0?"22050":"11025")+
            "-"+(depth.getSelectedIndex()==0?"16":"8")+".wav"));
        if(picker.showSaveDialog(dialog)!=JFileChooser.APPROVE_OPTION)return;
        Path destination=picker.getSelectedFile().toPath();
        if(destination.toAbsolutePath().normalize().equals(selected.toAbsolutePath().normalize())){
            JOptionPane.showMessageDialog(dialog,"Choose a different filename: overwriting originals is forbidden.",
                "Original protected",JOptionPane.WARNING_MESSAGE);return;
        }
        final EditorSoundProfile.Source input=source;
        export.setEnabled(false);
        new SwingWorker<EditorSoundProfile.Output,Void>(){
            @Override protected EditorSoundProfile.Output doInBackground()throws Exception{
                var made=EditorSoundProfile.export(input,rate.getSelectedIndex()==0?22050:11025,
                    depth.getSelectedIndex()==0?16:8);
                EditorSoundProfile.save(destination,made);
                return made;
            }
            @Override protected void done(){
                export.setEnabled(true);
                try{
                    var made=get();
                    report.append("\nEXPORTED: "+destination+"\n"+
                        "Result: "+made.sampleRate()+" Hz / "+made.bitDepth()+" bit / "+
                        made.frames()+" frames / "+made.wav().length+" bytes.\n"+
                        "Original remains unchanged. Live mixer integration is separate.");
                }catch(Exception ex){report.append("\nEXPORT FAILED: "+cause(ex));}
            }
        }.execute();
    }
    private static String cause(Throwable t){while(t.getCause()!=null)t=t.getCause();return String.valueOf(t.getMessage());}
}
