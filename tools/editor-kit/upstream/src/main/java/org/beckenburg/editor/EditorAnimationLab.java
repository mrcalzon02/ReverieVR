package org.beckenburg.editor;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.GridLayout;
import java.nio.file.Path;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSlider;
import javax.swing.JSpinner;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.JComboBox;
import javax.swing.SpinnerNumberModel;
import javax.swing.Timer;

/** Lightweight offline animation-authoring tool. Its output is not yet bound
 * to Beckenburg's live ghost skeleton or host-controlled animation phases.
 */
public final class EditorAnimationLab extends JPanel {
    private EditorAnimationClip clip=new EditorAnimationClip("idle",15,60);
    private final JTextField name=new JTextField("idle",12),part=new JTextField("Head",12);
    private final JComboBox<String> rate=new JComboBox<>(new String[]{"15 fps","30 fps"});
    private final JComboBox<EditorAnimationClip.Playback> playback=
        new JComboBox<>(EditorAnimationClip.Playback.values());
    private final JSlider frame=new JSlider(0,59,0);
    private final JSpinner x=number(0,-100,100,.1),y=number(0,-100,100,.1),
        z=number(0,-100,100,.1),pitch=number(0,-360,360,5),
        yaw=number(0,-360,360,5),roll=number(0,-360,360,5),
        duration=new JSpinner(new SpinnerNumberModel(60,1,7200,1)),
        tolerance=new JSpinner(new SpinnerNumberModel(0.001,0.0,1.0,0.001));
    private final JTextArea info=new JTextArea(13,54);
    private final JDialog window;
    private final Timer playbackTimer;
    private boolean playing;
    private final JPanel canvas=new JPanel(){
        @Override protected void paintComponent(Graphics g){
            super.paintComponent(g);
            Graphics2D painter=(Graphics2D)g.create();
            painter.setColor(new Color(25,30,38));painter.fillRect(0,0,getWidth(),getHeight());
            painter.setColor(new Color(95,116,137));
            painter.drawLine(getWidth()/2,16,getWidth()/2,getHeight()-16);
            painter.drawLine(20,getHeight()/2,getWidth()-20,getHeight()/2);
            var sample=clip.sample(frame.getValue(),(EditorAnimationClip.Playback)playback.getSelectedItem());
            for(var pose:sample.parts().values()){
                int ox=getWidth()/2+(int)(pose.x()*35),oy=getHeight()/2-(int)(pose.y()*35);
                painter.setColor(pose.part().equals(part.getText())?new Color(255,204,104):new Color(139,199,214));
                int radius=Math.max(4,Math.min(22,(int)(9*pose.sx())));
                painter.fillRect(ox-radius,oy-radius,2*radius,2*radius);
                painter.drawString(pose.part()+" / yaw "+Math.round(pose.yaw()),ox+radius+4,oy);
            }
            painter.setColor(Color.LIGHT_GRAY);
            painter.drawString("RIGID PART AUTHORING PREVIEW (XY projection) / animation only",8,14);
            painter.dispose();
        }
    };
    private static JSpinner number(double initial,double min,double max,double step){
        return new JSpinner(new SpinnerNumberModel(initial,min,max,step));
    }
    private static double value(JSpinner input){return ((Number)input.getValue()).doubleValue();}
    private EditorAnimationLab(JFrame parent){
        super(new BorderLayout(5,5));
        window=new JDialog(parent,"PSX Animation / Rigid Rig Authoring Lab",false);
        playbackTimer=new Timer(1000/30,e->{
            if(!playing)return;
            frame.setValue((frame.getValue()+1)%clip.frames());
        });
        var header=new JPanel(new FlowLayout(FlowLayout.LEFT,4,3));
        header.add(new JLabel("Clip"));header.add(name);
        header.add(new JLabel("Rate"));header.add(rate);
        header.add(new JLabel("Frames"));header.add(duration);
        JButton update=new JButton("APPLY CLIP");update.addActionListener(e->act(()->{
            clip.rename(name.getText().strip());
            clip.timing(rate.getSelectedIndex()==0?15:30,((Number)duration.getValue()).intValue());
            frame.setMaximum(clip.frames()-1);playbackTimer.setDelay(1000/clip.fps());refresh();
        }));
        header.add(update);
        add(header,BorderLayout.NORTH);
        JPanel editor=new JPanel(new BorderLayout(4,4));
        canvas.setPreferredSize(new Dimension(620,195));
        editor.add(canvas,BorderLayout.NORTH);
        JPanel parameters=new JPanel(new GridLayout(0,4,4,3));
        parameters.add(new JLabel("Part (rigid node)"));parameters.add(part);
        parameters.add(new JLabel("X"));parameters.add(x);
        parameters.add(new JLabel("Y"));parameters.add(y);
        parameters.add(new JLabel("Z"));parameters.add(z);
        parameters.add(new JLabel("Pitch"));parameters.add(pitch);
        parameters.add(new JLabel("Yaw"));parameters.add(yaw);
        parameters.add(new JLabel("Roll"));parameters.add(roll);
        parameters.add(new JLabel("Prune tolerance"));parameters.add(tolerance);
        editor.add(parameters,BorderLayout.CENTER);
        JPanel timeline=new JPanel(new BorderLayout(4,3));
        frame.setPaintTicks(true);frame.setMajorTickSpacing(10);
        frame.addChangeListener(e->refresh());
        timeline.add(frame,BorderLayout.CENTER);
        JPanel controls=new JPanel(new FlowLayout(FlowLayout.LEFT,3,2));
        JButton play=new JButton("PLAY/PAUSE");play.addActionListener(e->{playing=!playing;refresh();});
        JButton key=new JButton("KEY PART");key.addActionListener(e->act(()->{
            clip.put(new EditorAnimationClip.Pose(part.getText().strip(),frame.getValue(),
                value(x),value(y),value(z),value(pitch),value(yaw),value(roll),1,1,1));refresh();
        }));
        JButton root=new JButton("KEY ROOT");root.addActionListener(e->act(()->{
            clip.put(new EditorAnimationClip.Root(frame.getValue(),value(x),value(y),value(z)));
            refresh();
        }));
        JButton remove=new JButton("DELETE PART KEY");remove.addActionListener(e->{
            clip.remove(part.getText().strip(),frame.getValue());refresh();
        });
        JButton compact=new JButton("PRUNE IDENTICAL");compact.addActionListener(e->act(()->{
            int removed=clip.compact(value(tolerance));
            refresh();info.append("\nPruned "+removed+" redundant STEPPED pose keys (first/last protected)");
        }));
        JButton store=new JButton("SAVE CLIP");store.addActionListener(e->save());
        JButton load=new JButton("OPEN CLIP");load.addActionListener(e->load());
        controls.add(play);controls.add(key);controls.add(root);controls.add(remove);
        controls.add(compact);controls.add(new JLabel("Interpolation"));controls.add(playback);
        controls.add(store);controls.add(load);
        playback.addActionListener(e->refresh());
        timeline.add(controls,BorderLayout.SOUTH);
        editor.add(timeline,BorderLayout.SOUTH);
        add(editor,BorderLayout.CENTER);
        info.setEditable(false);info.setFont(new java.awt.Font("Monospaced",java.awt.Font.PLAIN,12));
        add(new JScrollPane(info),BorderLayout.SOUTH);
        setBorder(javax.swing.BorderFactory.createEmptyBorder(5,5,5,5));
        window.setContentPane(this);window.setSize(860,650);window.setLocationRelativeTo(parent);
        window.addWindowListener(new java.awt.event.WindowAdapter(){
            @Override public void windowClosed(java.awt.event.WindowEvent e){playbackTimer.stop();}
        });
        playbackTimer.setDelay(1000/clip.fps());playbackTimer.start();
        refresh();
    }
    public static void show(JFrame parent){new EditorAnimationLab(parent).window.setVisible(true);}
    private void act(Runnable action){
        try{action.run();}catch(RuntimeException error){
            JOptionPane.showMessageDialog(window,error.getMessage(),"Invalid animation edit",JOptionPane.ERROR_MESSAGE);
        }
    }
    private void refresh(){
        if(frame.getValue()>=clip.frames())frame.setValue(clip.frames()-1);
        canvas.repaint();
        var pose=clip.sample(frame.getValue(),(EditorAnimationClip.Playback)playback.getSelectedItem());
        var text=new StringBuilder();
        text.append("Clip: ").append(clip.name()).append(" / ").append(clip.fps()).append(" fps / ")
            .append(clip.frames()).append(" frames / ").append(clip.keyCount()).append(" keys\n");
        text.append("Frame: ").append(frame.getValue()).append(" / ")
            .append((String)playback.getSelectedItem().toString())
            .append(" / ").append(playing?"PLAYING":"PAUSED").append("\n");
        text.append("Active rigid parts: ").append(pose.parts().keySet()).append("\n");
        text.append("Separate root motion: ").append(pose.root()).append("\n");
        text.append("This is a portable authoring preview, NOT an in-game animated actor.\n");
        text.append("No real ghost visibility, network state, hit timing or gameplay collision is modified.\n");
        text.append("Use named Head/Torso/Arm/Leg parts; key 15/30 fps poses.\n");
        text.append("Pruning removes repeated STEPPED keys only, not root motion.\n");
        info.setText(text.toString());
    }
    private void save(){
        JFileChooser chooser=new JFileChooser();chooser.setSelectedFile(new java.io.File(clip.name()+".anim"));
        if(chooser.showSaveDialog(window)!=JFileChooser.APPROVE_OPTION)return;
        try{clip.save(chooser.getSelectedFile().toPath());refresh();
            info.append("\nSaved "+chooser.getSelectedFile());}
        catch(Exception e){JOptionPane.showMessageDialog(window,e.getMessage(),"Save failed",JOptionPane.ERROR_MESSAGE);}
    }
    private void load(){
        JFileChooser chooser=new JFileChooser();
        if(chooser.showOpenDialog(window)!=JFileChooser.APPROVE_OPTION)return;
        try{
            clip=EditorAnimationClip.load(chooser.getSelectedFile().toPath());
            name.setText(clip.name());rate.setSelectedIndex(clip.fps()==15?0:1);
            duration.setValue(clip.frames());frame.setMaximum(clip.frames()-1);
            playbackTimer.setDelay(1000/clip.fps());
            frame.setValue(0);playing=false;refresh();
        }catch(Exception e){JOptionPane.showMessageDialog(window,e.getMessage(),"Load failed",JOptionPane.ERROR_MESSAGE);}
    }
}
