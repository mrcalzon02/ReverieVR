#include "omega_front_campaign_save.h"
#include <cassert>
#include <cstdint>
#include <iostream>

using namespace reverie::omega;

// Explicitly configure a valid campaign test fixture through the existing
// versioned codec. The production day advance and due-day settlement below
// always use the real Frontier methods, never hand-awarded resources.
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

static Session StartingCampaign(int pump_health,bool due_accord) {
    assert(pump_health==18 || pump_health==7 || pump_health==0);
    Session game(414);
    if(pump_health!=18 || due_accord) {
        CampaignSave::Bytes bytes{};
        assert(CampaignSave::Encode(game,&bytes));
        assert(bytes[8]==10); // v10 field layout, as in infrastructure tests
        bytes[39]=static_cast<uint8_t>(pump_health);
        if(due_accord)bytes[28]=3; // stage fixture, not an awarded victory
        Reseal(bytes);
        Session configured(20);
        assert(CampaignSave::Decode(&configured,bytes.data(),bytes.size()));
        game=configured;
    }
    assert(game.frontier.pump_health()==pump_health);
    if(due_accord) {
        // Form a genuine dated Scrapline obligation using paid travel,
        // scouting, negotiation and an accord, not a forged debt record.
        Frontier &f=game.frontier;
        assert(f.Travel(10));
        assert(f.Scout(9));
        assert(f.Negotiate(9));
        assert(f.ConcludeAccord(9));
        assert(f.operation()==4);
        assert(f.obligation_status(3)==Obligations::Due);
        assert(f.obligation_due_day(3)>f.day());
    }
    return game;
}

static void AssertSectorParity(const Session &a,const Session &b) {
    assert(a.Checksum()==b.Checksum());
    assert(a.frontier.Checksum()==b.frontier.Checksum());
    assert(a.frontier.day()==b.frontier.day());
    assert(a.frontier.population()==b.frontier.population());
    assert(a.frontier.rations()==b.frontier.rations());
    assert(a.frontier.salvage()==b.frontier.salvage());
    assert(a.frontier.trust()==b.frontier.trust());
    assert(a.frontier.pump_health()==b.frontier.pump_health());
    assert(a.frontier.morphology_epoch()==b.frontier.morphology_epoch());
    for(int op=3;op<=5;++op) {
        assert(a.frontier.obligation_status(op)==b.frontier.obligation_status(op));
        assert(a.frontier.obligation_due_day(op)==b.frontier.obligation_due_day(op));
    }
    for(int i=0;i<kFrontierSectors;++i) {
        const FrontierSector &x=*a.frontier.sector(i);
        const FrontierSector &y=*b.frontier.sector(i);
        assert(x.ecology==y.ecology && x.previous==y.previous);
        assert(x.owner==y.owner && x.anomaly==y.anomaly);
        assert(x.stability==y.stability && x.residents==y.residents);
        assert(x.food==y.food && x.water==y.water);
        assert(x.wreck_mass==y.wreck_mass && x.claims==y.claims);
    }
}

static void CompareDailyCheckpoint(int pump_health,bool due_accord) {
    Session uninterrupted=StartingCampaign(pump_health,due_accord);
    CampaignSave::Bytes bytes{};
    assert(CampaignSave::Encode(uninterrupted,&bytes));
    Session reloaded(999);
    assert(CampaignSave::Decode(&reloaded,bytes.data(),bytes.size()));
    AssertSectorParity(uninterrupted,reloaded);

    const int deadline=due_accord
        ?uninterrupted.frontier.obligation_due_day(3):-1;
    bool settled=false;
    for(int step=0;step<12;++step) {
        const int day=uninterrupted.frontier.day();
        const uint16_t prior_pump=uninterrupted.frontier.sector(10)->water;
        const int prior_salvage=uninterrupted.frontier.salvage();
        const uint16_t prior_wreck=uninterrupted.frontier.sector(9)->wreck_mass;
        assert(uninterrupted.frontier.AdvanceDay());
        assert(reloaded.frontier.AdvanceDay());
        assert(uninterrupted.frontier.day()==day+1);

        // The starting pump's physical output is 6/3/0 on the first
        // campaign day. A destroyed pump cannot produce or transfer water.
        if(step==0 && !due_accord) {
            const uint16_t expected=static_cast<uint16_t>(
                53+(pump_health==18?6:pump_health==7?3:0));
            assert(uninterrupted.frontier.sector(10)->water==expected);
        }
        if(pump_health==0)
            assert(uninterrupted.frontier.sector(10)->water==prior_pump);

        if(due_accord) {
            if(day+1<deadline) {
                assert(uninterrupted.frontier.obligation_status(3)==Obligations::Due);
            } else {
                assert(uninterrupted.frontier.obligation_status(3)==Obligations::Honored);
                if(!settled) {
                    assert(day+1==deadline);
                    assert(uninterrupted.frontier.salvage()==prior_salvage-2);
                    assert(uninterrupted.frontier.sector(9)->wreck_mass==prior_wreck+2);
                    settled=true;
                } else {
                    assert(uninterrupted.frontier.salvage()==prior_salvage);
                    assert(uninterrupted.frontier.sector(9)->wreck_mass==prior_wreck);
                }
            }
        }
        AssertSectorParity(uninterrupted,reloaded);

        // Every day is serialized, decoded and re-encoded before the next
        // actual AdvanceDay. A reload must neither add a free production
        // tick nor replay a settled treaty payment.
        assert(CampaignSave::Encode(reloaded,&bytes));
        CampaignSave::Bytes direct{};
        assert(CampaignSave::Encode(uninterrupted,&direct));
        assert(bytes==direct);
        const int saved_day=reloaded.frontier.day();
        assert(CampaignSave::Decode(&reloaded,bytes.data(),bytes.size()));
        assert(reloaded.frontier.day()==saved_day);
        AssertSectorParity(uninterrupted,reloaded);
        CampaignSave::Bytes stable{};
        assert(CampaignSave::Encode(reloaded,&stable));
        assert(stable==direct);
    }
    if(due_accord)assert(settled);
}

int main() {
    CompareDailyCheckpoint(18,false);
    CompareDailyCheckpoint(7,false);
    CompareDailyCheckpoint(0,false);
    CompareDailyCheckpoint(18,true);
    std::cout << "Omega Front daily healthy/damaged/destroyed pump "
                 "and dated settlement checkpoints passed\n";
}
