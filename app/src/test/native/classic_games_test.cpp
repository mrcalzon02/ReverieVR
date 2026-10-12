#include "classic_games.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::classics;
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
    std::cout<<"Classic games deterministic simulation passed\n";
}
