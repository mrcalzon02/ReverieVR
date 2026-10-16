#ifndef REVERIE_CLASSIC_GAMES_H
#define REVERIE_CLASSIC_GAMES_H
#include "classic_pong_dynamics.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace reverie { namespace classics {
enum class Game : uint8_t { Pong=0, Snake=1, ChutesLadders=2 };
struct State {
    static constexpr int kGrid=12, kCells=kGrid*kGrid;
    struct BoardLink {int from,to;};
    // Common 100-square Chutes and Ladders layout. Keep simulation and
    // immutable board rendering backed by this one authoritative table.
    inline static constexpr std::array<BoardLink,19> kBoardLinks{{
        {1,38},{4,14},{9,31},{21,42},{28,84},{36,44},
        {51,67},{71,91},{80,100},
        {16,6},{47,26},{49,11},{56,53},{62,19},
        {64,60},{87,24},{93,73},{95,75},{98,78}
    }};
    Game game=Game::Pong;
    uint32_t seed=0x5a17c0deu;
    int score=0, opponent=0;
    bool finished=false, moving=false, player_turn=true;
    float clock=0, paddle=0, ai=0, ball_x=0, ball_y=0;
    float velocity_x=pong::kServeHorizontalSpeed, velocity_y=pong::kServeVerticalSpeed;
    float previous_paddle=0.f; // last simulated frame: includes headset pointer motion
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
        velocity_x=pong::kServeHorizontalSpeed;
        velocity_y=pong::kServeVerticalSpeed;
        previous_paddle=0.f;
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
        position=AdvanceChutes(position,die);
        if(position==100)finished=true;
        player_turn=false;clock=0;
    }
    static int Jump(int tile) {
        for(const auto &link:kBoardLinks)if(link.from==tile)return link.to;
        return tile;
    }
    static int AdvanceChutes(int tile,int roll) {
        // Rule: an overshooting spin forfeits movement (including any
        // chute/ladder); square 80's ladder is the other way to win.
        if(tile<0||tile>=100||roll<1||roll>6||tile+roll>100)
            return tile;
        return Jump(tile+roll);
    }
    void Step(float seconds,float axis) {
        if(!std::isfinite(seconds)||seconds<=0)return;
        const float dt=std::min(seconds,.1f);
        if(game==Game::Pong) {
            // The renderer may reposition the paddle from the head-tracked
            // ray *before* Step. Sampling against the previous simulation
            // position captures that movement as well as touchpad motion.
            if(std::isfinite(axis)&&std::abs(axis)>.12f)
                paddle=std::clamp(paddle+axis*dt*1.8f,-.78f,.78f);
            const float player_speed=pong::PaddleVelocity(
                paddle-previous_paddle,dt);
            previous_paddle=paddle;
            const float previous_ai=ai;
            const float goal=pong::CpuTarget(ball_y,velocity_y,velocity_x,
                                            moving,score,opponent);
            const float limit=dt*pong::kCpuPaddleSpeed;
            ai=std::clamp(ai+std::clamp(goal-ai,-limit,limit),-.78f,.78f);
            const float cpu_speed=pong::PaddleVelocity(ai-previous_ai,dt);
            if(!moving||finished)return;
            const float previous_x=ball_x;
            ball_x+=velocity_x*dt;
            ball_y+=velocity_y*dt;
            if(ball_y>.93f){ball_y=.93f;velocity_y=-std::abs(velocity_y);}
            if(ball_y<-.93f){ball_y=-.93f;velocity_y=std::abs(velocity_y);}
            if(ball_x<-.87f&&previous_x>=-.87f&&velocity_x<0.f
               &&std::abs(ball_y-paddle)<pong::kPaddleHalfHeight) {
                ball_x=-.87f;
                const auto rebound=pong::Bounce(velocity_x,velocity_y,
                    ball_y-paddle,player_speed,true);
                velocity_x=rebound.vx;velocity_y=rebound.vy;
            }
            if(ball_x>.87f&&previous_x<=.87f&&velocity_x>0.f
               &&std::abs(ball_y-ai)<.26f) {
                ball_x=.87f;
                const auto rebound=pong::Bounce(velocity_x,velocity_y,
                    ball_y-ai,cpu_speed,false);
                velocity_x=rebound.vx;velocity_y=rebound.vy;
            }
            if(ball_x>1.05f) {
                ++score;moving=false;ball_x=ball_y=0;
                velocity_x=(score+opponent)%2 ? -pong::kServeHorizontalSpeed
                    : pong::kServeHorizontalSpeed;
                velocity_y=(score+opponent)%2 ? -pong::kServeVerticalSpeed
                    : pong::kServeVerticalSpeed;
            }
            if(ball_x< -1.05f) {
                ++opponent;moving=false;ball_x=ball_y=0;
                velocity_x=(score+opponent)%2 ? -pong::kServeHorizontalSpeed
                    : pong::kServeHorizontalSpeed;
                velocity_y=(score+opponent)%2 ? -pong::kServeVerticalSpeed
                    : pong::kServeVerticalSpeed;
            }
            finished=(score>=11||opponent>=11);
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
                rival=AdvanceChutes(rival,die);
                if(rival==100)finished=true;
                player_turn=true;clock=0;
            }
        }
    }
};
}} // namespace reverie::classics
#endif
