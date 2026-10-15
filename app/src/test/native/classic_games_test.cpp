#include "classic_games.h"
#include "classic_breakout.h"
#include "classic_board_games.h"
#include "reverie_native_flat_mesh.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::classics;

static void TestFlatGeometry(){
    using namespace reverie::flatmesh;
    std::vector<float> a,b;
    Rect(a,0.f,0.f,2.f,2.f,-3.f,1.f,.5f,0.f);
    Rect(b,0.f,0.f,2.f,2.f,-3.f,1.f,.5f,0.f);
    assert(a==b&&a.size()==36u);
    assert(a[0]==-1.f&&a[1]==-1.f&&a[2]==-3.f);
    assert(a[6]==1.f&&a[7]==-1.f);
    Segment(a,0.f,0.f,1.f,0.f,.1f,-2.f,1.f,1.f,1.f);
    assert(a.size()==72u);
    Segment(a,0.f,0.f,0.f,0.f,.1f,-2.f,1.f,1.f,1.f);
    assert(a.size()==72u);
    Disc(a,0.f,0.f,.25f,-2.f,1.f,1.f,1.f);
    assert(a.size()==288u);
    for(float f:a)assert(std::isfinite(f));
}
static void TestBoardGames() {
    using Boards = reverie::boardgames::State;
    using BoardKind = reverie::boardgames::Kind;
    Boards checkers;
    checkers.Reset(BoardKind::Checkers);
    int light=0,dark=0;
    for(int i=0;i<64;i++){
        if(checkers.cells[i]>0)++light;
        if(checkers.cells[i]<0)++dark;
    }
    assert(light==12&&dark==12);
    assert(!checkers.CheckerMove(40,24)); // men cannot move two without capture
    assert(checkers.Select(40)); // lower white man
    assert(checkers.Select(33));
    assert(checkers.side==-1&&checkers.cells[33]==1);
    for(int i=0;i<4;i++)checkers.Step(.1f);
    assert(checkers.side==1||checkers.forced>=0); // CPU plays real moves

    // Forced capture and continued capture cannot be evaded by another man.
    Boards jumps;
    jumps.Reset(BoardKind::Checkers);
    jumps.cells.fill(0);jumps.side=1;
    jumps.cells[45]=1;jumps.cells[36]=-1;jumps.cells[18]=-1;
    jumps.cells[41]=1;
    assert(jumps.HasCapture(1));
    assert(!jumps.CheckerMove(41,34));
    assert(jumps.CheckerMove(45,27));
    assert(jumps.forced==27&&jumps.side==1&&jumps.cells[36]==0);
    assert(!jumps.CheckerMove(41,34));
    assert(jumps.CheckerMove(27,9));
    assert(jumps.forced==-1&&jumps.side==-1&&jumps.cells[18]==0);
    Boards crown;
    crown.Reset(BoardKind::Checkers);
    crown.cells.fill(0);crown.cells[9]=1;crown.side=1;
    assert(crown.CheckerMove(9,0));
    assert(crown.cells[0]==2);

    // Regulation 19x19 Go board with 361 playable intersections.
    const auto at=[](int x,int y){return y*19+x;};
    Boards go;
    go.Reset(BoardKind::Go);
    assert(go.Size()==19&&Boards::kCapacity==361);
    assert(go.Inside(18,18)&&!go.Inside(19,18));
    assert(go.GoMove(at(18,18)));
    assert(go.cells[at(18,18)]==1);
    assert(go.goHistory.size()==2);
    // Capture against the edge of the full-sized grid.
    Boards edgeGo;
    edgeGo.Reset(BoardKind::Go);
    edgeGo.cells[at(18,18)]=-1;
    edgeGo.cells[at(17,18)]=1;
    edgeGo.side=1;
    assert(edgeGo.GoMove(at(18,17)));
    assert(edgeGo.cells[at(18,18)]==0);
    assert(!edgeGo.GoMove(at(18,17)));
    Boards suicide;
    suicide.Reset(BoardKind::Go);
    suicide.cells[at(9,8)]=-1;
    suicide.cells[at(8,9)]=-1;
    suicide.cells[at(10,9)]=-1;
    suicide.cells[at(9,10)]=-1;
    assert(!suicide.GoMove(at(9,9)));
    // Ko: recapturing the prior whole-board shape is forbidden. The
    // setup board is added to history exactly as a played game would.
    Boards ko;
    ko.Reset(BoardKind::Go);
    ko.cells[at(2,2)]=1;
    ko.cells[at(2,1)]=-1;ko.cells[at(1,2)]=-1;ko.cells[at(3,2)]=-1;
    ko.cells[at(1,3)]=1;ko.cells[at(3,3)]=1;ko.cells[at(2,4)]=1;
    ko.goHistory.insert(ko.cells);
    ko.side=-1;
    assert(ko.GoMove(at(2,3)));
    assert(ko.cells[at(2,2)]==0);
    assert(!ko.GoMove(at(2,2)));
    // Positional superko rejects an earlier shape even if it was not
    // the immediately previous board, without hash-only false matches.
    Boards repeat;
    repeat.Reset(BoardKind::Go);
    Boards::Board forbidden=repeat.cells;
    forbidden[at(9,9)]=1;
    repeat.goHistory.insert(forbidden);
    assert(!repeat.GoMove(at(9,9)));
    assert(repeat.cells[at(9,9)]==0);
    // Both players may pass, and the 7.5 komi decides an empty board.
    Boards emptyGo;
    emptyGo.Reset(BoardKind::Go);
    assert(emptyGo.Pass()&&!emptyGo.finished);
    assert(emptyGo.Pass()&&emptyGo.finished&&emptyGo.winner==-1);
    Boards cpuGo;
    cpuGo.Reset(BoardKind::Go);
    assert(cpuGo.GoMove(at(9,9)));
    for(int i=0;i<3;i++)cpuGo.Step(.1f);
    assert(cpuGo.ply==2&&cpuGo.side==1);
    assert(cpuGo.goHistory.size()==3);
    // The CPU should still respond late on a regulation board, instead
    // of auto-passing at the obsolete 155-ply 9x9 cutoff.
    Boards laterGo;
    laterGo.Reset(BoardKind::Go);
    laterGo.ply=160;laterGo.side=-1;
    for(int i=0;i<3;i++)laterGo.Step(.1f);
    assert(laterGo.ply==161&&!laterGo.finished&&laterGo.side==1);
    // AI only passes a board with no useful legal placement.
    Boards surrounded;
    surrounded.Reset(BoardKind::Go);
    surrounded.cells.fill(-1);surrounded.side=-1;
    surrounded.cells[at(9,9)]=0;
    surrounded.goHistory.insert(surrounded.cells);
    surrounded.Step(.3f);
    assert(surrounded.passes==1&&surrounded.side==1);

    Boards chess;
    chess.Reset(BoardKind::Chess);
    assert(chess.Size()==8&&chess.cells[60]==6&&chess.cells[4]==-6);
    assert(!chess.ChessLegal(52,28)); // e2-e5 illegal
    assert(chess.ChessMove(52,36));  // e2-e4
    assert(chess.enPassant==44);
    assert(chess.ChessMove(8,16)); // a7-a6
    assert(chess.ChessMove(36,28)); // e4-e5
    assert(chess.ChessMove(11,27)); // d7-d5
    assert(chess.ChessLegal(28,19)); // e5xd6 en passant
    assert(chess.ChessMove(28,19));
    assert(chess.cells[27]==0&&chess.cells[19]==1);

    Boards castle;
    castle.Reset(BoardKind::Chess);
    castle.cells.fill(0);
    castle.cells[60]=6;castle.cells[63]=4;castle.cells[4]=-6;
    castle.castles=1;castle.side=1;
    assert(castle.ChessLegal(60,62));
    assert(castle.ChessMove(60,62));
    assert(castle.cells[62]==6&&castle.cells[61]==4);
    Boards illegalKing;
    illegalKing.Reset(BoardKind::Chess);
    illegalKing.cells.fill(0);
    illegalKing.cells[60]=6;illegalKing.cells[4]=-4;illegalKing.cells[0]=-6;
    assert(!illegalKing.ChessLegal(60,52)); // rook threatens e-file

    Boards mate;
    mate.Reset(BoardKind::Chess);
    assert(mate.ChessMove(53,45)); // f3
    assert(mate.ChessMove(12,28)); // ...e5
    assert(mate.ChessMove(54,38)); // g4
    assert(mate.ChessMove(3,39)); // ...Qh4 mate
    assert(mate.finished&&mate.winner==-1);
    assert(mate.Select(-1)); // restart from a completed game
    assert(!mate.finished&&mate.cells[60]==6);

    Boards cpuChess;
    cpuChess.Reset(BoardKind::Chess);
    assert(cpuChess.ChessMove(52,36));
    for(int i=0;i<3;i++)cpuChess.Step(.1f);
    assert(cpuChess.ply==2&&cpuChess.side==1);
    assert(!cpuChess.KingInCheck(cpuChess.cells,-1));

    std::cout<<"Checkers, Go 9x9 and Chess deterministic rules/CPU passed\\n";
}

static void TestBreakout() {
    using reverie::classics::Breakout;
    Breakout game;
    game.Reset();
    assert(game.lives==3&&game.score==0&&game.remaining==112);
    assert(game.wall==1&&Breakout::kColumns==14&&Breakout::kRows==8);
    assert(Breakout::BrickValue(0)==7&&Breakout::BrickValue(2)==5);
    assert(Breakout::BrickValue(4)==3&&Breakout::BrickValue(7)==1);
    assert(!game.moving&&!game.finished);
    game.SetPaddle(.5f);
    game.Step(.05f,0.f);
    assert(game.ballX==game.paddle&&!game.moving);
    game.Press();
    assert(game.moving);
    Breakout split=game;
    game.Step(.05f,.45f);
    split.Step(.025f,.45f);
    split.Step(.025f,.45f);
    assert(std::fabs(game.ballX-split.ballX)<.0001f);
    assert(std::fabs(game.ballY-split.ballY)<.0001f);
    assert(game.score==split.score&&game.remaining==split.remaining);
    assert(game.paddle>0.f);
    const float oldPaddle=game.paddle;
    game.SetPaddle(INFINITY);
    assert(game.paddle==oldPaddle);
    game.SetPaddle(-999.f);
    assert(game.paddle>=-.77f&&game.paddle<=.77f);

    // One wall bounce must keep the ball inside its court.
    game.ballX=.94f;game.ballY=-.2f;game.velocityX=.5f;
    game.velocityY=.7f;
    game.Tick();
    assert(game.ballX<=.94f&&game.velocityX<0.f);
    // A real brick collision changes exactly one tile and awards points.
    game.Reset();game.Press();
    game.ballX=Breakout::BrickX(0);
    game.ballY=Breakout::BrickY(0)-.080f;
    game.velocityY=.7f;
    game.Tick();
    assert(game.bricks[0]==0&&game.remaining==111&&game.score==7);
    assert(game.velocityY<0.f);
    // Paddle contact reverses descent and permits directional deflection.
    game.ballX=game.paddle+.1f;
    game.ballY=-.75f;game.velocityY=-.7f;
    game.Tick();
    assert(game.velocityY>0.f&&game.lives==3);

    // Falling below the bottom consumes exactly one life, then requires
    // an explicit new serve; a finished match restarts with a press.
    game.ballY=-.986f;game.ballX=.5f;
    game.velocityY=-.7f;
    game.Tick();
    assert(game.lives==2&&!game.moving&&!game.finished);
    game.Press();assert(game.moving);
    game.lives=1;game.ballY=-.986f;game.velocityY=-.7f;
    game.Tick();
    assert(game.lives==0&&game.finished&&!game.won);
    game.Press();assert(game.lives==3&&!game.finished&&!game.moving);

    // Reaching the top wall after a red-row hit halves the paddle.
    game.Press();game.topRowBroken=true;game.ballY=.915f;
    game.velocityY=.7f;game.Tick();
    assert(game.narrowPaddle&&game.PaddleWidth()<.25f);
    // Original two-wall campaign: first clearance installs the second
    // full 112-brick wall; only the second clearance wins.
    game.bricks.fill(0);game.bricks[9]=1;game.remaining=1;
    game.ballX=Breakout::BrickX(9);
    game.ballY=Breakout::BrickY(0)-.080f;
    game.velocityY=.7f;
    game.Tick();
    assert(game.wall==2&&game.remaining==112&&game.score==7);
    assert(!game.finished&&game.moving);
    game.bricks.fill(0);game.bricks[9]=1;game.remaining=1;
    game.ballX=Breakout::BrickX(9);
    game.ballY=Breakout::BrickY(0)-.080f;
    game.velocityY=.7f;game.Tick();
    assert(game.remaining==0&&game.score==14);
    assert(game.finished&&game.won&&!game.moving);
    game.Press();
    assert(game.remaining==112&&game.wall==1&&!game.finished);
    std::cout<<"Classic Breakout physics, scoring and lives passed\\n";
}

int main() {
    TestFlatGeometry();
    TestBreakout();
    State a,b;
    a.Reset(Game::Pong);b.Reset(Game::Pong);
    assert(!a.moving && !a.finished);
    a.Press();b.Press();
    for(int i=0;i<120;i++) {
        a.Step(.05f,.4f);
        b.Step(.025f,.4f);
        b.Step(.025f,.4f);
    }
    assert(std::isfinite(a.ball_x)&&a.score+a.opponent>=0);
    assert(a.score==b.score&&a.opponent==b.opponent);
    // Full arcade court: opposing side paddles, ricochet off ceiling,
    // first to eleven instead of the seven-point prototype.
    State pong;
    pong.Reset(Game::Pong);
    pong.Press();pong.paddle=.2f;pong.ball_x=-.87f;
    pong.ball_y=.20f;pong.velocity_x=-.55f;
    pong.velocity_y=0.f;
    pong.Step(.02f,0.f);
    assert(pong.velocity_x>0.f); // human paddle returned the ball
    pong.Reset(Game::Pong);pong.Press();
    pong.score=10;pong.ball_x=1.045f;pong.ball_y=.60f;
    pong.velocity_x=.55f;pong.velocity_y=0.f;
    pong.Step(.02f,0.f);
    assert(pong.score==11&&pong.finished);
    pong.Press();
    assert(pong.score==0&&!pong.finished&&!pong.moving);
    pong.Press();pong.opponent=10;pong.ball_x=-1.045f;
    pong.ball_y=.60f;pong.velocity_x=-.55f;pong.velocity_y=0.f;
    pong.Step(.02f,0.f);
    assert(pong.opponent==11&&pong.finished);
    State snake;
    snake.Reset(Game::Snake);
    assert(snake.length==3&&snake.food>=0&&snake.food<State::kCells);
    snake.Press();
    snake.Direction(3);
    assert(snake.desired==1); // cannot reverse into the body
    for(int tick=0;tick<3;tick++)snake.Step(.1f,0);
    assert(snake.snake[0]==6*12+6);
    snake.Direction(0);
    for(int tick=0;tick<3;tick++)snake.Step(.1f,0);
    assert(snake.snake[0]==5*12+6);
    snake.Direction(3);
    for(int i=0;i<20&&!snake.finished;i++)snake.Step(.25f,0);
    assert(snake.finished);
    snake.Press();
    assert(!snake.finished&&!snake.moving&&snake.length==3);
    // Near-full boards cannot falsely win due to occupied-cell RNG retries.
    State crowded;
    crowded.Reset(Game::Snake);
    crowded.length=State::kCells-1;
    crowded.snake.fill(-1);
    for(int i=0;i<State::kCells-1;i++)crowded.snake[i]=i;
    for(uint32_t seed=1;seed<=128;seed++) {
        crowded.seed=seed;
        crowded.finished=false;
        crowded.NewFood();
        assert(!crowded.finished&&crowded.food==State::kCells-1);
    }
    crowded.length=State::kCells-2;
    crowded.snake[State::kCells-2]=-1;
    bool found_penultimate=false,found_last=false;
    for(uint32_t seed=1;seed<=128;seed++) {
        crowded.seed=seed;
        crowded.finished=false;
        crowded.NewFood();
        assert(!crowded.finished);
        assert(crowded.food==State::kCells-2 ||
               crowded.food==State::kCells-1);
        found_penultimate|=crowded.food==State::kCells-2;
        found_last|=crowded.food==State::kCells-1;
    }
    assert(found_penultimate&&found_last);
    crowded.length=State::kCells;
    crowded.snake[State::kCells-2]=State::kCells-2;
    crowded.snake[State::kCells-1]=State::kCells-1;
    crowded.finished=false;
    crowded.NewFood();
    assert(crowded.finished&&crowded.food==-1);

    assert(State::Jump(3)==22&&State::Jump(17)==7);
    assert(State::Jump(45)==45&&State::Jump(99)==79);
    State ladder;
    ladder.Reset(Game::ChutesLadders);
    assert(ladder.player_turn&&ladder.position==0);
    ladder.Press();
    assert(!ladder.player_turn&&ladder.die>=1&&ladder.die<=6);
    int pos=ladder.position;
    ladder.Press();
    assert(ladder.position==pos); // cannot take the opponent's turn
    for(int tick=0;tick<8;tick++)ladder.Step(.1f,0);
    assert(ladder.player_turn&&ladder.rival>=1);
    for(int i=0;i<300&&!ladder.finished;i++) {
        if(ladder.player_turn)ladder.Press();
        else for(int tick=0;tick<8;tick++)ladder.Step(.1f,0);
    }
    assert(ladder.finished&&((ladder.position==100)||(ladder.rival==100)));
    ladder.Press();
    assert(!ladder.finished&&ladder.position==0&&ladder.rival==0);
    TestBoardGames();
    std::cout<<"Classic games deterministic simulation passed\n";
}
