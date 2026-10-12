#ifndef REVERIE_CLASSIC_GAMES_H
#define REVERIE_CLASSIC_GAMES_H
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace reverie { namespace classics {
enum class Game : uint8_t { Pong=0, Snake=1, ChutesLadders=2 };
struct State {
    static constexpr int kGrid=12, kCells=kGrid*kGrid;
    Game game=Game::Pong;
    uint32_t seed=0x5a17c0deu;
    int score=0, opponent=0;
    bool finished=false, moving=false, player_turn=true;
    float clock=0, paddle=0, ai=0, ball_x=0, ball_y=0;
    float velocity_x=.46f, velocity_y=-.67f;
    int length=3, heading=1, desired=1, food=0;
    std::array<int,kCells> snake{};
    int position=0, rival=0, die=0;
    uint32_t Random() {
        seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
        return seed;
    }
    void NewFood() {
        // Select from the actual free cells. Repeated random retries can
        // falsely end a nearly full board even while food space remains.
        // One seeded draw and a bounded stack-only scan are deterministic.
        std::array<uint8_t,kCells> occupied{};
        for(int i=0;i<length && i<kCells;i++) {
            const int cell=snake[i];
            if(cell>=0 && cell<kCells)occupied[cell]=1;
        }
        int available=0;
        for(uint8_t used:occupied)if(!used)++available;
        if(available==0){food=-1;finished=true;return;}
        int pick=static_cast<int>(Random()%static_cast<uint32_t>(available));
        for(int cell=0;cell<kCells;cell++) {
            if(!occupied[cell] && pick--==0){food=cell;return;}
        }
    }
    void Reset(Game target) {
        game=target;score=opponent=0;finished=moving=false;
        player_turn=true;clock=0;paddle=ai=ball_x=ball_y=0;
        velocity_x=.46f;velocity_y=-.67f;
        length=3;heading=desired=1;
        snake.fill(-1);
        snake[0]=6*kGrid+5;snake[1]=6*kGrid+4;snake[2]=6*kGrid+3;
        position=rival=die=0;
        if(target==Game::Snake)NewFood();
    }
    void Direction(int target) {
        if(target>=0 && target<4 && (target+2)%4!=heading)
            desired=target;
    }
    void Press() {
        if(finished){Reset(game);return;}
        if(game==Game::Pong){moving=true;return;}
        if(game==Game::Snake){moving=true;return;}
        if(!player_turn)return;
        die=static_cast<int>(Random()%6u)+1;
        position=Jump(std::min(100,position+die));
        if(position==100)finished=true;
        player_turn=false;clock=0;
    }
    static int Jump(int tile) {
        switch(tile) {
            case 3:return 22;case 8:return 30;case 28:return 55;
            case 58:return 77;case 75:return 94;
            case 17:return 7;case 54:return 34;case 62:return 18;
            case 87:return 36;case 99:return 79;
            default:return tile;
        }
    }
    void Step(float seconds,float axis) {
        if(!std::isfinite(seconds)||seconds<=0)return;
        const float dt=std::min(seconds,.1f);
        if(game==Game::Pong) {
            if(std::isfinite(axis)&&std::abs(axis)>.12f)
                paddle=std::clamp(paddle+axis*dt*1.8f,-.78f,.78f);
            ai=std::clamp(ai+std::clamp(ball_x-ai,-dt*.87f,dt*.87f),-.78f,.78f);
            if(!moving||finished)return;
            ball_x+=velocity_x*dt;
            ball_y+=velocity_y*dt;
            if(ball_x> .96f){ball_x=.96f;velocity_x=-std::abs(velocity_x);}
            if(ball_x<-.96f){ball_x=-.96f;velocity_x= std::abs(velocity_x);}
            if(ball_y<-.87f) {
                if(std::abs(ball_x-paddle)<.26f) {
                    ball_y=-.87f;velocity_y=std::abs(velocity_y);
                    velocity_x+=(ball_x-paddle)*.31f;
                } else {++opponent;moving=false;ball_x=ball_y=0;}
            }
            if(ball_y>.87f) {
                if(std::abs(ball_x-ai)<.24f) {
                    ball_y=.87f;velocity_y=-std::abs(velocity_y);
                } else {++score;moving=false;ball_x=ball_y=0;}
            }
            finished=(score>=7||opponent>=7);
        } else if(game==Game::Snake) {
            if(!moving||finished)return;
            clock+=dt;
            if(clock<.24f)return;
            clock-=.24f;
            heading=desired;
            int x=snake[0]%kGrid,y=snake[0]/kGrid;
            if(heading==0)--y;
            if(heading==1)++x;
            if(heading==2)++y;
            if(heading==3)--x;
            if(x<0||x>=kGrid||y<0||y>=kGrid){finished=true;return;}
            const int target=y*kGrid+x;
            const bool eat=target==food;
            const int check=length-(eat?0:1);
            for(int i=0;i<check;i++)
                if(snake[i]==target){finished=true;return;}
            if(eat&&length<kCells)++length;
            for(int i=length-1;i>0;i--)snake[i]=snake[i-1];
            snake[0]=target;
            if(eat){++score;NewFood();}
        } else if(!finished&&!player_turn) {
            clock+=dt;
            if(clock>=.72f) {
                die=static_cast<int>(Random()%6u)+1;
                rival=Jump(std::min(100,rival+die));
                if(rival==100)finished=true;
                player_turn=true;clock=0;
            }
        }
    }
};
}} // namespace reverie::classics
#endif
