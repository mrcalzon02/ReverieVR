package io.github.mrcalzon02.reverievr;

import javax.swing.*;
import org.beckenburg.editor.EditorHistory;
import org.beckenburg.editor.EditorPalettePanel;
import org.beckenburg.editor.EditorProject;
import javax.swing.event.ListSelectionEvent;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.Point2D;
import java.io.IOException;
import java.nio.file.*;
import java.util.*;
import java.util.List;

/** Actual desktop scene authoring workbench; NOT a gameplay/runtime port. */
public final class ReverieDesktop {
    private final Path repo, workspace;
    private final JFrame frame=new JFrame("ReverieVR - Desktop Development Workbench");
    private final JComboBox<String> project=new JComboBox<>();
    private final JComboBox<String> environment=new JComboBox<>();
    private final JTextField sitkaRegion=new JTextField("harbor",10);
    private final JComboBox<Double> snapStep=new JComboBox<>(new Double[]{.25, .5, 1., 2., 5., 10.});
    private final JPanel sitkaControls=new JPanel(new FlowLayout(FlowLayout.LEFT,8,2));
    private final JPanel paletteHost=new JPanel(new BorderLayout());
    private ReverieSceneProject editorProject;
    private LightingAuthoringPanel lightingPanel;
    private EditorHistory<String> editHistory;
    private final DefaultListModel<String> objects=new DefaultListModel<>();
    private final JList<String> list=new JList<>(objects);
    private final JLabel status=new JLabel("Ready");
    private final JSpinner[] fields={
        new JSpinner(new SpinnerNumberModel(0.0,-10000.0,10000.0,0.25)),
        new JSpinner(new SpinnerNumberModel(0.0,-10000.0,10000.0,0.25)),
        new JSpinner(new SpinnerNumberModel(0.0,-10000.0,10000.0,0.25)),
        new JSpinner(new SpinnerNumberModel(0.0,-3600.0,3600.0,5.0)),
        new JSpinner(new SpinnerNumberModel(1.0,0.01,500.0,0.1))
    };
    private final View view=new View();
    private JButton publishTest;
    private final Map<String,ObjMeshReader.Mesh> meshes=new HashMap<>();
    private SceneDocument scene=new SceneDocument();
    private String activeId="home";
    private boolean updating=false, dirty=false;
    private int nextId=1;

    private ReverieDesktop(Path repo,Path workspace) throws IOException {
        this.repo=repo.toRealPath(); this.workspace=workspace.toAbsolutePath().normalize();
        if(!Files.isRegularFile(this.repo.resolve("app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java")))
            throw new IOException("Select the ReverieVR repository root, not an arbitrary directory");
        Files.createDirectories(this.workspace.resolve("scenes"));
        project.addItem("home");
        for(GameModuleRegistry.Entry e:GameModuleRegistry.GAMES) project.addItem(e.id);
        project.addItem("sitka"); // Authoring only, not a native playable entry.
        for(HomeEnvironment e:HomeEnvironment.values()) environment.addItem(e.preferenceValue);
        configureWindow();
        load("home");
    }
    private void configureWindow(){
        frame.setDefaultCloseOperation(WindowConstants.DO_NOTHING_ON_CLOSE);
        frame.addWindowListener(new WindowAdapter(){@Override public void windowClosing(WindowEvent e){if(confirmDiscard())frame.dispose();}});
        JPanel upper=new JPanel(new FlowLayout(FlowLayout.LEFT,8,6));
        upper.add(new JLabel("Project")); upper.add(project);
        upper.add(new JLabel("Home biome")); upper.add(environment);
        sitkaControls.add(new JLabel("Sitka editing region"));
        sitkaControls.add(sitkaRegion);
        sitkaControls.add(button("Open / create region",this::openSitkaRegion));
        sitkaControls.add(button("Water and weather editor",this::launchSitkaEnvironment));
        sitkaControls.add(new JLabel("Region blockout / authoring only"));
        sitkaControls.setVisible(false);
        JButton save=button("Save scene",this::save); upper.add(save);
        upper.add(new JLabel("Snap (m)"));upper.add(snapStep);
        publishTest=button("Stage Test Chamber for APK",this::publishTestChamber);
        publishTest.setToolTipText("Copies only box/cylinder layout into the Test Chamber APK assets; rebuild the Android app to see changes");
        publishTest.setEnabled(false);upper.add(publishTest);
        JButton revert=button("Reload",()->{if(confirmDiscard())load(activeId);});upper.add(revert);
        JButton reset=button("Reset view",()->{view.yaw=.55;view.pitch=.43;view.distance=20;view.repaint();});upper.add(reset);
        project.addActionListener(e->{if(updating)return;String target=(String)project.getSelectedItem();
            if(target==null||target.equals(activeId))return;
            if(!confirmDiscard()){updating=true;project.setSelectedItem(activeId);updating=false;return;}
            load(target);
        });
        environment.addActionListener(e->{if(updating)return;editHistory.execute(()->{scene.environment=(String)environment.getSelectedItem();changed();});});
        JPanel right=new JPanel(new BorderLayout(4,4));
        right.setPreferredSize(new Dimension(312,740));
        JPanel add=new JPanel(new GridLayout(3,2,4,4));
        add.add(button("Add box",()->add("box","-")));
        add.add(button("Add cylinder",()->add("cylinder","-")));
        add.add(button("Import OBJ",this::importObj));
        add.add(button("Delete selected",this::delete));
        add.add(button("Duplicate selected",this::duplicate));
        add.add(button("Snap selected",this::snapSelected));
        right.add(add,BorderLayout.NORTH);
        list.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        list.addListSelectionListener((ListSelectionEvent e)->{if(!e.getValueIsAdjusting())refreshFields();});
        JScrollPane items=new JScrollPane(list);items.setBorder(BorderFactory.createTitledBorder("Scene objects (real editable transforms)"));
        JSplitPane hierarchyAndAssets=new JSplitPane(JSplitPane.VERTICAL_SPLIT,items,paletteHost);
        hierarchyAndAssets.setResizeWeight(.6);
        hierarchyAndAssets.setDividerLocation(.6);
        right.add(hierarchyAndAssets,BorderLayout.CENTER);
        JPanel transforms=new JPanel(new GridLayout(0,2,5,5));
        String[] labels={"X","Y (height)","Z","Yaw degrees","Scale"};
        for(int i=0;i<fields.length;i++){
            final int index=i;
            transforms.add(new JLabel(labels[i]));transforms.add(fields[i]);
            fields[i].addChangeListener(e->{if(!updating)editTransform(index);});
        }
        JPanel lower=new JPanel(new BorderLayout(4,4));
        lower.setBorder(BorderFactory.createTitledBorder("Selected transform"));
        lower.add(transforms,BorderLayout.CENTER);
        JPanel history=new JPanel(new GridLayout(1,2,4,4));
        history.add(button("Undo",()->{if(editHistory!=null)editHistory.undo();}));
        history.add(button("Redo",()->{if(editHistory!=null)editHistory.redo();}));
        lower.add(history,BorderLayout.SOUTH);
        right.add(lower,BorderLayout.SOUTH);
        JPanel main=new JPanel(new BorderLayout(5,5));
        JPanel header=new JPanel(new BorderLayout());header.add(upper,BorderLayout.NORTH);
        header.add(sitkaControls,BorderLayout.SOUTH);main.add(header,BorderLayout.NORTH);
        JSplitPane split=new JSplitPane(JSplitPane.HORIZONTAL_SPLIT,view,right);
        split.setResizeWeight(.74);main.add(split,BorderLayout.CENTER);
        status.setBorder(BorderFactory.createEmptyBorder(4,8,4,8));main.add(status,BorderLayout.SOUTH);
        frame.setContentPane(main);frame.setSize(1300,820);frame.setMinimumSize(new Dimension(800,550));
        frame.setLocationRelativeTo(null);
        frame.getRootPane().getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW).put(KeyStroke.getKeyStroke("control S"),"save");
        frame.getRootPane().getActionMap().put("save",new AbstractAction(){@Override public void actionPerformed(ActionEvent e){save();}});
        frame.getRootPane().getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW).put(KeyStroke.getKeyStroke("control Z"),"undo");
        frame.getRootPane().getActionMap().put("undo",new AbstractAction(){@Override public void actionPerformed(ActionEvent e){if(editHistory!=null)editHistory.undo();}});
        frame.getRootPane().getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW).put(KeyStroke.getKeyStroke("control Y"),"redo");
        frame.getRootPane().getActionMap().put("redo",new AbstractAction(){@Override public void actionPerformed(ActionEvent e){if(editHistory!=null)editHistory.redo();}});
    }
    private JButton button(String title,Runnable action){JButton b=new JButton(title);b.addActionListener(e->action.run());return b;}
    static Path sceneFile(Path workspace,String id,String region){
        if("sitka".equals(id)) {
            if(region==null || !region.matches("[a-z0-9][a-z0-9-]{0,47}"))
                throw new IllegalArgumentException(
                    "Region must be 1-48 lowercase letters, numbers or dashes");
            return workspace.resolve("scenes").resolve("sitka")
                .resolve(region+".rvscene");
        }
        if(id==null || !id.matches("[a-z0-9-]{3,80}"))
            throw new IllegalArgumentException("Invalid editor project");
        return workspace.resolve("scenes").resolve(id+".rvscene");
    }
    private Path file(String id){return sceneFile(workspace,id,sitkaRegion.getText().trim());}
    private Path lightingFile(String id){
        if("sitka".equals(id))
            return workspace.resolve("sitka-environment/lighting/sitka.properties");
        return workspace.resolve("lighting").resolve(id+".properties");
    }
    private void openSitkaRegion(){
        if(!"sitka".equals(activeId)||!confirmDiscard())return;
        try{file("sitka");load("sitka");}
        catch(IllegalArgumentException error){this.error("Invalid Sitka region",error);}
    }
    private void launchSitkaEnvironment(){
        if(!"sitka".equals(activeId))return;
        // Start the ACTUAL water/weather authoring program, not an inert
        // simulation button. Its independent documents never fake terrain.
        String win=System.getProperty("os.name","").toLowerCase(Locale.ROOT);
        Path launcher=repo.resolve("tools/sitka/")
            .resolve(win.contains("win")?"launch-editor.cmd":"launch-editor.sh");
        Path folder=workspace.resolve("sitka-environment");
        try{
            Files.createDirectories(folder);
            ProcessBuilder start=win.contains("win")
                ? new ProcessBuilder("cmd","/c",launcher.toString(),folder.toString())
                : new ProcessBuilder("bash",launcher.toString(),folder.toString());
            start.directory(repo.toFile());
            start.redirectErrorStream(true);
            start.redirectOutput(ProcessBuilder.Redirect.appendTo(
                workspace.resolve("sitka-editor-launch.log").toFile()));
            Process child=start.start();
            status.setText("Started Sitka water/weather editor process "+
                child.pid()+" | log: "+workspace.resolve("sitka-editor-launch.log"));
        }catch(IOException failure){error("Could not launch Sitka environment editor",failure);}
    }
    private void restoreSnapshot(SceneDocument restored){
        scene=restored;
        updating=true;environment.setSelectedItem(scene.environment);updating=false;
        refreshObjects(Math.min(list.getSelectedIndex(),scene.pieces.size()-1));
        changed();
    }
    private void load(String id){
        try{
            Path target=file(id);
            SceneDocument found=SceneDocument.load(target);
            scene=found;activeId=id;nextId=1;meshes.clear();dirty=false;
            editorProject=new ReverieSceneProject(
                id,target,()->scene,this::restoreSnapshot);
            editHistory=new EditorHistory<>(editorProject,64);
            updating=true;project.setSelectedItem(id);
            environment.setSelectedItem(scene.environment);updating=false;
            sitkaControls.setVisible("sitka".equals(id));
            paletteHost.removeAll();
            EditorPalettePanel palette=new EditorPalettePanel(editorProject,
                this::choosePalette);
            paletteHost.setBorder(BorderFactory.createTitledBorder(
                "Beckenburg shared asset palette"));
            lightingPanel=new LightingAuthoringPanel(lightingFile(id));
            JTabbedPane authoringDocks=new JTabbedPane();
            authoringDocks.addTab("Objects",palette);
            authoringDocks.addTab("Lighting",lightingPanel);
            paletteHost.add(authoringDocks,BorderLayout.CENTER);
            paletteHost.revalidate();paletteHost.repaint();
            refreshObjects(-1);
            publishTest.setEnabled("procedural-test-chamber".equals(activeId));
            status.setText("Editor source / non-playable preview: "+target);view.repaint();
        }catch(IOException|IllegalArgumentException ex){
            updating=true;project.setSelectedItem(activeId);updating=false;
            error("Scene load failed",ex);
        }
    }
    private void choosePalette(EditorProject.Asset asset){
        String uri=asset.uri();
        switch(uri){
            case "reverie://primitive/box":
            case "reverie://sitka/building":
            case "reverie://sitka/shore":
            case "reverie://ledger/counter":add("box","-");return;
            case "reverie://primitive/cylinder":
            case "reverie://sitka/piling":
            case "reverie://ledger/stool":add("cylinder","-");return;
            case "reverie://asset/import-obj":importObj();return;
            default: throw new IllegalArgumentException("Unknown palette command: "+uri);
        }
    }
    private void save(){try{
            editorProject.save();dirty=false;
            status.setText("Saved "+editorProject.documentPath()+
                " (authoring scene, not yet deployed to its game)");
        }catch(IOException|IllegalArgumentException ex){error("Scene save failed",ex);}}
    private void publishTestChamber(){
        if(!"procedural-test-chamber".equals(activeId))return;
        Path dest=repo.resolve("app/src/main/assets/native/test-chamber/scene.rvscene");
        try{
            if(scene.pieces.stream().anyMatch(p->p.kind.equals("obj")))
                throw new IOException("OBJ references cannot render in the native Test Chamber yet. Remove them or use primitives.");
            String encoded=scene.encode();
            if(encoded.getBytes(java.nio.charset.StandardCharsets.UTF_8).length>150000)
                throw new IOException("Native scene exceeds its 150000-byte size limit");
            if(Files.exists(dest)){
                int approval=JOptionPane.showConfirmDialog(frame,
                    "Replace the bundled Test Chamber scene? The next full APK build will include this layout.",
                    "Stage native scene",JOptionPane.YES_NO_OPTION,JOptionPane.WARNING_MESSAGE);
                if(approval!=JOptionPane.YES_OPTION)return;
            }
            scene.save(file(activeId));
            scene.save(dest);
            dirty=false;
            status.setText("Staged "+scene.pieces.size()+" objects: "+repo.relativize(dest)+" — rebuild APK to test in VR");
        }catch(IOException|IllegalArgumentException ex){error("Test Chamber scene staging failed",ex);}
    }
    private boolean confirmDiscard(){if(!dirty && (lightingPanel==null || !lightingPanel.dirty()))return true;
        int choice=JOptionPane.showConfirmDialog(frame,"Discard unsaved scene or lighting edits to "+activeId+"?","Unsaved scene",JOptionPane.YES_NO_OPTION,JOptionPane.WARNING_MESSAGE);
        return choice==JOptionPane.YES_OPTION;
    }
    private void error(String message,Exception ex){status.setText(message+": "+ex.getMessage());JOptionPane.showMessageDialog(frame,ex.toString(),message,JOptionPane.ERROR_MESSAGE);}
    private void changed(){dirty=true;status.setText("Unsaved edits: "+activeId+
        " | "+editorProject.documentPath()+" (authoring only)");view.repaint();}
    private void refreshObjects(int selected){updating=true;objects.clear();for(EditorProject.Node p:editorProject.hierarchy())objects.addElement(p.id()+" ["+p.kind()+"]");
        list.setSelectedIndex(selected);updating=false;refreshFields();view.repaint();}
    private SceneDocument.Piece selected(){int i=list.getSelectedIndex();return i<0||i>=scene.pieces.size()?null:scene.pieces.get(i);}
    private void refreshFields(){SceneDocument.Piece p=selected();updating=true;
        for(int i=0;i<fields.length;i++){
            fields[i].setEnabled(p!=null);
            if(p!=null)fields[i].setValue(new double[]{p.x,p.y,p.z,p.yaw,p.scale}[i]);
        }updating=false;view.repaint();}
    private void editTransform(int index){SceneDocument.Piece p=selected();if(p==null)return;
        double value=((Number)fields[index].getValue()).doubleValue();
        if(Double.compare(value,new double[]{p.x,p.y,p.z,p.yaw,p.scale}[index])==0)return;
        try{editHistory.execute(()->{
            switch(index){case 0:p.x=value;break;case 1:p.y=value;break;
                case 2:p.z=value;break;case 3:p.yaw=value;break;case 4:p.scale=value;break;}
            p.validate();changed();
        });}catch(IllegalArgumentException error){this.error("Invalid transform",error);}
    }
    private void add(String kind,String asset){if(scene.pieces.size()>=SceneDocument.MAX_OBJECTS){status.setText("Scene limit reached");return;}
        try{editHistory.execute(()->{
            String id;do{id="object-"+(nextId++);}while(hasId(id));
            SceneDocument.Piece p=new SceneDocument.Piece(id,kind,asset,0,0,0,0,1);
            scene.pieces.add(p);refreshObjects(scene.pieces.size()-1);changed();
        });}
        catch(IllegalArgumentException ex){error("Unable to add object",ex);}
    }
    private boolean hasId(String id){return scene.pieces.stream().anyMatch(p->p.id.equals(id));}
    private void delete(){int i=list.getSelectedIndex();if(i<0)return;editHistory.execute(()->{scene.pieces.remove(i);refreshObjects(Math.min(i,scene.pieces.size()-1));changed();});}
    private void duplicate(){
        SceneDocument.Piece origin=selected();
        if(origin==null||scene.pieces.size()>=SceneDocument.MAX_OBJECTS)return;
        editHistory.execute(()->{
            String id;do{id="object-"+(nextId++);}while(hasId(id));
            SceneDocument.Piece copy=new SceneDocument.Piece(
                id,origin.kind,origin.asset,origin.x+1,origin.y,origin.z+1,
                origin.yaw,origin.scale);
            scene.pieces.add(copy);
            refreshObjects(scene.pieces.size()-1);
            changed();
        });
    }
    private void snapSelected(){
        SceneDocument.Piece p=selected();if(p==null)return;
        double step=((Number)snapStep.getSelectedItem()).doubleValue();
        editHistory.execute(()->{
            p.x=Math.rint(p.x/step)*step;
            p.y=Math.rint(p.y/step)*step;
            p.z=Math.rint(p.z/step)*step;
            p.validate();
            refreshFields();
            changed();
        });
    }
    private void importObj(){
        JFileChooser chooser=new JFileChooser(repo.resolve("app/src/main/assets").toFile());
        chooser.setDialogTitle("Choose an OBJ inside the ReverieVR source checkout");
        if(chooser.showOpenDialog(frame)!=JFileChooser.APPROVE_OPTION)return;
        try{Path real=chooser.getSelectedFile().toPath().toRealPath();
            if(!real.startsWith(repo))throw new IOException("For reproducible game assets, OBJ must be inside the repository");
            String relative=repo.relativize(real).toString().replace('\\','/');
            if(!SceneDocument.safeAsset(relative))throw new IOException("Only repository-relative .obj meshes are supported");
            readMesh(relative);add("obj",relative);
        }catch(IOException ex){error("Import failed",ex);}
    }
    private ObjMeshReader.Mesh readMesh(String asset)throws IOException{
        if(meshes.containsKey(asset))return meshes.get(asset);
        if(!SceneDocument.safeAsset(asset))throw new IOException("Invalid OBJ path");
        Path real=repo.resolve(asset).toRealPath();if(!real.startsWith(repo))throw new IOException("OBJ escapes checkout");
        if(Files.size(real)>20_000_000)throw new IOException("OBJ exceeds preview limit");
        try(var source=Files.newBufferedReader(real)){
            ObjMeshReader.Mesh mesh=ObjMeshReader.read(source);meshes.put(asset,mesh);return mesh;
        }
    }
    private final class View extends JPanel{
        double yaw=.55,pitch=.43,distance=20;
        int lastX,lastY;boolean dragging=false;
        View(){setOpaque(true);setBackground(new Color(9,18,29));setToolTipText("Drag: orbit | Wheel: zoom | Click object: select");
            MouseAdapter mouse=new MouseAdapter(){
                @Override public void mousePressed(MouseEvent e){lastX=e.getX();lastY=e.getY();dragging=false;}
                @Override public void mouseDragged(MouseEvent e){yaw+=(e.getX()-lastX)*.008;pitch=Math.max(-1.35,Math.min(1.35,pitch+(e.getY()-lastY)*.008));lastX=e.getX();lastY=e.getY();dragging=true;repaint();}
                @Override public void mouseReleased(MouseEvent e){if(!dragging)pick(e.getX(),e.getY());}
                @Override public void mouseWheelMoved(MouseWheelEvent e){distance=Math.max(3,Math.min(130,distance*Math.pow(1.11,e.getPreciseWheelRotation())));repaint();}
            };addMouseListener(mouse);addMouseMotionListener(mouse);addMouseWheelListener(mouse);
        }
        Point2D.Double project(double x,double y,double z){
            double c=Math.cos(yaw),s=Math.sin(yaw),cp=Math.cos(pitch),sp=Math.sin(pitch);
            double horizontal=c*x-s*z,depth=s*x+c*z;
            double vertical=cp*y-sp*depth, cameraDepth=sp*y+cp*depth+distance;
            if(cameraDepth<.1)return null;
            double focal=Math.min(getWidth(),getHeight())*.87;
            return new Point2D.Double(getWidth()*.5+focal*horizontal/cameraDepth,getHeight()*.58-focal*vertical/cameraDepth);
        }
        void pick(int x,int y){double best=24*24;int candidate=-1;
            for(int i=0;i<scene.pieces.size();i++){SceneDocument.Piece p=scene.pieces.get(i);Point2D.Double v=project(p.x,p.y,p.z);
                if(v!=null){double d=v.distanceSq(x,y);if(d<best){best=d;candidate=i;}}}
            if(candidate>=0)list.setSelectedIndex(candidate);
        }
        void line(Graphics2D g,double a,double b,double c,double x,double y,double z){Point2D p=project(a,b,c),q=project(x,y,z);if(p!=null&&q!=null)g.drawLine((int)p.getX(),(int)p.getY(),(int)q.getX(),(int)q.getY());}
        @Override protected void paintComponent(Graphics g0){super.paintComponent(g0);Graphics2D g=(Graphics2D)g0.create();
            try{g.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
                g.setColor(new Color(24,40,55));for(int i=-20;i<=20;i+=2){line(g,i,0,-20,i,0,20);line(g,-20,0,i,20,0,i);}
                g.setStroke(new BasicStroke(2));g.setColor(new Color(227,100,102));line(g,0,0,0,5,0,0);
                g.setColor(new Color(103,215,155));line(g,0,0,0,0,5,0);
                g.setColor(new Color(79,166,242));line(g,0,0,0,0,0,5);
                for(int i=0;i<scene.pieces.size();i++)drawPiece(g,scene.pieces.get(i),i==list.getSelectedIndex());
                g.setFont(new Font(Font.MONOSPACED,Font.BOLD,14));g.setColor(new Color(224,236,248));
                g.drawString("REVERIEVR  /  SCENE WORKBENCH",18,27);
                g.setFont(new Font(Font.MONOSPACED,Font.PLAIN,12));g.setColor(new Color(146,183,202));
                g.drawString("AUTHORING VIEW  -  NOT ANDROID RENDERER",18,47);
                g.drawString("Orbit: drag   Zoom: wheel   Select: click",18,getHeight()-20);
            }finally{g.dispose();}
        }
        void drawPiece(Graphics2D g,SceneDocument.Piece p,boolean focused){
            g.setStroke(new BasicStroke(focused?2.5f:1.2f));g.setColor(focused?new Color(250,215,112):new Color(91,186,219));
            if(p.kind.equals("obj")){
                try{ObjMeshReader.Mesh mesh=readMesh(p.asset);float[] t=mesh.triangles;
                    // Bounded wireframe preview; the authored original remains unchanged.
                    for(int i=0;i<t.length&&i<9*15000;i+=9){
                        for(int j=0;j<3;j++){int k=(j+1)%3;
                            double[] a=transform(t[i+j*3],t[i+j*3+1],t[i+j*3+2],p);
                            double[] b=transform(t[i+k*3],t[i+k*3+1],t[i+k*3+2],p);
                            line(g,a[0],a[1],a[2],b[0],b[1],b[2]);}
                    }
                }catch(IOException error){Point2D center=project(p.x,p.y,p.z);if(center!=null)g.drawString("OBJ unavailable: "+p.id,(int)center.getX(),(int)center.getY());}
            }else if(p.kind.equals("box")){
                double[][] pts={{-1,0,-1},{1,0,-1},{1,0,1},{-1,0,1},{-1,2,-1},{1,2,-1},{1,2,1},{-1,2,1}};
                for(int i=0;i<4;i++){edge(g,pts[i],pts[(i+1)%4],p);edge(g,pts[i+4],pts[(i+1)%4+4],p);edge(g,pts[i],pts[i+4],p);}
            }else{
                for(int i=0;i<16;i++){double a=2*Math.PI*i/16,b=2*Math.PI*(i+1)/16;
                    edge(g,new double[]{Math.cos(a),0,Math.sin(a)},new double[]{Math.cos(b),0,Math.sin(b)},p);
                    edge(g,new double[]{Math.cos(a),2,Math.sin(a)},new double[]{Math.cos(b),2,Math.sin(b)},p);
                    edge(g,new double[]{Math.cos(a),0,Math.sin(a)},new double[]{Math.cos(a),2,Math.sin(a)},p);}
            }
            if(focused){Point2D center=project(p.x,p.y,p.z);if(center!=null)g.drawString(p.id,(int)center.getX()+8,(int)center.getY()-8);}
        }
        double[] transform(double x,double y,double z,SceneDocument.Piece p){double r=Math.toRadians(p.yaw),c=Math.cos(r),s=Math.sin(r);
            return new double[]{p.x+p.scale*(x*c-z*s),p.y+p.scale*y,p.z+p.scale*(x*s+z*c)};}
        void edge(Graphics2D g,double[] a,double[] b,SceneDocument.Piece p){double[] x=transform(a[0],a[1],a[2],p),y=transform(b[0],b[1],b[2],p);line(g,x[0],x[1],x[2],y[0],y[1],y[2]);}
    }
    public static void main(String[] args) throws Exception{
        if(args.length<1||args.length>2){System.err.println("Usage: ReverieDesktop <ReverieVR-repository-root> [workspace]");System.exit(2);}
        if(GraphicsEnvironment.isHeadless())throw new IllegalStateException("A desktop graphical environment is required");
        Path root=Path.of(args[0]);Path workspace=args.length==2?Path.of(args[1]):root.resolve("build/desktop-workspace");
        SwingUtilities.invokeLater(()->{
            try{new ReverieDesktop(root,workspace).frame.setVisible(true);}
            catch(Exception error){JOptionPane.showMessageDialog(null,error.getMessage(),"Desktop workbench failed",JOptionPane.ERROR_MESSAGE);}
        });
    }
}