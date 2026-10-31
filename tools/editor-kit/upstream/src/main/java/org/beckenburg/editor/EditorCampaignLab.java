package org.beckenburg.editor;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.GridLayout;
import java.awt.Graphics;
import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;

/** Actual interactive authoring simulation, NOT a game campaign start/stop switch. */
public final class EditorCampaignLab {
    private EditorCampaignFlow flow=new EditorCampaignFlow();
    private final JDialog dialog;
    private final JTextArea log=new JTextArea(12,65);
    private final JLabel status=new JLabel();
    private final JTextField flag=new JTextField("has_blue_keycard",14),value=new JTextField("1",4);
    private final JTextField asset=new JTextField("castle_gate",14),
        consumer=new JTextField("start_event",14),reference=new JTextField("link1",9);
    private final JPanel graph=new JPanel(){
        @Override protected void paintComponent(Graphics g){
            super.paintComponent(g);
            String[] names={"PENDING","ACTIVE","PAUSED","COMPLETE"};
            int w=Math.max(120,getWidth()/4);
            for(int i=0;i<names.length;i++){
                int x=i*w+10,y=18;
                boolean selected=flow.state().name().equals(names[i]);
                g.setColor(selected?new Color(78,172,108):new Color(71,82,96));
                if(names[i].equals("PAUSED")&&selected)g.setColor(new Color(201,150,64));
                g.fillRoundRect(x,y,Math.max(95,w-24),42,10,10);
                g.setColor(Color.WHITE);g.drawString(names[i],x+12,y+25);
                if(i<3){g.setColor(Color.GRAY);g.drawString("→",x+w-18,y+25);}
            }
            g.setColor(Color.DARK_GRAY);
            g.drawString("Simulation graph: START → ACTIVE ↔ PAUSED → COMPLETE; STOP returns PENDING",12,85);
        }
    };
    private EditorCampaignLab(JFrame parent){
        dialog=new JDialog(parent,"PSX Campaign State Graph — AUTHORING SIMULATION",false);
        JPanel content=new JPanel(new BorderLayout(5,5));
        JPanel top=new JPanel(new BorderLayout());
        top.add(new JLabel("EDITOR SIMULATION ONLY — these controls never start, pause or stop an actual game session"),
            BorderLayout.NORTH);
        graph.setPreferredSize(new java.awt.Dimension(750,104));
        top.add(graph,BorderLayout.CENTER);top.add(status,BorderLayout.SOUTH);
        content.add(top,BorderLayout.NORTH);
        JPanel controls=new JPanel(new GridLayout(0,1,3,3));
        JPanel lifecycle=new JPanel(new FlowLayout(FlowLayout.LEFT,4,2));
        for(EditorCampaignFlow.Command cmd:EditorCampaignFlow.Command.values()){
            JButton button=new JButton(cmd.name());
            button.addActionListener(e->{
                var result=flow.apply(cmd);
                log.append(result.before()+" → "+result.after()+" : "+result.reason()+"\n");
                refresh();
            });
            lifecycle.add(button);
        }
        controls.add(lifecycle);
        JPanel flags=new JPanel(new FlowLayout(FlowLayout.LEFT,4,2));
        flags.add(new JLabel("Persistent simulation flag"));flags.add(flag);flags.add(value);
        JButton putFlag=new JButton("SET FLAG");
        putFlag.addActionListener(e->act(()->{
            flow.setFlag(flag.getText().strip(),Integer.parseInt(value.getText().strip()));
            log.append("Set authored flag "+flag.getText()+"\n");refresh();
        }));
        flags.add(putFlag);controls.add(flags);
        JPanel depend=new JPanel(new FlowLayout(FlowLayout.LEFT,4,2));
        depend.add(new JLabel("Reference id"));depend.add(reference);
        depend.add(new JLabel("Asset id"));depend.add(asset);
        depend.add(new JLabel("Consumer"));depend.add(consumer);
        JButton bind=new JButton("BIND");bind.addActionListener(e->act(()->{
            flow.reference(new EditorCampaignFlow.Reference(reference.getText().strip(),
                asset.getText().strip(),consumer.getText().strip()));
            log.append("Bound dependency "+reference.getText()+"\n");refresh();
        }));
        JButton inspect=new JButton("CHECK DELETE");
        inspect.addActionListener(e->{
            var list=flow.dependents(asset.getText().strip());
            log.append("Asset '"+asset.getText()+"' dependents: "+list+"\n");
            try{flow.deleteAsset(asset.getText().strip());
                log.append("No known references (this authoring graph only; live game dependency scan still required).\n");
            }catch(IllegalStateException ex){log.append("DEPENDENCY WARNING: "+ex.getMessage()+"\n");}
        });
        depend.add(bind);depend.add(inspect);controls.add(depend);
        content.add(controls,BorderLayout.CENTER);
        log.setEditable(false);log.setFont(new Font(Font.MONOSPACED,Font.PLAIN,12));
        JPanel bottom=new JPanel(new BorderLayout());
        bottom.add(new JScrollPane(log),BorderLayout.CENTER);
        JPanel footer=new JPanel(new FlowLayout(FlowLayout.RIGHT,3,2));
        JButton save=new JButton("SAVE FLOW");save.addActionListener(e->save());
        JButton open=new JButton("OPEN FLOW");open.addActionListener(e->open());
        JButton close=new JButton("CLOSE");close.addActionListener(e->dialog.dispose());
        footer.add(open);footer.add(save);footer.add(close);
        bottom.add(footer,BorderLayout.SOUTH);content.add(bottom,BorderLayout.SOUTH);
        content.setBorder(javax.swing.BorderFactory.createEmptyBorder(6,6,6,6));
        dialog.setContentPane(content);dialog.setSize(970,580);
        dialog.setLocationRelativeTo(parent);
        log.setText("No arbitrary scripts are run. Safe transitions, persistent flags and dependency guards only.\n");
        refresh();
    }
    public static void show(JFrame parent){new EditorCampaignLab(parent).dialog.setVisible(true);}
    private void act(Runnable r){
        try{r.run();}catch(RuntimeException err){
            JOptionPane.showMessageDialog(dialog,err.getMessage(),"Invalid graph edit",JOptionPane.ERROR_MESSAGE);
        }
    }
    private void refresh(){
        status.setText("State: "+flow.state()+"  |  transitions/edits: "+flow.revisions()+
            "  |  flags: "+flow.flags().size()+"  |  dependencies: "+flow.references().size());
        graph.repaint();
    }
    private void save(){
        JFileChooser picker=new JFileChooser();
        picker.setSelectedFile(new java.io.File("campaign-preview.flow"));
        if(picker.showSaveDialog(dialog)!=JFileChooser.APPROVE_OPTION)return;
        try{flow.save(picker.getSelectedFile().toPath());
            log.append("Saved authoring graph: "+picker.getSelectedFile()+"\n");
        }catch(Exception e){JOptionPane.showMessageDialog(dialog,e.getMessage(),"Save failed",JOptionPane.ERROR_MESSAGE);}
    }
    private void open(){
        JFileChooser picker=new JFileChooser();
        if(picker.showOpenDialog(dialog)!=JFileChooser.APPROVE_OPTION)return;
        try{flow=EditorCampaignFlow.load(picker.getSelectedFile().toPath());refresh();
            log.append("Loaded authoring graph: "+picker.getSelectedFile()+"\n");
        }catch(Exception e){JOptionPane.showMessageDialog(dialog,e.getMessage(),"Open failed",JOptionPane.ERROR_MESSAGE);}
    }
}
