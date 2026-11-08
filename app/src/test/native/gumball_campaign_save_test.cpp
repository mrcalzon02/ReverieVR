#include "gumball_campaign_save.h"
#include <cassert>
#include <cstdio>
#include <iostream>
using namespace reverie::gumball;
static CampaignSave::Bytes Bytes(const Simulation& s,const ShipAssembly& ship){
    CampaignSave::Bytes bytes{};
    if(!CampaignSave::Encode(s,ship,&bytes)){
        std::fprintf(stderr,"ENCODE_FAILED phase=%d time=%f target=%d spawned=%d defeated=%d hp=%d\n",
            static_cast<int>(s.phase()),s.elapsed(),s.target(),s.spawned(),s.defeated(),s.hp());
        assert(false);
    }
    return bytes;
}
static void Continue(Simulation& game,const ShipAssembly& ship,unsigned ticks){
    Simulation resumed;ShipAssembly rebuilt;
    auto before=Bytes(game,ship);
    assert(CampaignSave::Decode(&resumed,&rebuilt,before.data(),before.size()));
    assert(Bytes(resumed,rebuilt)==before);
    for(unsigned i=0;i<ticks;++i){
        const float x=i%61<31?.7f:-.7f,y=i%113<57?.2f:-.2f;
        const bool fire=i%7!=0;
        game.update(.025f,x,y,fire);resumed.update(.025f,x,y,fire);
        assert(Bytes(game,ship)==Bytes(resumed,rebuilt));
    }
}
int main(){
    ShipAssembly ship;
    assert(ship.edit(4,2,2,Part::Armor)==ShipEdit::Placed);
    Simulation starter(109);assert(starter.configure_ship_profile(ship.profile(),ship.ready()));
    assert(starter.hire_support(SupportKind::CapitaineEclair));
    Continue(starter,ship,0);
    assert(starter.launch(0));
    for(int i=0;i<240;++i)starter.update(.025f,0.f,0.f,false);
    assert(starter.phase()==Phase::Flight&&starter.support_pulses()>0);
    const auto active=Bytes(starter,ship);
    Continue(starter,ship,1800);
    assert(starter.phase()!=Phase::Flight);
    const auto cash=starter.cash();Continue(starter,ship,100);
    assert(starter.cash()==cash); // Result reload cannot award again.
    assert(starter.recover());Continue(starter,ship,0);
    // Repeated snapshots include all named guest and mission cases, varied
    // RNG, salvage, shots and timers; identical future input must stay exact.
    unsigned guests=0;
    for(unsigned seed=1;seed<=36;++seed){
        Simulation game(seed);auto progress=game.snapshot();
        progress.visited=3;progress.cash=5000;progress.levels[1]=8;progress.levels[3]=7;
        if(seed%7==0)progress.node=1022;
        progress.checksum=SnapshotChecksum(progress);assert(game.restore(progress));
        assert(game.configure_ship_profile(ship.profile(),ship.ready()));
        assert(game.hire_support(seed%2?SupportKind::CapitaineEclair:SupportKind::MademoiselleMacaron));
        assert(game.buy_support_kit(SupportKit::HullPatch));
        assert(game.buy_support_kit(SupportKit::ShieldTonic));
        const unsigned route=seed%3;
        guests|=1u<<static_cast<unsigned>(ChooseCastGuest(seed,game.lane(route).id,3,game.lane(route).hazard));
        assert(game.launch(route));
        for(unsigned i=0;i<seed*5;++i)game.update(.025f,.35f,0.f,i%3!=0);
        Continue(game,ship,800);
    }
    assert(guests==511u);
    Simulation unchanged(77);ShipAssembly unchangedShip;
    const auto unchangedBytes=Bytes(unchanged,unchangedShip);
    for(size_t i=0;i<active.size();++i){
        auto broken=active;broken[i]^=1u;
        assert(!CampaignSave::Decode(&unchanged,&unchangedShip,broken.data(),broken.size()));
        assert(Bytes(unchanged,unchangedShip)==unchangedBytes);
    }
    assert(!CampaignSave::Decode(&unchanged,&unchangedShip,active.data(),active.size()-1));
    // A correctly rehashed impossible phase must also fail semantic checks.
    auto invalid=active;invalid[12]=255;
    uint64_t hash=14695981039346656037ull;
    for(size_t i=0;i<invalid.size()-8;++i)hash=(hash^invalid[i])*1099511628211ull;
    for(size_t i=invalid.size()-8;i<invalid.size();++i){invalid[i]=static_cast<uint8_t>(hash);hash>>=8;}
    assert(!CampaignSave::Decode(&unchanged,&unchangedShip,invalid.data(),invalid.size()));
    auto invalidFloat=unchangedBytes;
    // Mission time limit is binary32 at offset 58, including a fresh chart.
    invalidFloat[58]=0;invalidFloat[59]=0;invalidFloat[60]=0xc0;invalidFloat[61]=0x7f;
    hash=14695981039346656037ull;
    for(size_t i=0;i<invalidFloat.size()-8;++i)hash=(hash^invalidFloat[i])*1099511628211ull;
    for(size_t i=invalidFloat.size()-8;i<invalidFloat.size();++i){invalidFloat[i]=static_cast<uint8_t>(hash);hash>>=8;}
    assert(!CampaignSave::Decode(&unchanged,&unchangedShip,invalidFloat.data(),invalidFloat.size()));
    std::cout<<"PASS Gumball full live sortie/ship/checkpoint next-state determinism, all nine guests, paid support, corrupt transactional rejection\n";
}
