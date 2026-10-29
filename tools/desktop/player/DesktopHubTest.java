package io.github.mrcalzon02.reverievr;
import java.awt.Graphics2D;
import java.awt.image.BufferedImage;
import java.nio.file.Files;
import java.nio.file.Path;
public final class DesktopHubTest {
 static void require(boolean ok,String message){if(!ok)throw new AssertionError(message);}
 public static void main(String[] args) throws Exception {
  Path root=Files.createTempDirectory("reverie-desktop-home-");
  try {
   DesktopHubState s=new DesktopHubState(root);
   require(s.availableGames().size()==3,"only actually ported games selectable");
   require(s.pageCount()==1,"three current ports are one page");
   s.open(DesktopHubState.Panel.GAMES);
   double priorZ=s.z;
   s.move(0,1,.1);
   require(s.panel==DesktopHubState.Panel.GAMES && s.z<priorZ,"in-room browser must preserve locomotion");
   s.search('p');require(s.availableGames().size()==1,"search actual registry");
   s.search('\b');require(s.availableGames().size()==3,"delete search");
   require(!s.launch("breakwater-battery"),"unported game must not expose fake launch");
   require(s.launch(DesktopHubState.PONG),"desktop pong launches");
   require(s.panel==DesktopHubState.Panel.NONE,"game hides catalog for game focus");
   require(!s.launch(DesktopHubState.SNAKE),"no nested games");
   s.back();require(s.runningGame.isEmpty() && s.panel==DesktopHubState.Panel.GAMES,"exit restores Home browser");
   require(s.z<priorZ,"same Home position after game");
   s.back();require(s.panel==DesktopHubState.Panel.NONE,"Back dismisses in-room browser");
   s.environment=HomeEnvironment.DUNE_BEACH;s.outfit=4;s.showHelp=false;s.save();
   DesktopHubState loaded=new DesktopHubState(root);loaded.load();
   require(loaded.environment==HomeEnvironment.DUNE_BEACH&&loaded.outfit==4&&!loaded.showHelp,"persistent settings");
   for(String id:DesktopHubState.PORTED_IDS){
    DesktopArcade game=new DesktopArcade(id,23);
    BufferedImage img=new BufferedImage(DesktopArcade.WIDTH,DesktopArcade.HEIGHT,BufferedImage.TYPE_INT_ARGB);
    Graphics2D g=img.createGraphics();game.draw(g);g.dispose();
    require(img.getRGB(0,0)!=0,"draws genuine frame for "+id);
    for(int i=0;i<120;i++)game.update(.016);
    game.key(java.awt.event.KeyEvent.VK_R,true);
    require(!game.ended,"restart game for "+id);
   }
   DesktopArcade snake=new DesktopArcade(DesktopHubState.SNAKE,99);
   for(int i=0;i<150&&!snake.ended;i++)snake.update(.12);
   require(snake.ended,"snake should collide with wall in bounded grid");
   DesktopArcade pong=new DesktopArcade(DesktopHubState.PONG,11);
   pong.ballX=1;pong.ballY=10;pong.vx=-200;
   pong.update(.1);
   require(pong.aiScore>0,"pong scoring must be real");
   require(!DesktopHubState.ported("classic-chess"),"missing desktop chess correctly unavailable");
   System.out.println("DesktopHub: PASS state, keyboard search, live Home movement, game lifecycle, save, 3 games rendering, snake boundaries, pong scoring");
  }finally{
   try(var paths=Files.walk(root)) {paths.sorted(java.util.Comparator.reverseOrder()).forEach(p->{try{Files.delete(p);}catch(Exception ex){throw new RuntimeException(ex);}});}
  }
 }
}
