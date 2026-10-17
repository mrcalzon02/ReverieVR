#include "omega_front_campaign_save.h"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace reverie::omega;

static void Reseal(CampaignSave::Bytes &bytes) {
    uint64_t hash=UINT64_C(14695981039346656037);
    for(size_t i=0;i<bytes.size()-8;++i) {
        hash^=bytes[i];
        hash*=UINT64_C(1099511628211);
    }
    for(size_t i=bytes.size()-8;i<bytes.size();++i) {
        bytes[i]=static_cast<uint8_t>(hash);
        hash>>=8;
    }
}
static Session AtOperation(int op) {
    Session seed(414);
    CampaignSave::Bytes data{};
    assert(CampaignSave::Encode(seed,&data));
    data[28]=static_cast<uint8_t>(op); // fixture-only campaign stage
    data[39]=7;                        // actual Hearthwell waterworks damage
    Reseal(data);
    Session configured(1);
    assert(CampaignSave::Decode(&configured,data.data(),data.size()));
    assert(configured.frontier.operation()==op);
    assert(configured.frontier.pump_health()==7);
    return configured;
}
static void MoveToBattle(Frontier &f,int op) {
    if(op==0 || op==2) assert(f.Travel(10));
    else if(op==1) assert(f.Travel(5));
    else if(op==3) assert(f.Travel(10) && f.Travel(9));
    else if(op==4) assert(f.Travel(7));
    else if(op==5) assert(f.Travel(5) && f.Travel(4));
}
int main() {
    // Only operations physically at S011 use and modify the real waterworks.
    for(int op=0;op<kHearthfallOperations;++op) {
        Session game=AtOperation(op);
        Frontier &front=game.frontier;
        MoveToBattle(front,op);
        assert(front.pump_health()==7);
        assert(front.StartOperation());
        const bool atPump=InfrastructureScope::UsesHearthwellPump(op);
        Battle *fight=front.current_battle();
        assert(fight && fight->pump_hp()==(atPump?7:18));
        for(int tick=0;tick<160 && fight->result()==Result::Active;++tick)
            fight->Step(); // real enemy attacks and mission timeouts
        assert(fight->result()!=Result::Active);
        const int siteHealth=fight->pump_hp();
        assert(front.ResolveOperation());
        assert(front.last_report().pump_remaining==siteHealth);
        assert(front.pump_health()==(atPump?siteHealth:7));
        CampaignSave::Bytes bytes{};
        assert(CampaignSave::Encode(game,&bytes));
        Session reloaded(8);
        assert(CampaignSave::Decode(&reloaded,bytes.data(),bytes.size()));
        assert(reloaded.frontier.pump_health()==front.pump_health());
        assert(reloaded.Checksum()==game.Checksum());
    }
    std::cout<<"Omega Front infrastructure boundaries and save roundtrip passed\n";
}
