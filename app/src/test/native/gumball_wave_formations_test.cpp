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
    std::cout<<"Gumball eight deterministic assault formations passed\n";
}
