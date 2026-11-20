#include "omega_front_campaign_save.h"
#include <cassert>
#include <iostream>

using namespace reverie::omega;

static void Tap(Session &session) {
    session.Input({0,0,0,false,false,-1,-1});
    session.Input({0,0,0,true,false,-1,-1});
}

// Drive the original opening mission through normal campaign inputs and
// real defender orders. No synthetic victory or resource edits are allowed.
static void WinFirstBattle(Session &session) {
    session.Input({0,0,0,false,false,2,2});
    Tap(session); // Travel to Lantern Pump.
    assert(session.frontier.location()==10);
    Tap(session); // Pay supplies and enter the tactical encounter.
    assert(session.view==FrontMode::Tactical && session.frontier.deployed());
    for(int turn=0;turn<160 &&
            session.tactical.battle.result()==Result::Active;++turn) {
        Battle &battle=session.tactical.battle;
        for(int defender=0;defender<4;++defender) {
            const Unit *unit=battle.unit(defender);
            if(!unit || !unit->alive() || battle.queued(defender))continue;
            for(int target=4;target<battle.unit_count();++target) {
                const Unit *enemy=battle.unit(target);
                if(enemy && enemy->alive() &&
                   battle.QueueAttack(defender,target))break;
            }
        }
        assert(session.Input({0,0,0,false,false,-1,-1,true}));
        session.Input({0,0,0,false,false,-1,-1,false});
    }
    assert(session.tactical.battle.result()==Result::Defended);
    assert(session.frontier.current_battle());
    assert(session.frontier.current_battle()->Checksum()==
           session.tactical.battle.Checksum());
}

int main() {
    Session uninterrupted(414);
    WinFirstBattle(uninterrupted);
    const int salvage_before=uninterrupted.frontier.salvage();
    const int trust_before=uninterrupted.frontier.trust();
    const uint32_t orders_before=uninterrupted.orders;
    assert(uninterrupted.frontier.operation()==0);
    assert(uninterrupted.frontier.last_report().outcome==Result::Active);

    // A terminal result has not yet been acknowledged. Persist this exact
    // handoff boundary rather than saving only after the reward was paid.
    CampaignSave::Bytes pending{};
    assert(CampaignSave::Encode(uninterrupted,&pending));
    Session resumed(927);
    assert(CampaignSave::Decode(&resumed,pending.data(),pending.size()));
    assert(resumed.Checksum()==uninterrupted.Checksum());
    assert(resumed.view==FrontMode::Tactical && resumed.frontier.deployed());
    assert(resumed.tactical.battle.result()==Result::Defended);
    CampaignSave::Bytes pending_roundtrip{};
    assert(CampaignSave::Encode(resumed,&pending_roundtrip));
    assert(pending_roundtrip==pending);

    // The first real Select consumes each pending after-action result once.
    Tap(uninterrupted);
    Tap(resumed);
    assert(uninterrupted.view==FrontMode::SectorMap);
    assert(!uninterrupted.frontier.deployed());
    assert(uninterrupted.frontier.operation()==1);
    assert(uninterrupted.orders==orders_before+1);
    const FrontierReport report=uninterrupted.frontier.last_report();
    assert(report.operation==0 && report.outcome==Result::Defended);
    assert(report.enemies_lost>0);
    assert(uninterrupted.frontier.salvage()==
           salvage_before+2*report.enemies_lost);
    assert(uninterrupted.frontier.trust()==trust_before+5);
    assert(uninterrupted.frontier.sector(10)->owner==kIndependentOwner);
    assert(resumed.Checksum()==uninterrupted.Checksum());
    assert(resumed.frontier.last_report().enemies_lost==report.enemies_lost);

    CampaignSave::Bytes awarded{},awarded_replay{};
    assert(CampaignSave::Encode(uninterrupted,&awarded));
    assert(CampaignSave::Encode(resumed,&awarded_replay));
    assert(awarded_replay==awarded);
    Session second_reload(42);
    assert(CampaignSave::Decode(&second_reload,awarded.data(),awarded.size()));
    assert(second_reload.Checksum()==uninterrupted.Checksum());
    // Once the battle is finalized, even an explicit duplicate resolve
    // cannot collect loot, change ownership, or advance the operation.
    const uint64_t stable=second_reload.Checksum();
    assert(!second_reload.frontier.ResolveOperation());
    assert(second_reload.Checksum()==stable);
    assert(second_reload.frontier.salvage()==
           salvage_before+2*report.enemies_lost);
    CampaignSave::Bytes stable_bytes{};
    assert(CampaignSave::Encode(second_reload,&stable_bytes));
    assert(stable_bytes==awarded);
    std::cout<<"PASS Omega Front terminal victory checkpoint to one-time campaign reward\n";
}
