#include "breakwater_wave_schedule.h"
#include <cassert>
#include <iostream>
#include <cstdint>
using reverie::breakwater::waves::ForPeriod;
int main(){
    const uint8_t days[3]={2,3,4};
    const unsigned expected[4]={244u,280u,316u,352u};
    for(unsigned tier=0;tier<4;++tier){
        unsigned total=0;
        unsigned periods=0;
        for(uint8_t location=0;location<3;++location)
            for(uint8_t day=1;day<=days[location];++day) {
                const uint8_t daytime=ForPeriod(location,day,false,tier);
                const uint8_t nighttime=ForPeriod(location,day,true,tier);
                assert(daytime>nighttime && daytime<=32u);
                assert(nighttime>=8u && nighttime<=32u);
                // Every actual stored progress value in both periods must
                // reload, including waves 9..25 omitted by the legacy
                // eight-wave checkpoint cap.
                for(uint32_t wave=1;wave<=daytime;++wave)
                    assert(reverie::breakwater::waves::CheckpointAllowed(
                        wave,location,day,false,tier));
                for(uint32_t wave=1;wave<=nighttime;++wave)
                    assert(reverie::breakwater::waves::CheckpointAllowed(
                        wave,location,day,true,tier));
                for(bool night:{false,true}) {
                    const uint32_t last=night?nighttime:daytime;
                    assert(!reverie::breakwater::waves::CheckpointAllowed(
                        0u,location,day,night,tier));
                    assert(!reverie::breakwater::waves::CheckpointAllowed(
                        last+1u,location,day,night,tier));
                    assert(!reverie::breakwater::waves::CheckpointAllowed(
                        257u,location,day,night,tier));
                    assert(!reverie::breakwater::waves::CheckpointAllowed(
                        65537u,location,day,night,tier));
                    assert(!reverie::breakwater::waves::CheckpointAllowed(
                        0xffffffffu,location,day,night,tier));
                }
                total+=daytime+nighttime;
                periods+=2u;
            }
        assert(periods==18u && total==expected[tier]);
        assert(ForPeriod(0,1,false,tier)==12u+2u*tier);
        assert(ForPeriod(0,1,true,tier)==8u+2u*tier);
    }
    static_assert(ForPeriod(0,1,false,0)==12u,"Cadet");
    static_assert(ForPeriod(0,1,false,3)==18u,"Siege");
    static_assert(ForPeriod(2,4,false,3)==25u,"Final daytime");
    static_assert(ForPeriod(2,4,true,3)==21u,"Final night");
    assert(ForPeriod(255,255,false,255)==32u);
    static_assert(reverie::breakwater::waves::CheckpointAllowed(
        25u,2u,4u,false,3u),"Siege late daytime save must load");
    static_assert(!reverie::breakwater::waves::CheckpointAllowed(
        26u,2u,4u,false,3u),"Siege beyond-day wave must reject");
    static_assert(reverie::breakwater::waves::CheckpointAllowed(
        21u,2u,4u,true,3u),"Siege final night save must load");
    static_assert(!reverie::breakwater::waves::CheckpointAllowed(
        22u,2u,4u,true,3u),"Siege night-overflow must reject");
    std::cout<<"Breakwater 18-period schedule: 244-352 distinct waves passed\n";
}
