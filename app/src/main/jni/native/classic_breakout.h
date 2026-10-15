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
    static constexpr int kColumns=14, kRows=8, kBricks=kColumns*kRows;
    static constexpr float kTick=.01f;
    std::array<uint8_t,kBricks> bricks{};
    float paddle=0.f, ballX=0.f, ballY=-.71f;
    float velocityX=.37f, velocityY=.70f, accumulator=0.f;
    int lives=3, score=0, remaining=kBricks, wall=1, paddleHits=0;
    bool narrowPaddle=false, topRowBroken=false;
    bool moving=false, finished=false, won=false;

    static float BrickX(int column) { return -.884f+.136f*column; }
    static float BrickY(int row) { return .76f-.119f*row; }
    static int BrickValue(int row) { return row<2?7:(row<4?5:(row<6?3:1)); }
    float PaddleWidth() const {return narrowPaddle?.22f:.40f;}
    void Boost(float factor){
        velocityX=std::clamp(velocityX*factor,-1.15f,1.15f);
        velocityY=std::clamp(velocityY*factor,-1.15f,1.15f);
    }

    void Reset() {
        bricks.fill(1);
        paddle=ballX=accumulator=0.f;
        ballY=-.71f;
        velocityX=.37f;
        velocityY=.70f;
        lives=3;score=0;remaining=kBricks;wall=1;paddleHits=0;
        narrowPaddle=topRowBroken=false;
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
        if(ballY>= .92f){
            ballY=.92f;velocityY=-std::fabs(velocityY);
            if(topRowBroken)narrowPaddle=true;
        }

        for(int i=0;i<kBricks;i++)if(bricks[i]){
            const float x=BrickX(i%kColumns),y=BrickY(i/kColumns);
            if(std::fabs(ballX-x)>.101f||std::fabs(ballY-y)>.092f)
                continue;
            bricks[i]=0;--remaining;score+=BrickValue(i/kColumns);
            if(i/kColumns<2)topRowBroken=true;
            if(i/kColumns<4)Boost(1.04f);
            // The approach side determines which component bounces.
            if(oldX<x-.101f||oldX>x+.101f)velocityX=-velocityX;
            else velocityY=-velocityY;
            ballX=oldX;ballY=oldY;
            if(remaining==0){
                if(wall==1){
                    ++wall;bricks.fill(1);remaining=kBricks;
                }else{moving=false;finished=won=true;}
            }
            break;
        }
        if(moving&&velocityY<0.f&&oldY>=-.754f&&ballY<=-.754f &&
           std::fabs(ballX-paddle)<=PaddleWidth()*.5f+.035f){
            ballY=-.754f;
            velocityY=std::fabs(velocityY);
            velocityX=std::clamp(velocityX+(ballX-paddle)*.54f,
                                  -.76f,.76f);
            // Never allow a perfectly vertical stalemate.
            if(std::fabs(velocityX)<.10f)
                velocityX=velocityX<0.f?-.10f:.10f;
            ++paddleHits;
            if(paddleHits==4||paddleHits==12)Boost(1.10f);
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
