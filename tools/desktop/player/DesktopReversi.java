package io.github.mrcalzon02.reverievr;

/** Offline human-versus-computer Reversi rules, independent of Swing and Android. */
final class DesktopReversi {
    static final int EMPTY = 0, BLACK = 1, WHITE = -1, SIZE = 8;
    static final int ORIGIN_X = 238, ORIGIN_Y = 64, CELL = 54;
    private static final int[][] DIRECTIONS = {
        {-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}
    };
    final int[][] board = new int[SIZE][SIZE];
    int current = BLACK, passes, cursorX = 2, cursorY = 3;
    boolean ended;
    String message = "Black to move. You play black.";

    DesktopReversi() { reset(); }

    void reset() {
        for (int y=0; y<SIZE; y++) java.util.Arrays.fill(board[y], EMPTY);
        board[3][3]=WHITE; board[4][4]=WHITE;
        board[3][4]=BLACK; board[4][3]=BLACK;
        current=BLACK; passes=0; cursorX=2; cursorY=3; ended=false;
        message="Black to move. You play black.";
    }

    private static boolean inside(int x,int y) { return x>=0&&x<SIZE&&y>=0&&y<SIZE; }

    int flips(int x,int y,int side) {
        if (!inside(x,y) || board[y][x]!=EMPTY || (side!=BLACK && side!=WHITE)) return 0;
        int total=0;
        for (int[] d:DIRECTIONS) {
            int cx=x+d[0], cy=y+d[1], n=0;
            while (inside(cx,cy) && board[cy][cx]==-side) {
                n++; cx+=d[0]; cy+=d[1];
            }
            if (n>0 && inside(cx,cy) && board[cy][cx]==side) total+=n;
        }
        return total;
    }

    boolean canMove(int side) {
        for (int y=0;y<SIZE;y++) for (int x=0;x<SIZE;x++)
            if (flips(x,y,side)>0) return true;
        return false;
    }

    int count(int side) {
        int result=0;
        for (int[] row:board) for (int cell:row) if (cell==side) result++;
        return result;
    }

    private void apply(int x,int y,int side) {
        board[y][x]=side;
        for (int[] d:DIRECTIONS) {
            int cx=x+d[0],cy=y+d[1],n=0;
            while (inside(cx,cy) && board[cy][cx]==-side) {
                n++; cx+=d[0]; cy+=d[1];
            }
            if (n==0 || !inside(cx,cy) || board[cy][cx]!=side) continue;
            cx=x+d[0]; cy=y+d[1];
            for (int i=0;i<n;i++,cx+=d[0],cy+=d[1]) board[cy][cx]=side;
        }
    }

    /** A human turn, followed by deterministic computer turns and mandatory passes. */
    boolean play(int x,int y) {
        if (ended || current!=BLACK || flips(x,y,BLACK)==0) return false;
        apply(x,y,BLACK);
        current=WHITE;
        advance();
        return true;
    }

    /** Resolve mandatory passes and computer moves until the human can act or game ends. */
    void advance() {
        for (int step=0;step<=SIZE*SIZE+2;step++) {
            boolean currentCan=canMove(current), otherCan=canMove(-current);
            if (!currentCan && !otherCan) {
                ended=true;
                int black=count(BLACK),white=count(WHITE);
                message=black==white?"Draw game.":black>white?"Black wins!":"White wins!";
                return;
            }
            if (!currentCan) {
                passes++;
                message=(current==BLACK?"Black":"White")+" has no move and passes.";
                current=-current;
                if (current==BLACK) return;
                continue;
            }
            if (current==BLACK) {
                message=passes>0?"Black to move after pass.":"Black to move.";
                return;
            }
            int bestX=-1,bestY=-1,bestValue=Integer.MIN_VALUE;
            for (int y=0;y<SIZE;y++) for (int x=0;x<SIZE;x++) {
                int n=flips(x,y,WHITE);
                if (n==0) continue;
                int value=n+((x==0||x==7)&&(y==0||y==7)?32:0);
                if (value>bestValue) {bestValue=value;bestX=x;bestY=y;}
            }
            apply(bestX,bestY,WHITE);
            current=BLACK;
        }
        throw new IllegalStateException("Reversi turn progression did not settle");
    }

    void moveCursor(int dx,int dy) {
        cursorX=Math.floorMod(cursorX+dx,SIZE);
        cursorY=Math.floorMod(cursorY+dy,SIZE);
    }

    boolean click(int px,int py) {
        if (px<ORIGIN_X || py<ORIGIN_Y || px>=ORIGIN_X+SIZE*CELL || py>=ORIGIN_Y+SIZE*CELL)
            return false;
        cursorX=(px-ORIGIN_X)/CELL;
        cursorY=(py-ORIGIN_Y)/CELL;
        return play(cursorX,cursorY);
    }
}
