#ifndef REVERIE_CLASSIC_BREAKOUT_H
#define REVERIE_CLASSIC_BREAKOUT_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace reverie { namespace classics {

// Original low-poly Breakout-style arcade game. Simulation is independent of
// GL, eye rendering, Android input and the host's module lifetime.
struct Breakout {
    static constexpr int kColumns=10, kRows=5, kBricks=kColumns*kRows;
    static constexpr float kTick=.01f;
    std::array<uint8_t,kBricks> bricks{};
    float paddle=0.f, ballX=0.f, ballY=-.71f;
    float velocityX=.37f, velocityY=.70f, accumulator=0.f;
    int lives=3, score=0, remaining=kBricks;
    bool moving=false, finished=false, won=false;

    static float BrickX(int column) { return -.855f+.19f*column; }
    static float BrickY(int row) { return .73f-.145f*row; }

    void Reset() {
        bricks.fill(1);
        paddle=ballX=accumulator=0.f;
        ballY=-.71f;
        velocityX=.37f;
        velocityY=.70f;
        lives=3;score=0;remaining=kBricks;
        moving=finished=won=false;
    }
    void SetPaddle(float x) {
        if(std::isfinite(x))paddle=std::clamp(x,-.77f,.77f);
        if(!moving&&!finished)ballX=paddle;
    }
    void Press() {
        if(finished){Reset();return;}
        if(!moving&&lives>0){
            ballX=paddle;ballY=-.71f;
            velocityX=(score%2==0?.37f:-.37f);
            velocityY=.70f;
            accumulator=0.f;
            moving=true;
        }
    }
    // Fixed 10 ms physics ticks make split updates deterministic and avoid
    // tunneling through the 0.1-unit rows on slower reference devices.
    void Step(float seconds,float axis) {
        if(!std::isfinite(seconds)||seconds<=0.f)return;
        const float dt=std::min(seconds,.1f);
        if(std::isfinite(axis)&&std::fabs(axis)>.12f)
            SetPaddle(paddle+axis*dt*1.7f);
        if(!moving||finished){if(!finished)ballX=paddle;return;}
        accumulator+=dt;
        int ticks=0;
        while(accumulator+1e-6f>=kTick&&ticks++<11&&moving&&!finished){
            accumulator-=kTick;
            Tick();
        }
        if(accumulator<0.f)accumulator=0.f;
    }
    void Tick() {
        const float oldX=ballX,oldY=ballY;
        ballX+=velocityX*kTick;
        ballY+=velocityY*kTick;
        if(ballX<=-.94f){ballX=-.94f;velocityX=std::fabs(velocityX);}
        if(ballX>= .94f){ballX= .94f;velocityX=-std::fabs(velocityX);}
        if(ballY>= .92f){ballY=.92f;velocityY=-std::fabs(velocityY);}

        for(int i=0;i<kBricks;i++)if(bricks[i]){
            const float x=BrickX(i%kColumns),y=BrickY(i/kColumns);
            if(std::fabs(ballX-x)>.108f||std::fabs(ballY-y)>.081f)
                continue;
            bricks[i]=0;--remaining;score+=10;
            // The approach side determines which component bounces.
            if(oldX<x-.108f||oldX>x+.108f)velocityX=-velocityX;
            else velocityY=-velocityY;
            ballX=oldX;ballY=oldY;
            if(remaining==0){moving=false;finished=won=true;}
            break;
        }
        if(moving&&velocityY<0.f&&oldY>=-.754f&&ballY<=-.754f &&
           std::fabs(ballX-paddle)<=.235f){
            ballY=-.754f;
            velocityY=std::fabs(velocityY);
            velocityX=std::clamp(velocityX+(ballX-paddle)*.54f,
                                  -.76f,.76f);
            // Never allow a perfectly vertical stalemate.
            if(std::fabs(velocityX)<.10f)
                velocityX=velocityX<0.f?-.10f:.10f;
        }
        if(moving&&ballY<-.99f){
            --lives;
            moving=false;accumulator=0.f;ballX=paddle;ballY=-.71f;
            if(lives<=0){lives=0;finished=true;won=false;}
        }
    }
};
}} // namespace reverie::classics
#endif
