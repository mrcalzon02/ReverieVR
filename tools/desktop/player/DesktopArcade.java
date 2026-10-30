package io.github.mrcalzon02.reverievr;

import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics2D;
import java.awt.event.KeyEvent;
import java.util.ArrayDeque;
import java.util.Deque;
import java.util.Random;

/** Playable desktop-classic prototypes; only ported IDs are admitted by DesktopHubState. */
final class DesktopArcade {
    static final int WIDTH=920, HEIGHT=570;
    private static final Color PAPER=new Color(217,238,241);
    private static final Color BLUE=new Color(71,161,208);
    private static final Color GOLD=new Color(245,200,104);
    final String id;
    final Random random;
    double ballX, ballY, vx, vy, paddle, enemy, breakoutPaddle, breakoutBallX,
        breakoutBallY, breakoutVx, breakoutVy;
    int playerScore, aiScore, lives=3, brickCount, snakeScore;
    boolean[][] bricks;
    DesktopReversi reversi;
    Deque<Integer> snake;
    int snakeDirection=1, requestedDirection=1, food, snakeTicks;
    boolean ended;
    private double accumulator;
    private final boolean[] down = new boolean[4];

    DesktopArcade(String id, long seed) {
        if (!DesktopHubState.ported(id)) throw new IllegalArgumentException("Not a ported desktop game: "+id);
        this.id=id;
        random=new Random(seed);
        reset();
    }

    void reset() {
        ended=false; accumulator=0;
        playerScore=0; aiScore=0; snakeScore=0; lives=3;
        java.util.Arrays.fill(down,false);
        ballX=WIDTH/2.0;ballY=HEIGHT/2.0;vx=210;vy=114;paddle=HEIGHT/2.0;enemy=paddle;
        breakoutPaddle=WIDTH/2.0;brickCount=0;
        breakoutBallX=WIDTH/2.0;breakoutBallY=HEIGHT-105;breakoutVx=167;breakoutVy=-226;
        bricks=new boolean[5][11];
        for(int row=0;row<bricks.length;row++)for(int col=0;col<bricks[row].length;col++) {
            bricks[row][col]=true;brickCount++;
        }
        snake=new ArrayDeque<>();
        snake.add(8*30+6);snake.add(8*30+5);snake.add(8*30+4);
        snakeDirection=1;requestedDirection=1;snakeTicks=0;
        spawnFood();
        reversi=id.equals(DesktopHubState.REVERSI)?new DesktopReversi():null;
    }

    void key(int code, boolean pressed) {
        int dir = code==KeyEvent.VK_UP || code==KeyEvent.VK_W ? 0
            : code==KeyEvent.VK_DOWN || code==KeyEvent.VK_S ? 1
            : code==KeyEvent.VK_LEFT || code==KeyEvent.VK_A ? 2
            : code==KeyEvent.VK_RIGHT || code==KeyEvent.VK_D ? 3 : -1;
        if(dir>=0) down[dir]=pressed;
        if(id.equals(DesktopHubState.SNAKE) && pressed && dir>=0) {
            int target=dir==0?0:dir==3?1:dir==1?2:3;
            if((target+2)%4 != snakeDirection) requestedDirection=target;
        }
        if(reversi!=null && pressed) {
            if(dir==0)reversi.moveCursor(0,-1);
            else if(dir==1)reversi.moveCursor(0,1);
            else if(dir==2)reversi.moveCursor(-1,0);
            else if(dir==3)reversi.moveCursor(1,0);
            else if(code==KeyEvent.VK_ENTER || code==KeyEvent.VK_SPACE)
                reversi.play(reversi.cursorX,reversi.cursorY);
        }
        if(pressed&&code==KeyEvent.VK_R) reset();
    }

    void update(double seconds) {
        if(ended) return;
        seconds=Math.max(0,Math.min(seconds,.1));
        if(id.equals(DesktopHubState.PONG)) pong(seconds);
        else if(id.equals(DesktopHubState.SNAKE)) {
            accumulator+=seconds;
            while(accumulator>=.11 && !ended) {accumulator-=.11; stepSnake();}
        } else if(id.equals(DesktopHubState.BREAKOUT)) breakout(seconds);
    }

    /** Pixel coordinates are in the same 920x570 logical canvas used by draw(). */
    boolean click(int x,int y) { return reversi!=null && reversi.click(x,y); }

    private void pong(double dt) {
        if(down[0])paddle-=330*dt;
        if(down[1])paddle+=330*dt;
        paddle=bound(paddle,55,HEIGHT-55);
        enemy=bound(enemy+bound(ballY-enemy,-240*dt,240*dt),55,HEIGHT-55);
        ballX+=vx*dt;ballY+=vy*dt;
        if(ballY<12){ballY=12;vy=Math.abs(vy);}
        if(ballY>HEIGHT-12){ballY=HEIGHT-12;vy=-Math.abs(vy);}
        if(ballX<40 && vx<0 && Math.abs(ballY-paddle)<60) {
            ballX=40;vx=Math.abs(vx)*1.04;vy=(ballY-paddle)*4;
        }
        if(ballX>WIDTH-40 && vx>0 && Math.abs(ballY-enemy)<60) {
            ballX=WIDTH-40;vx=-Math.abs(vx)*1.04;vy=(ballY-enemy)*4;
        }
        if(ballX < -10 || ballX > WIDTH+10) {
            if(ballX<0)aiScore++;else playerScore++;
            ballX=WIDTH/2.0;ballY=HEIGHT/2.0;
            vx=ballX<0?180:(playerScore+aiScore)%2==0?210:-210;
            vy=110+random.nextInt(80);
            if(playerScore>=7 || aiScore>=7) ended=true;
        }
    }

    private void spawnFood() {
        for(int tries=0;tries<900;tries++) {
            int next=random.nextInt(30*17);
            if(!snake.contains(next)) {food=next;return;}
        }
        ended=true;
    }

    private void stepSnake() {
        snakeDirection=requestedDirection;
        int head=snake.peekFirst(),x=head%30,y=head/30;
        int dx=snakeDirection==1?1:snakeDirection==3?-1:0;
        int dy=snakeDirection==2?1:snakeDirection==0?-1:0;
        x+=dx;y+=dy;
        if(x<0||x>=30||y<0||y>=17){ended=true;return;}
        int position=y*30+x;
        boolean grow=position==food;
        int tail=snake.peekLast();
        if(snake.contains(position) && (grow||position!=tail)){ended=true;return;}
        snake.addFirst(position);
        if(grow){snakeScore++;spawnFood();}else snake.removeLast();
        snakeTicks++;
    }

    private void breakout(double dt) {
        if(down[2])breakoutPaddle-=430*dt;
        if(down[3])breakoutPaddle+=430*dt;
        breakoutPaddle=bound(breakoutPaddle,75,WIDTH-75);
        double prevY=breakoutBallY;
        breakoutBallX+=breakoutVx*dt;
        breakoutBallY+=breakoutVy*dt;
        if(breakoutBallX<10){breakoutBallX=10;breakoutVx=Math.abs(breakoutVx);}
        if(breakoutBallX>WIDTH-10){breakoutBallX=WIDTH-10;breakoutVx=-Math.abs(breakoutVx);}
        if(breakoutBallY<10){breakoutBallY=10;breakoutVy=Math.abs(breakoutVy);}
        if(prevY<HEIGHT-59&&breakoutBallY>=HEIGHT-59&&breakoutVy>0
            &&Math.abs(breakoutBallX-breakoutPaddle)<69) {
            breakoutBallY=HEIGHT-59;breakoutVy=-Math.abs(breakoutVy);
            breakoutVx=(breakoutBallX-breakoutPaddle)*4.0;
        }
        if(breakoutBallY>HEIGHT+10) {
            lives--;
            if(lives<=0){ended=true;return;}
            breakoutBallX=WIDTH/2.0;breakoutBallY=HEIGHT-105;
            breakoutVx=167;breakoutVy=-226;
        }
        if(breakoutBallY>=65 && breakoutBallY<245) {
            int col=(int)((breakoutBallX-35)/77),row=(int)((breakoutBallY-65)/34);
            if(row>=0&&row<5&&col>=0&&col<11&&bricks[row][col]){
                bricks[row][col]=false;brickCount--;breakoutVy=-breakoutVy;
                playerScore+=10;
                if(brickCount==0)ended=true;
            }
        }
    }

    private static double bound(double v,double min,double max){return Math.max(min,Math.min(max,v));}
    private static void text(Graphics2D g,String t,int x,int y,int size,Color color) {
        g.setColor(color);g.setFont(new Font(Font.MONOSPACED,Font.BOLD,size));g.drawString(t,x,y);
    }

    void draw(Graphics2D g) {
        g.setColor(new Color(7,16,29));g.fillRect(0,0,WIDTH,HEIGHT);
        g.setStroke(new BasicStroke(3f));
        if(id.equals(DesktopHubState.PONG)){
            g.setColor(new Color(31,65,86));
            for(int y=18;y<HEIGHT-10;y+=38)g.fillRect(WIDTH/2-2,y,4,22);
            g.setColor(PAPER);g.fillRoundRect(28,(int)paddle-54,14,108,6,6);
            g.fillRoundRect(WIDTH-42,(int)enemy-54,14,108,6,6);
            g.setColor(GOLD);g.fillOval((int)ballX-9,(int)ballY-9,18,18);
            text(g,playerScore+" : "+aiScore,WIDTH/2-52,48,28,PAPER);
            text(g,"W/S or UP/DOWN to move your paddle",22,HEIGHT-22,16,BLUE);
        }else if(id.equals(DesktopHubState.SNAKE)){
            final int cell=24,ox=100,oy=68;
            g.setColor(new Color(17,36,53));g.fillRect(ox-5,oy-5,730,418);
            for(int pos:snake){int x=pos%30,y=pos/30;
                g.setColor(pos==snake.peekFirst()?GOLD:new Color(85,204,160));
                g.fillRoundRect(ox+x*cell,oy+y*cell,cell-2,cell-2,5,5);
            }
            g.setColor(new Color(228,111,129));
            g.fillOval(ox+(food%30)*cell+3,oy+(food/30)*cell+3,cell-8,cell-8);
            text(g,"SCORE "+snakeScore,100,44,24,PAPER);
            text(g,"ARROWS / WASD to turn. Eat the coral, avoid yourself.",100,HEIGHT-28,16,BLUE);
        }else if(id.equals(DesktopHubState.BREAKOUT)){
            g.setColor(new Color(40,78,96));g.drawRect(8,8,WIDTH-16,HEIGHT-16);
            for(int row=0;row<5;row++)for(int col=0;col<11;col++)if(bricks[row][col]){
                g.setColor((row+col)%3==0?GOLD:(row+col)%3==1?BLUE:new Color(113,215,181));
                g.fillRoundRect(38+col*77,65+row*34,70,27,5,5);
            }
            g.setColor(PAPER);g.fillRoundRect((int)breakoutPaddle-66,HEIGHT-54,132,14,8,8);
            g.setColor(GOLD);g.fillOval((int)breakoutBallX-9,(int)breakoutBallY-9,18,18);
            text(g,"POINTS "+playerScore+"    LIVES "+lives,30,36,23,PAPER);
            text(g,"LEFT/RIGHT or A/D to move. Clear the board.",30,HEIGHT-17,16,BLUE);
        }
        if(reversi!=null) {
            final int ox=DesktopReversi.ORIGIN_X, oy=DesktopReversi.ORIGIN_Y,
                cell=DesktopReversi.CELL;
            g.setColor(new Color(19,72,68));
            g.fillRoundRect(ox-8,oy-8,cell*8+16,cell*8+16,10,10);
            for(int y=0;y<8;y++)for(int x=0;x<8;x++){
                int px=ox+x*cell,py=oy+y*cell;
                g.setColor((x+y)%2==0?new Color(30,108,91):new Color(25,92,81));
                g.fillRect(px,py,cell-1,cell-1);
                int disc=reversi.board[y][x];
                if(disc!=DesktopReversi.EMPTY){
                    g.setColor(disc==DesktopReversi.BLACK?new Color(12,20,29):PAPER);
                    g.fillOval(px+6,py+6,cell-12,cell-12);
                    g.setColor(disc==DesktopReversi.BLACK?new Color(92,120,136):new Color(140,174,180));
                    g.drawOval(px+6,py+6,cell-12,cell-12);
                }else if(!reversi.ended && reversi.flips(x,y,DesktopReversi.BLACK)>0){
                    g.setColor(GOLD);g.fillOval(px+cell/2-4,py+cell/2-4,8,8);
                }
                if(x==reversi.cursorX && y==reversi.cursorY){
                    g.setColor(GOLD);g.setStroke(new BasicStroke(3));
                    g.drawRect(px+2,py+2,cell-5,cell-5);
                }
            }
            text(g,"REVERSI",25,53,30,GOLD);
            text(g,"YOU / BLACK",687,118,17,PAPER);
            text(g,Integer.toString(reversi.count(DesktopReversi.BLACK)),725,154,32,GOLD);
            text(g,"CPU / WHITE",687,219,17,PAPER);
            text(g,Integer.toString(reversi.count(DesktopReversi.WHITE)),725,255,32,GOLD);
            text(g,"Arrows / WASD",680,339,16,BLUE);
            text(g,"Enter / Space",680,368,16,BLUE);
            text(g,"or click a cell",680,397,16,BLUE);
            text(g,"R: new game",680,439,16,PAPER);
            text(g,reversi.message,24,HEIGHT-24,16,reversi.ended?GOLD:PAPER);
        }
        if(ended){
            g.setColor(new Color(5,14,23,228));g.fillRoundRect(190,206,WIDTH-380,145,15,15);
            text(g,id.equals(DesktopHubState.SNAKE)?"GAME OVER":id.equals(DesktopHubState.BREAKOUT)
                &&brickCount==0?"BOARD CLEARED":"ROUND OVER",270,267,32,GOLD);
            text(g,"Press R to play again or ESC for Home",230,310,18,PAPER);
        }
    }
}
