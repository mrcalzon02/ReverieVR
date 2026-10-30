package io.github.mrcalzon02.reverievr;

import dev.mrcalzon.lighting.LightModel;
import dev.mrcalzon.lighting.LightingModule;
import java.awt.*;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.*;
import java.util.Properties;
import javax.swing.*;

/** Game-private desktop lighting authoring dock backed by Beckenburg's real
 * Java 17 lighting settings and physical falloff math. No VR runtime claim.
 * Never writes user-global Beckenburg lighting preferences.
 */
public final class LightingAuthoringPanel extends JPanel {
    private final Path path;
    private final JComboBox<LightingModule.Preset> preset=
        new JComboBox<>(LightingModule.Preset.values());
    private final JComboBox<LightModel.Beam> beam=
        new JComboBox<>(LightModel.Beam.values());
    private final JSpinner ambient=new JSpinner(new SpinnerNumberModel(100,50,150,5));
    private final JSpinner brightness=new JSpinner(new SpinnerNumberModel(100,50,150,5));
    private final JSpinner reach=new JSpinner(new SpinnerNumberModel(100,65,140,5));
    private final JSpinner diffusion=new JSpinner(new SpinnerNumberModel(100,5,100,5));
    private final JSpinner flicker=new JSpinner(new SpinnerNumberModel(100,0,200,10));
    private final JSpinner cache=new JSpinner(new SpinnerNumberModel(100,50,150,10));
    private final JComboBox<Integer> cell=new JComboBox<>(new Integer[]{2,4,8});
    private final JCheckBox outdoor=new JCheckBox("Distant outdoor LOD");
    private final JCheckBox changed=new JCheckBox("Changed-only vertex update");
    private final JCheckBox reuse=new JCheckBox("Source dependency reuse");
    private final JLabel status=new JLabel("Lighting profile: authoring only");
    private final JPanel preview=new JPanel(){
        @Override protected void paintComponent(Graphics g0){
            super.paintComponent(g0);
            Graphics2D g=(Graphics2D)g0.create();
            try{
                g.setColor(new Color(12,19,29));g.fillRect(0,0,getWidth(),getHeight());
                LightingModule.Settings s=settings();
                final int w=getWidth(),h=getHeight();
                final double radius=Math.max(1.,Math.min(w,h)*.42*s.reachScale());
                final LightModel.Beam pattern=(LightModel.Beam)beam.getSelectedItem();
                for(int y=0;y<h;y+=7)for(int x=0;x<w;x+=7){
                    double dx=x-w*.50,dy=y-h*.55;
                    double distance=Math.hypot(dx,dy);
                    double angle=LightModel.angular(pattern,dy/Math.max(1.,distance));
                    double fade=LightModel.diffusionFade(distance,radius,
                        s.diffusionFraction());
                    double contribution=LightModel.attenuation(s.sourceScale(),
                        angle,fade,(distance/45.)*(distance/45.),1.);
                    int value=(int)Math.round(20*s.ambientScale()+
                        225*Math.min(1.,contribution));
                    g.setColor(new Color(Math.min(255,value),
                        Math.min(255,(int)(value*.91)),Math.min(255,(int)(value*.73))));
                    g.fillRect(x,y,7,7);
                }
                g.setColor(new Color(206,223,236));
                g.drawString("BECKENBURG SOURCE PROFILE / PREVIEW ONLY",10,19);
                g.drawString("No wall occlusion or in-game light baking",10,h-10);
            }finally{g.dispose();}
        }
    };
    private boolean updating;
    private boolean dirty;

    public LightingAuthoringPanel(Path file) throws IOException{
        super(new BorderLayout(4,4));
        path=file.toAbsolutePath().normalize();
        JPanel fields=new JPanel(new GridLayout(0,2,4,3));
        fields.add(new JLabel("Profile"));fields.add(preset);
        fields.add(new JLabel("Beam shape"));fields.add(beam);
        fields.add(new JLabel("Ambient %"));fields.add(ambient);
        fields.add(new JLabel("Source strength %"));fields.add(brightness);
        fields.add(new JLabel("Source reach %"));fields.add(reach);
        fields.add(new JLabel("Diffusion %"));fields.add(diffusion);
        fields.add(new JLabel("Flicker %"));fields.add(flicker);
        fields.add(new JLabel("Cache allowance %"));fields.add(cache);
        fields.add(new JLabel("Light cell size (m)"));fields.add(cell);
        fields.add(outdoor);fields.add(changed);
        fields.add(reuse);fields.add(new JLabel(""));
        JPanel actions=new JPanel(new FlowLayout(FlowLayout.LEFT,3,2));
        JButton save=new JButton("Save lighting");save.addActionListener(e->saveWithDialog());
        JButton reload=new JButton("Reload");reload.addActionListener(e->{
            try{load();}catch(IOException problem){status.setText("Load failed: "+problem.getMessage());}
        });
        actions.add(save);actions.add(reload);
        JPanel top=new JPanel(new BorderLayout());
        top.add(fields,BorderLayout.CENTER);top.add(actions,BorderLayout.SOUTH);
        add(new JScrollPane(top),BorderLayout.NORTH);
        preview.setPreferredSize(new Dimension(280,155));
        add(preview,BorderLayout.CENTER);
        add(status,BorderLayout.SOUTH);
        preset.addActionListener(e->{
            if(updating)return;
            LightingModule.Preset name=(LightingModule.Preset)preset.getSelectedItem();
            if(name!=LightingModule.Preset.CUSTOM){
                setSettings(LightingModule.preset(name));
            }
            onEdit();
        });
        for(JSpinner input:new JSpinner[]{ambient,brightness,reach,diffusion,flicker,cache})
            input.addChangeListener(e->customize());
        cell.addActionListener(e->customize());
        for(JCheckBox input:new JCheckBox[]{outdoor,changed,reuse})
            input.addActionListener(e->customize());
        beam.addActionListener(e->onEdit());
        load();
    }
    private int value(JSpinner input){return ((Number)input.getValue()).intValue();}
    private void customize(){
        if(updating)return;
        updating=true;preset.setSelectedItem(LightingModule.Preset.CUSTOM);updating=false;
        onEdit();
    }
    private void onEdit(){
        if(updating)return;
        settings(); // validate actual Beckenburg source contract
        dirty=true;
        status.setText("Unsaved lighting profile: "+path.getFileName());
        preview.repaint();
    }
    private LightingModule.Settings settings(){
        return new LightingModule.Settings(
            (LightingModule.Preset)preset.getSelectedItem(),
            value(ambient),value(brightness),value(reach),value(diffusion),
            value(flicker),value(cache),(Integer)cell.getSelectedItem(),
            outdoor.isSelected(),changed.isSelected(),reuse.isSelected());
    }
    private void setSettings(LightingModule.Settings s){
        updating=true;
        preset.setSelectedItem(s.preset());
        ambient.setValue(s.ambientPercent());
        brightness.setValue(s.sourcePercent());
        reach.setValue(s.reachPercent());
        diffusion.setValue(s.diffusionPercent());
        flicker.setValue(s.flickerPercent());
        cache.setValue(s.cacheBudgetPercent());
        cell.setSelectedItem(s.cellMetres());
        outdoor.setSelected(s.distantOutdoorLod());
        changed.setSelected(s.changedOnlyVertices());
        reuse.setSelected(s.dependencyReuse());
        updating=false;
        preview.repaint();
    }
    public Path documentPath(){return path;}
    public boolean dirty(){return dirty;}
    public void save() throws IOException{
        LightingModule.Settings s=settings();
        Properties p=new Properties();
        p.setProperty("schema","1");
        p.setProperty("preset",s.preset().name());
        p.setProperty("beam",((LightModel.Beam)beam.getSelectedItem()).name());
        p.setProperty("ambient",String.valueOf(s.ambientPercent()));
        p.setProperty("source",String.valueOf(s.sourcePercent()));
        p.setProperty("reach",String.valueOf(s.reachPercent()));
        p.setProperty("diffusion",String.valueOf(s.diffusionPercent()));
        p.setProperty("flicker",String.valueOf(s.flickerPercent()));
        p.setProperty("cache",String.valueOf(s.cacheBudgetPercent()));
        p.setProperty("cell",String.valueOf(s.cellMetres()));
        p.setProperty("outdoor",String.valueOf(s.distantOutdoorLod()));
        p.setProperty("changed",String.valueOf(s.changedOnlyVertices()));
        p.setProperty("reuse",String.valueOf(s.dependencyReuse()));
        Files.createDirectories(path.getParent());
        Path tmp=Files.createTempFile(path.getParent(),".lighting-",".tmp");
        try{
            try(OutputStream o=Files.newOutputStream(tmp)){p.store(o,"ReverieVR lighting authoring; not native gameplay");}
            try{Files.move(tmp,path,StandardCopyOption.REPLACE_EXISTING,
                StandardCopyOption.ATOMIC_MOVE);}
            catch(AtomicMoveNotSupportedException e){
                Files.move(tmp,path,StandardCopyOption.REPLACE_EXISTING);
            }
        }finally{Files.deleteIfExists(tmp);}
        dirty=false;status.setText("Saved authoring lighting profile: "+path);
    }
    public void load() throws IOException{
        if(!Files.isRegularFile(path)){
            setSettings(LightingModule.preset(LightingModule.Preset.BALANCED));
            beam.setSelectedItem(LightModel.Beam.OMNIDIRECTIONAL);
            dirty=false;status.setText("Balanced default (unsaved): "+path.getFileName());
            return;
        }
        Properties p=new Properties();
        try(InputStream in=Files.newInputStream(path)){p.load(in);}
        try{
            if(!"1".equals(p.getProperty("schema")))throw new IllegalArgumentException("Unsupported profile version");
            LightingModule.Preset mode=LightingModule.Preset.valueOf(p.getProperty("preset"));
            LightingModule.Settings proposed=new LightingModule.Settings(mode,
                Integer.parseInt(p.getProperty("ambient")),
                Integer.parseInt(p.getProperty("source")),
                Integer.parseInt(p.getProperty("reach")),
                Integer.parseInt(p.getProperty("diffusion")),
                Integer.parseInt(p.getProperty("flicker")),
                Integer.parseInt(p.getProperty("cache")),
                Integer.parseInt(p.getProperty("cell")),
                booleanValue(p,"outdoor"),booleanValue(p,"changed"),booleanValue(p,"reuse"));
            if(mode!=LightingModule.Preset.CUSTOM &&
                !proposed.equals(LightingModule.preset(mode)))
                throw new IllegalArgumentException("Invalid named lighting preset");
            LightModel.Beam savedBeam=LightModel.Beam.valueOf(p.getProperty("beam"));
            setSettings(proposed);
            updating=true;beam.setSelectedItem(savedBeam);updating=false;
        }catch(RuntimeException ex){
            throw new IOException("Invalid lighting file "+path+": "+ex.getMessage(),ex);
        }
        dirty=false;status.setText("Loaded lighting profile: "+path);
        preview.repaint();
    }
    private boolean booleanValue(Properties p,String key){
        String raw=p.getProperty(key);
        if(!"true".equals(raw)&&!"false".equals(raw))
            throw new IllegalArgumentException("Invalid boolean "+key);
        return Boolean.parseBoolean(raw);
    }
    private void saveWithDialog(){
        try{save();}catch(IOException|IllegalArgumentException ex){
            status.setText("Lighting save failed: "+ex.getMessage());
            JOptionPane.showMessageDialog(this,ex.toString(),"Lighting save failed",
                JOptionPane.ERROR_MESSAGE);
        }
    }
}
