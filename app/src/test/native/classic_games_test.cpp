#include "classic_games.h"
#include "classic_board_games.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::classics;

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

    Boards go;
    go.Reset(BoardKind::Go);
    assert(go.Size()==9);
    go.cells[40]=-1;go.cells[31]=1;go.cells[39]=1;go.cells[41]=1;
    assert(go.GoMove(49)); // fourth liberty removed captures black
    assert(go.cells[40]==0&&go.cells[49]==1);
    assert(!go.GoMove(49)); // cannot place on occupied point
    Boards suicide;
    suicide.Reset(BoardKind::Go);
    suicide.cells[31]=-1;suicide.cells[39]=-1;
    suicide.cells[41]=-1;suicide.cells[49]=-1;
    assert(!suicide.GoMove(40));
    // Simple ko: immediate board-position recreation is forbidden.
    Boards ko;
    ko.Reset(BoardKind::Go);
    ko.cells[2*9+2]=1;
    ko.cells[1*9+2]=-1;ko.cells[2*9+1]=-1;ko.cells[2*9+3]=-1;
    ko.cells[3*9+1]=1;ko.cells[3*9+3]=1;ko.cells[4*9+2]=1;
    ko.side=-1;
    assert(ko.GoMove(3*9+2));
    assert(ko.cells[2*9+2]==0);
    assert(!ko.GoMove(2*9+2));
    Boards emptyGo;
    emptyGo.Reset(BoardKind::Go);
    assert(emptyGo.Pass()&&!emptyGo.finished);
    assert(emptyGo.Pass()&&emptyGo.finished&&emptyGo.winner==-1); // komi
    Boards cpuGo;
    cpuGo.Reset(BoardKind::Go);
    assert(cpuGo.GoMove(40));
    for(int i=0;i<3;i++)cpuGo.Step(.1f);
    assert(cpuGo.ply==2&&cpuGo.side==1);

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

int main() {
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
