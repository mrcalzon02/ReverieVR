#include "gumball_wave_formations.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
using namespace reverie::gumball;
int main(){
    bool observed[8]={};
    for(unsigned lane=0;lane<4096;lane+=29){
        for(int spawn=0;spawn<40;++spawn){
            const auto a=PlanWaveEntry(42u,lane,spawn,40,static_cast<unsigned>(spawn*31));
            const auto b=PlanWaveEntry(42u,lane,spawn,40,static_cast<unsigned>(spawn*31));
            assert(a.x==b.x&&a.y==b.y&&a.next_delay==b.next_delay);
            assert(std::isfinite(a.x)&&std::fabs(a.x)<=1.7f);
            assert(a.y>=.98f&&a.y<=1.35f);
            assert(a.next_delay>=.7f&&a.next_delay<=2.90f);
            observed[static_cast<unsigned>(a.formation)]=true;
            assert(std::strlen(FormationName(a.formation))>=3u);
            assert(a.formation==FormationFor(42u,lane,spawn));
            if(spawn%5!=4)assert(a.formation==FormationFor(42u,lane,spawn+1));
        }
    }
    for(bool found:observed)assert(found);
    // Geometry is genuinely differentiated: all eight doctrines exist and
    // exhibit distinct placements, even without relying on random jitter.
    bool shapeSeen[8]={};
    for(unsigned start=0;start<8;++start){
        unsigned lane=0;
        while(static_cast<unsigned>(FormationFor(91u,lane,0))!=start)++lane;
        const auto p0=PlanWaveEntry(91u,lane,0,10,6u);
        const auto p1=PlanWaveEntry(91u,lane,1,10,6u);
        const auto p4=PlanWaveEntry(91u,lane,4,10,6u);
        shapeSeen[start]=p0.x!=p1.x||p1.x!=p4.x||start==0;
        assert(p4.next_delay==2.90f);
        assert(p0.next_delay<1.f);
        const auto next=PlanWaveEntry(91u,lane,5,10,6u);
        assert(static_cast<unsigned>(next.formation)==(start+1u)%8u);
        if(start==static_cast<unsigned>(WaveFormation::RearPressure))
            assert(p0.y==.98f);
    }
    for(bool found:shapeSeen)assert(found);
    // Every doctrine has an actual bounded steering and firing identity.
    for(unsigned f=0;f<8;++f)for(unsigned slot=0;slot<5;++slot)
        for(int tick=0;tick<400;++tick){
            const float age=tick*.025f;
            const WaveTactics a=TacticsFor(static_cast<WaveFormation>(f),
                                           slot,age,-1.1f,.9f);
            const WaveTactics b=TacticsFor(static_cast<WaveFormation>(f),
                                           slot,age,-1.1f,.9f);
            assert(a.lateral_speed==b.lateral_speed&&a.shot_aim==b.shot_aim);
            assert(std::isfinite(a.lateral_speed)
                   &&std::fabs(a.lateral_speed)<=.52f);
            assert(std::isfinite(a.shot_aim)&&std::fabs(a.shot_aim)<=.23f);
            assert(a.fire_interval_scale>=.93f&&a.fire_interval_scale<=1.08f);
        }
    const auto line=TacticsFor(WaveFormation::Column,0,1.f,1.f,-1.f);
    const auto pincer=TacticsFor(WaveFormation::Pincer,0,1.f,1.f,-1.f);
    const auto rear=TacticsFor(WaveFormation::RearPressure,0,1.f,1.f,-1.f);
    const auto crossLeft=TacticsFor(WaveFormation::Crossfire,0,1.f,-1.f,0.f);
    const auto crossRight=TacticsFor(WaveFormation::Crossfire,1,1.f,1.f,0.f);
    const auto escort=TacticsFor(WaveFormation::Escort,2,1.f,1.f,-1.f);
    assert(line.lateral_speed==0.f&&line.shot_aim==0.f);
    assert(pincer.lateral_speed<0.f&&pincer.shot_aim<0.f);
    assert(rear.lateral_speed<pincer.lateral_speed
           &&rear.fire_interval_scale<1.f);
    assert(crossLeft.lateral_speed>0.f&&crossRight.lateral_speed<0.f);
    assert(escort.lateral_speed==0.f&&escort.fire_interval_scale>1.f);
    const auto bad=TacticsFor(WaveFormation::Spiral,0,NAN,1.f,0.f);
    assert(bad.lateral_speed==0.f&&bad.shot_aim==0.f);
    std::cout<<"Gumball eight deterministic assault formations passed\n";
}
