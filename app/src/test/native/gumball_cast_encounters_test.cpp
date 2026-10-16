#include "gumball_cast_encounters.h"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace reverie::gumball;
int main(){
    assert(ChooseCastGuest(1,37,0,2)==CastGuest::None);
    CastEncounter unused;unused.begin(1,37,0,2);
    for(int i=0;i<200;++i)unused.update(.05f);
    assert(!unused.active()&&!unused.claim_pulse());
    bool seen[4]={};
    for(uint32_t seed=1;seed<=512;++seed){
        const CastGuest role=ChooseCastGuest(seed,141,2,3);
        seen[static_cast<unsigned>(role)]=true;
        CastEncounter a,b;a.begin(seed,141,2,3);b.begin(seed,141,2,3);
        for(int i=0;i<250;++i){
            a.update(.05f);b.update(.05f);
            assert(a.kind()==b.kind()&&a.active()==b.active());
            assert(a.pulse_ready()==b.pulse_ready());
            if(a.pulse_ready()){assert(a.claim_pulse()&&b.claim_pulse());}
            assert(a.pulses()<=5);
        }
        assert(a.pulses()==b.pulses());
        if(role==CastGuest::PipParfait||role==CastGuest::LollyWren){
            assert(a.triggered()&&a.pulses()==5&&!a.active());
        } else assert(a.pulses()==0);
    }
    for(bool got:seen)assert(got);
    assert(std::strcmp(CastGuestName(CastGuest::PipParfait),"PIP PARFAIT")==0);
    CastEncounter sanity;sanity.begin(1,141,3,4);
    sanity.update(NAN);sanity.update(-1.f);
    assert(!sanity.triggered());
    std::cout<<"cast encounter deterministic schedule passes\n";
}
