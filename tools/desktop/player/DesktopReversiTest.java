package io.github.mrcalzon02.reverievr;

/** Deterministic rules regression, run by the existing desktop-player test script. */
public final class DesktopReversiTest {
    private static void check(boolean value,String message) {
        if (!value) throw new AssertionError(message);
    }
    public static void main(String[] args) {
        DesktopReversi game=new DesktopReversi();
        check(game.count(DesktopReversi.BLACK)==2 && game.count(DesktopReversi.WHITE)==2,"initial discs");
        int legal=0;
        for (int y=0;y<8;y++) for (int x=0;x<8;x++)
            if (game.flips(x,y,DesktopReversi.BLACK)>0) legal++;
        check(legal==4,"four initial legal moves");
        check(!game.play(0,0) && !game.play(-1,0) && !game.play(8,0),"illegal/out-of-range rejected");
        check(game.count(DesktopReversi.BLACK)==2 && game.count(DesktopReversi.WHITE)==2,"illegal moves do not mutate");
        check(game.play(2,3),"human legal directional flip");
        check(game.current==DesktopReversi.BLACK && !game.ended,"computer responds and returns turn");
        check(game.count(DesktopReversi.BLACK)+game.count(DesktopReversi.WHITE)==6,"one human and one AI placement");
        check(!game.click(0,0),"outside board does not place");
        game.reset();
        check(game.click(DesktopReversi.ORIGIN_X+2*DesktopReversi.CELL+3,
            DesktopReversi.ORIGIN_Y+3*DesktopReversi.CELL+3),"mouse cell maps to legal move");
        game.reset();
        for (int[] row:game.board) java.util.Arrays.fill(row,DesktopReversi.EMPTY);
        for (int[] d:new int[][]{{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}}) {
            game.board[3+d[1]][3+d[0]]=DesktopReversi.WHITE;
            game.board[3+2*d[1]][3+2*d[0]]=DesktopReversi.BLACK;
        }
        check(game.flips(3,3,DesktopReversi.BLACK)==8,"all eight directional flips");
        game.reset();
        for (int[] row:game.board) java.util.Arrays.fill(row,DesktopReversi.BLACK);
        game.board[0][0]=DesktopReversi.EMPTY;
        game.board[0][1]=DesktopReversi.WHITE;
        game.current=DesktopReversi.WHITE;
        game.advance();
        check(!game.ended && game.current==DesktopReversi.BLACK && game.passes==1,
            "computer must pass when only human can move");
        check(game.play(0,0),"human takes passed turn");
        check(game.ended && game.count(DesktopReversi.BLACK)==64 && game.count(DesktopReversi.WHITE)==0,
            "terminal score and winner");
        check(!game.play(0,0),"cannot move after terminal state");
        game.reset();
        for (int[] row:game.board) java.util.Arrays.fill(row,DesktopReversi.WHITE);
        game.board[0][0]=DesktopReversi.EMPTY;
        game.board[0][1]=DesktopReversi.BLACK;
        game.current=DesktopReversi.BLACK;
        game.advance();
        check(game.ended && game.passes==1 && game.count(DesktopReversi.WHITE)==64,
            "human forced pass, AI final move, white victory");
        game.reset();
        game.moveCursor(-3,-4);
        check(game.cursorX==7 && game.cursorY==7,"keyboard cursor wraps safely");
        System.out.println("DesktopReversi: PASS legal/illegal, 8-direction flips, mouse, AI, both forced passes, scoring, restart, cursor");
    }
}
