package io.github.mrcalzon02.reverievr;

import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import javax.swing.Timer;
import javax.swing.WindowConstants;
import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseWheelEvent;
import java.awt.geom.AffineTransform;
import java.awt.geom.NoninvertibleTransformException;
import java.awt.geom.Point2D;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.nio.file.Path;
import java.util.List;

/** Real offline desktop Home/player prototype. Does not pretend to host Android JNI modules. */
public final class ReverieDesktopPlayer {
    private static final int WALL_WIDTH=1024, WALL_HEIGHT=700;
    private static final Color INK=new Color(7,18,30);
    private static final Color BOARD=new Color(15,33,48);
    private static final Color FRAME=new Color(68,143,194);
    private static final Color PAPER=new Color(214,237,245);
    private static final Color GOLD=new Color(247,201,105);
    private static final int[] COLORS={0x568DCA,0xC96782,0x3EA88F,0xAE8CDE,0xE2AF55,0xBAC8D3};
    private final DesktopHubState state;
    private final JFrame frame=new JFrame("ReverieVR — Desktop Home (development player)");
    private final Surface surface=new Surface();
    private final BufferedImage wall=new BufferedImage(WALL_WIDTH,WALL_HEIGHT,BufferedImage.TYPE_INT_ARGB);
    private final boolean[] keys=new boolean[1024];
    private DesktopArcade game;
    private boolean quick,searchFocused,mouseDragging;
    private int lastMouseX,lastMouseY;
    private long lastTick=System.nanoTime();
    private AffineTransform wallTransform=new AffineTransform();
    private AffineTransform keyboardTransform=new AffineTransform();
    private boolean wallVisible;
    private String notice="";
    private long noticeUntil;

    private ReverieDesktopPlayer(Path workspace) throws IOException {
        state=new DesktopHubState(workspace);
        try {state.load();}catch(IOException ex){notice="Settings ignored: "+ex.getMessage();noticeUntil=System.nanoTime()+8_000_000_000L;}
        frame.setDefaultCloseOperation(WindowConstants.DO_NOTHING_ON_CLOSE);
        frame.addWindowListener(new java.awt.event.WindowAdapter() {
            @Override public void windowClosing(java.awt.event.WindowEvent e) {exit();}
        });
        frame.setContentPane(surface);
        frame.setMinimumSize(new Dimension(780,510));frame.setSize(1260,800);
        frame.setLocationByPlatform(true);
        surface.setFocusable(true);
        new Timer(16,e->{tick();surface.repaint();}).start();
    }

    private void show(){frame.setVisible(true);surface.requestFocusInWindow();}
    private void exit(){
        try{state.save();}catch(IOException ex){System.err.println("ReverieVR desktop save: "+ex.getMessage());}
        frame.dispose();System.exit(0);
    }
    private void tick(){
        long now=System.nanoTime();double dt=Math.max(0,Math.min(.1,(now-lastTick)/1.0e9));lastTick=now;
        if(game!=null){game.update(dt);return;}
        double forward=(isDown(KeyEvent.VK_W)||isDown(KeyEvent.VK_UP)?1:0)
            -(isDown(KeyEvent.VK_S)||isDown(KeyEvent.VK_DOWN)?1:0);
        double strafe=(isDown(KeyEvent.VK_D)?1:0)-(isDown(KeyEvent.VK_A)?1:0);
        if(searchFocused||quick)forward=strafe=0;
        state.move(strafe,forward,dt);
        if(!searchFocused&&!quick){
            if(isDown(KeyEvent.VK_Q))state.turn(-dt*1.4);
            if(isDown(KeyEvent.VK_E))state.turn(dt*1.4);
        }
    }
    private boolean isDown(int key){return key>=0&&key<keys.length&&keys[key];}

    private void notifyUser(String message){notice=message;noticeUntil=System.nanoTime()+5_000_000_000L;}
    private void openGame(String id){
        if(!state.launch(id)) {notifyUser("That game has not been ported to desktop.");return;}
        game=new DesktopArcade(id,System.nanoTime());
        searchFocused=false;quick=false;
        java.util.Arrays.fill(keys,false);
    }
    private void back(){
        if(searchFocused){searchFocused=false;return;}
        if(quick){quick=false;return;}
        if(game!=null){game=null;state.back();return;}
        if(state.panel!=DesktopHubState.Panel.NONE){state.back();return;}
        quick=true;
    }
    private void homeAction(String command){
        switch(command){
            case "games":state.open(DesktopHubState.Panel.GAMES);searchFocused=false;quick=false;break;
            case "environment":state.environment=HomeEnvironment.values()[(state.environment.ordinal()+1)%HomeEnvironment.values().length];break;
            case "wardrobe":state.open(DesktopHubState.Panel.WARDROBE);quick=false;break;
            case "settings":state.open(DesktopHubState.Panel.SETTINGS);quick=false;break;
            case "help":state.open(DesktopHubState.Panel.HELP);quick=false;break;
            case "quick":quick=!quick;break;
            case "recenter":state.recenter();break;
            case "search":state.open(DesktopHubState.Panel.GAMES);searchFocused=true;quick=false;break;
            case "exit":exit();break;
        }
    }

    private static void text(Graphics2D g,String text,int x,int y,int size,Color color){
        g.setColor(color);g.setFont(new Font(Font.MONOSPACED,Font.BOLD,size));g.drawString(text,x,y);
    }
    private static void box(Graphics2D g,int x,int y,int w,int h,Color color){
        g.setColor(color);g.fillRoundRect(x,y,w,h,12,12);
        g.setColor(FRAME);g.setStroke(new BasicStroke(2));g.drawRoundRect(x,y,w,h,12,12);
    }
    private void drawButton(Graphics2D g,String label,int x,int y,int w,int h){
        box(g,x,y,w,h,BOARD);
        g.setFont(new Font(Font.MONOSPACED,Font.BOLD,18));
        while(g.getFontMetrics().stringWidth(label)>w-16&&g.getFont().getSize()>12)
            g.setFont(g.getFont().deriveFont((float)g.getFont().getSize()-1));
        g.setColor(PAPER);g.drawString(label,x+(w-g.getFontMetrics().stringWidth(label))/2,y+h/2+6);
    }
    private void paintWall(){
        Graphics2D g=wall.createGraphics();
        try{
            g.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
            g.setColor(new Color(9,22,35));g.fillRect(0,0,WALL_WIDTH,WALL_HEIGHT);
            box(g,13,32,232,621,new Color(13,28,43));
            box(g,258,32,500,621,new Color(11,26,39));
            box(g,771,32,240,621,new Color(13,28,43));
            text(g,"REVERIE VR",24,81,25,GOLD);
            text(g,"DESKTOP HOME",24,114,16,PAPER);
            drawButton(g,"Native Games",30,159,199,59);
            drawButton(g,"Change Room",30,237,199,59);
            drawButton(g,"Wardrobe",30,315,199,59);
            drawButton(g,"Settings",30,393,199,59);
            drawButton(g,"Controls / Help",30,471,199,59);
            drawButton(g,"Exit",30,549,199,59);
            text(g,"QUICK OPTIONS",784,92,20,GOLD);
            drawButton(g,"Quick Menu",784,146,212,56);
            drawButton(g,"Recenter",784,225,212,56);
            drawButton(g,"Search Games",784,304,212,56);
            text(g,"OFFLINE DESKTOP",786,421,17,PAPER);
            text(g,"HMD not required",786,451,16,new Color(142,184,209));
            text(g,"Native gameplay ports",786,492,14,new Color(142,184,209));
            text(g,DesktopHubState.PORTED_IDS.size()+" of "+GameModuleRegistry.GAMES.length+" available",786,522,17,GOLD);
            text(g,"WINDOWED / INPUT",786,565,14,new Color(142,184,209));
            text(g,"Mouse / Keyboard",786,593,16,PAPER);
            center(g);
        }finally{g.dispose();}
    }
    private void center(Graphics2D g){
        if(state.panel==DesktopHubState.Panel.NONE){
            text(g,"WELCOME TO REVERIEVR",285,119,29,PAPER);
            text(g,state.environment.displayName,285,155,20,GOLD);
            g.setColor(new Color(29,59,79));g.fillRoundRect(295,192,426,287,9,9);
            g.setColor(new Color(78,172,198));g.setStroke(new BasicStroke(2));g.drawRoundRect(295,192,426,287,9,9);
            g.setColor(new Color(18,38,52));g.fillOval(361,222,270,210);
            g.setColor(new Color(COLORS[state.outfit]));g.fillOval(446,258,88,120);
            g.setColor(new Color(202,225,228));g.fillOval(465,237,50,52);
            text(g,"A HOME THAT STAYS WITH YOU",318,530,18,PAPER);
            text(g,"Choose Native Games to open the library here",282,562,15,new Color(149,193,205));
        }else if(state.panel==DesktopHubState.Panel.GAMES)drawGames(g);
        else if(state.panel==DesktopHubState.Panel.WARDROBE)drawWardrobe(g);
        else if(state.panel==DesktopHubState.Panel.SETTINGS)drawSettings(g);
        else drawHelp(g);
    }
    private void drawGames(Graphics2D g){
        text(g,"NATIVE GAMES",275,88,27,PAPER);
        text(g,"DESKTOP PLAYABLE  "+state.availableGames().size()+" / "+GameModuleRegistry.GAMES.length,
            275,124,17,GOLD);
        drawButton(g,searchFocused?"Search: "+state.query+"_":"Search: "+state.query+"   [/]",
            275,135,470,49);
        List<GameModuleRegistry.Entry> ready=state.availableGames();
        int first=state.page*DesktopHubState.PAGE_SIZE;
        for(int slot=0;slot<DesktopHubState.PAGE_SIZE;slot++){
            int index=first+slot;
            if(index>=ready.size())continue;
            int x=275+(slot%2)*238,y=197+(slot/2)*166;
            drawGameCard(g,ready.get(index),x,y);
        }
        if(state.pageCount()>1){
            drawButton(g,"PREV",275,532,110,52);
            drawButton(g,"NEXT",398,532,110,52);
        }
        drawButton(g,"CLOSE",520,532,225,52);
        text(g,"The Android app and other unported games are unchanged.",275,607,13,new Color(143,179,197));
    }
    private void drawGameCard(Graphics2D g,GameModuleRegistry.Entry entry,int x,int y){
        box(g,x,y,226,155,new Color(11,27,42));
        if(entry.id.equals(DesktopHubState.PONG)){
            g.setColor(new Color(77,170,208));g.fillRect(x+18,y+28,10,60);g.fillRect(x+200,y+57,10,60);
            g.setColor(GOLD);g.fillOval(x+110,y+62,15,15);
        }else if(entry.id.equals(DesktopHubState.SNAKE)){
            g.setColor(new Color(85,210,159));
            for(int i=0;i<6;i++)g.fillRoundRect(x+47+i*24,y+50+i%2*18,22,22,5,5);
            g.setColor(new Color(236,121,139));g.fillOval(x+165,y+49,18,18);
        }else{
            g.setColor(GOLD);for(int col=0;col<5;col++)g.fillRect(x+25+col*35,y+40,29,15);
            g.setColor(new Color(79,181,217));g.fillRect(x+85,y+94,58,8);
            g.setColor(PAPER);g.fillOval(x+103,y+78,12,12);
        }
        text(g,entry.displayName,x+14,y+128,19,PAPER);
        text(g,"PLAY",x+174,y+143,13,new Color(94,220,167));
    }
    private void drawWardrobe(Graphics2D g){
        text(g,"AVATAR WARDROBE",275,112,28,PAPER);
        text(g,"Choose a desktop Home outfit color",275,165,18,GOLD);
        for(int i=0;i<COLORS.length;i++){
            int x=297+(i%3)*140,y=200+(i/3)*133;
            g.setColor(new Color(COLORS[i]));g.fillRoundRect(x,y,105,90,12,12);
            g.setColor(i==state.outfit?GOLD:FRAME);g.setStroke(new BasicStroke(i==state.outfit?5:2));
            g.drawRoundRect(x,y,105,90,12,12);
        }
        drawButton(g,"CLOSE WARDROBE",320,545,370,68);
    }
    private void drawSettings(Graphics2D g){
        text(g,"DESKTOP SETTINGS",277,110,28,PAPER);
        text(g,"Home environment: "+state.environment.displayName,276,170,18,GOLD);
        drawButton(g,"CYCLE HOME ENVIRONMENT",282,205,455,65);
        drawButton(g,state.showHelp?"HIDE KEYBOARD HINTS":"SHOW KEYBOARD HINTS",282,290,455,65);
        drawButton(g,"RECENTER VIEW",282,375,455,65);
        drawButton(g,"CLOSE SETTINGS",282,530,455,65);
    }
    private void drawHelp(Graphics2D g){
        text(g,"CONTROLS / HELP",275,108,27,PAPER);
        String[] lines={"WASD / arrows - Move within the Home room",
            "Q / E - Turn; Shift+drag to look around",
            "Mouse click - Select wall controls or desk keys",
            "Native Games opens IN Home, not a new room",
            "Press / to search the game catalog",
            "Esc - Close panel / return from gameplay",
            "F1 - Quick Menu; R - Restart a classic",
            "Pong: W/S; Snake: arrows; Breakout: A/D",
            "Settings/outfit are saved locally and offline"};
        for(int i=0;i<lines.length;i++)text(g,lines[i],282,160+i*40,16,i%2==0?PAPER:new Color(148,194,213));
        drawButton(g,"CLOSE HELP",310,568,399,60);
    }

    private void clickWall(double x,double y){
        if(quick){
            if(y>=240&&y<325){quick=false;return;}
            if(y>=335&&y<420){quick=false;homeAction("settings");return;}
            if(y>=430&&y<520){quick=false;state.recenter();return;}
            if(y>=525&&y<618){quick=false;exit();return;}
            return;
        }
        if(x>=29&&x<232){
            if(y>=159&&y<218)homeAction("games");
            else if(y>=237&&y<296)homeAction("environment");
            else if(y>=315&&y<374)homeAction("wardrobe");
            else if(y>=393&&y<452)homeAction("settings");
            else if(y>=471&&y<530)homeAction("help");
            else if(y>=549&&y<608)homeAction("exit");
            return;
        }
        if(x>=784&&x<996){
            if(y>=146&&y<202)homeAction("quick");
            else if(y>=225&&y<281)homeAction("recenter");
            else if(y>=304&&y<360)homeAction("search");
            return;
        }
        if(x<258||x>758) return;
        if(state.panel==DesktopHubState.Panel.GAMES){
            if(y>=135&&y<184){searchFocused=true;return;}
            if(y>=197&&y<518){
                int slot=((int)y-197)/166*2+((int)x>=513?1:0);
                if(slot>=0&&slot<4){
                    List<GameModuleRegistry.Entry> entries=state.availableGames();
                    int index=state.page*4+slot;
                    if(index<entries.size() && x>=275+(slot%2)*238
                        && x<501+(slot%2)*238 && y<352+(slot/2)*166)openGame(entries.get(index).id);
                }
            }else if(y>=532&&y<584){
                if(state.pageCount()>1 && x>=275&&x<385)state.nextPage(-1);
                else if(state.pageCount()>1 && x>=398&&x<508)state.nextPage(1);
                else if(x>=520&&x<745){state.close();searchFocused=false;}
            }
        }else if(state.panel==DesktopHubState.Panel.WARDROBE){
            if(y>=200&&y<423){
                int col=((int)x-297)/140,row=((int)y-200)/133,index=row*3+col;
                if(col>=0&&col<3&&row>=0&&row<2&&index<COLORS.length)state.outfit=index;
            }
            if(y>=545&&y<613)state.close();
        }else if(state.panel==DesktopHubState.Panel.SETTINGS){
            if(y>=205&&y<270)homeAction("environment");
            else if(y>=290&&y<355)state.showHelp=!state.showHelp;
            else if(y>=375&&y<440)state.recenter();
            else if(y>=530&&y<595)state.close();
        }else if(state.panel==DesktopHubState.Panel.HELP){
            if(y>=568&&y<628)state.close();
        }
    }

    private final class Surface extends JPanel {
        Surface(){setBackground(INK);
            KeyAdapter keyboard=new KeyAdapter(){
                @Override public void keyPressed(KeyEvent e){
                    int k=e.getKeyCode();if(k>=0&&k<keys.length)keys[k]=true;
                    if(k==KeyEvent.VK_ESCAPE){back();return;}
                    if(k==KeyEvent.VK_F1){if(game==null)quick=!quick;return;}
                    if(quick){
                        if(k==KeyEvent.VK_S){quick=false;homeAction("settings");}
                        else if(k==KeyEvent.VK_C){quick=false;state.recenter();}
                        else if(k==KeyEvent.VK_X){quick=false;exit();}
                        return;
                    }
                    if(game!=null){game.key(k,true);return;}
                    if(k==KeyEvent.VK_SLASH && state.panel==DesktopHubState.Panel.GAMES){searchFocused=true;return;}
                    if(k==KeyEvent.VK_ENTER&&searchFocused){searchFocused=false;return;}
                    if(k==KeyEvent.VK_BACK_SPACE&&searchFocused)state.search('\b');
                }
                @Override public void keyReleased(KeyEvent e){
                    int k=e.getKeyCode();if(k>=0&&k<keys.length)keys[k]=false;
                    if(game!=null)game.key(k,false);
                }
                @Override public void keyTyped(KeyEvent e){
                    if(game==null&&searchFocused&&e.getKeyChar()!='/'
                        &&e.getKeyChar()!='\b'&&e.getKeyChar()!='\n')state.search(e.getKeyChar());
                }
            };
            addKeyListener(keyboard);
            MouseAdapter mouse=new MouseAdapter(){
                @Override public void mousePressed(MouseEvent e){lastMouseX=e.getX();lastMouseY=e.getY();mouseDragging=false;requestFocusInWindow();}
                @Override public void mouseDragged(MouseEvent e){
                    int dx=e.getX()-lastMouseX;
                    if(game==null&&!quick&&e.isShiftDown())state.turn(dx*.004);
                    lastMouseX=e.getX();lastMouseY=e.getY();mouseDragging=true;
                }
                @Override public void mouseReleased(MouseEvent e){
                    if(mouseDragging||game!=null)return;
                    if(quick){
                        int cx=getWidth()/2,cy=getHeight()/2;
                        int x=e.getX(),y=e.getY();
                        if(x>=cx-255&&x<=cx+255){
                            if(y>=cy-85&&y<cy-25)quick=false;
                            else if(y>=cy-24&&y<cy+35){quick=false;homeAction("settings");}
                            else if(y>=cy+35&&y<cy+91){quick=false;state.recenter();}
                            else if(y>=cy+91&&y<cy+158){quick=false;exit();}
                        }
                        return;
                    }
                    DesktopDeskKeyboard.Key deskKey=DesktopDeskKeyboard.hit(state,getWidth(),getHeight(),
                        e.getX(),e.getY());
                    if(deskKey!=null){
                        searchFocused=DesktopDeskKeyboard.press(state,searchFocused,deskKey);
                        return;
                    }
                    if(!wallVisible)return;
                    try{
                        Point2D source=wallTransform.createInverse().transform(e.getPoint(),null);
                        if(source.getX()>=0&&source.getX()<WALL_WIDTH
                            &&source.getY()>=0&&source.getY()<WALL_HEIGHT)
                            clickWall(source.getX(),source.getY());
                    }catch(NoninvertibleTransformException ignored){}
                }
                @Override public void mouseWheelMoved(MouseWheelEvent e){
                    if(game==null)state.move(0,-e.getPreciseWheelRotation()*2,.05);
                }
            };
            addMouseListener(mouse);addMouseMotionListener(mouse);addMouseWheelListener(mouse);
        }

        private Point2D point(double x,double y,double z){
            double dx=x-state.x,dz=z-state.z;
            double sideways=Math.cos(state.yaw)*dx+Math.sin(state.yaw)*dz;
            double depth=Math.sin(state.yaw)*dx-Math.cos(state.yaw)*dz;
            if(depth<.15)return null;
            double focal=Math.min(getWidth(),getHeight())*1.08;
            return new Point2D.Double(getWidth()*.5+sideways*focal/depth,
                getHeight()*.52-(y-1.0)*focal/depth);
        }
        private void poly(Graphics2D g,Color color,double[][] corners){
            int[] xs=new int[corners.length],ys=new int[corners.length];
            for(int i=0;i<corners.length;i++){
                Point2D p=point(corners[i][0],corners[i][1],corners[i][2]);
                if(p==null)return;
                xs[i]=(int)p.getX();ys[i]=(int)p.getY();
            }
            g.setColor(color);g.fillPolygon(xs,ys,xs.length);
        }
        private void room(Graphics2D g){
            Color sky,ground,wallColor;
            switch(state.environment){
                case FOREST_GLADE:
                    sky=new Color(29,71,72);ground=new Color(40,73,54);wallColor=new Color(43,67,59);break;
                case DUNE_BEACH:
                    sky=new Color(24,67,100);ground=new Color(113,91,61);wallColor=new Color(55,75,82);break;
                default:
                    sky=new Color(21,39,60);ground=new Color(49,46,42);wallColor=new Color(50,58,60);
            }
            g.setColor(sky);g.fillRect(0,0,getWidth(),getHeight());
            g.setColor(ground);g.fillRect(0,getHeight()/2,getWidth(),getHeight()/2);
            poly(g,wallColor,new double[][]{{-5,2.8,-3.14},{5,2.8,-3.14},{5,-1,-3.14},{-5,-1,-3.14}});
            poly(g,ground,new double[][]{{-5,-1,-3.14},{5,-1,-3.14},{5,-1,4},{-5,-1,4}});
            poly(g,wallColor.darker(),new double[][]{{-5,2.8,-3.14},{-5,-1,-3.14},{-5,-1,4},{-5,2.8,4}});
            poly(g,wallColor.darker(),new double[][]{{5,2.8,-3.14},{5,-1,-3.14},{5,-1,4},{5,2.8,4}});
            for(int i=-4;i<=4;i++)poly(g,new Color(63,70,69,100),new double[][]{
                {i-.018,-.99,-3.13},{i+.018,-.99,-3.13},{i+.018,-.99,4},{i-.018,-.99,4}});
            paintWall();
            // The wall texture is on a fixed world plane; locomotion and
            // head-turning do NOT replace the room or discard the keyboard.
            Point2D p0=point(-2.9,2.20,-3.12),p1=point(2.9,2.20,-3.12),p2=point(-2.9,-.88,-3.12);
            wallVisible=p0!=null&&p1!=null&&p2!=null;
            if(wallVisible){
                wallTransform=new AffineTransform(
                    (p1.getX()-p0.getX())/WALL_WIDTH,
                    (p1.getY()-p0.getY())/WALL_WIDTH,
                    (p2.getX()-p0.getX())/WALL_HEIGHT,
                    (p2.getY()-p0.getY())/WALL_HEIGHT,
                    p0.getX(),p0.getY());
                g.drawImage(wall,wallTransform,null);
            }
            // The desk is in front of the fixed Home wall. Draw it AFTER the
            // wall texture so game browsing never hides physical keyboard keys.
            poly(g,new Color(99,78,61),new double[][]{
                {-1.5,-.30,-2.6},{1.5,-.30,-2.6},{1.5,-.30,-1.6},{-1.5,-.30,-1.6}});
            poly(g,new Color(42,60,72),new double[][]{
                {-1.33,-.28,-2.48},{1.33,-.28,-2.48},{1.33,-.28,-1.78},{-1.33,-.28,-1.78}});
            DesktopDeskKeyboard.draw(g,state,getWidth(),getHeight(),searchFocused);
            if(state.showHelp)text(g,"WASD move | Q/E turn | Shift+drag look | Click wall/desk | F1 Quick Menu",
                22,getHeight()-23,Math.max(12,Math.min(17,getWidth()/75)),PAPER);
        }
        private void drawQuick(Graphics2D g){
            g.setColor(new Color(3,11,20,225));g.fillRoundRect(getWidth()/2-263,getHeight()/2-180,526,366,16,16);
            g.setColor(FRAME);g.setStroke(new BasicStroke(3));g.drawRoundRect(getWidth()/2-263,getHeight()/2-180,526,366,16,16);
            text(g,"QUICK MENU",getWidth()/2-135,getHeight()/2-116,30,GOLD);
            text(g,"ESC / F1  RESUME",getWidth()/2-142,getHeight()/2-52,21,PAPER);
            text(g,"S  SETTINGS",getWidth()/2-142,getHeight()/2+6,21,PAPER);
            text(g,"C  RECENTER",getWidth()/2-142,getHeight()/2+64,21,PAPER);
            text(g,"X  EXIT",getWidth()/2-142,getHeight()/2+122,21,PAPER);
        }
        @Override protected void paintComponent(Graphics gr){super.paintComponent(gr);
            Graphics2D g=(Graphics2D)gr.create();
            try{
                g.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
                if(game!=null){
                    double scale=Math.min(getWidth()/(double)DesktopArcade.WIDTH,getHeight()/(double)DesktopArcade.HEIGHT);
                    int x=(int)((getWidth()-DesktopArcade.WIDTH*scale)/2),y=(int)((getHeight()-DesktopArcade.HEIGHT*scale)/2);
                    g.setColor(INK);g.fillRect(0,0,getWidth(),getHeight());
                    Graphics2D gg=(Graphics2D)g.create();gg.translate(x,y);gg.scale(scale,scale);
                    game.draw(gg);gg.dispose();
                    text(g,"ESC: return to your Home room",20,26,17,PAPER);
                }else{
                    room(g);
                    if(quick)drawQuick(g);
                }
                if(!notice.isEmpty()&&System.nanoTime()<noticeUntil)
                    text(g,notice,25,48,16,GOLD);
            }finally{g.dispose();}
        }
    }

    public static void main(String[] args) {
        if(args.length>1){System.err.println("Usage: ReverieDesktopPlayer [workspace]");System.exit(2);}
        final Path workspace=args.length==0?Path.of("build/desktop-workspace"):Path.of(args[0]);
        SwingUtilities.invokeLater(()->{
            try{new ReverieDesktopPlayer(workspace).show();}
            catch(IOException ex){System.err.println("Unable to start desktop Home: "+ex.getMessage());System.exit(1);}
        });
    }
}
