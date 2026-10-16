#include "omega_front_campaign_save.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
using namespace reverie::omega;

static void Reseal(CampaignSave::Bytes &data){
    uint64_t hash=UINT64_C(14695981039346656037);
    for(size_t i=0;i<data.size()-8;++i){hash^=data[i];hash*=UINT64_C(1099511628211);}
    for(size_t i=data.size()-8;i<data.size();++i){data[i]=static_cast<uint8_t>(hash);hash>>=8;}
}
// Synthetic *test-only* post-arc position. It is decoded by the real
// production validator and still uses all production deployment/resource and
// after-action paths. It is NOT evidence of winning the prior three missions.
static Session AtOperation(int op){
    Session first(414);
    CampaignSave::Bytes snapshot{};
    assert(CampaignSave::Encode(first,&snapshot));
    snapshot[28]=static_cast<uint8_t>(op); // schema v2 operation field
    Reseal(snapshot);
    Session resume(9);
    assert(CampaignSave::Decode(&resume,snapshot.data(),snapshot.size()));
    return resume;
}
static void FireRealOrders(Battle &battle){
    for(int id=0;id<4;++id){
        const Unit *u=battle.unit(id);
        if(!u||!u->alive()||battle.queued(id))continue;
        bool ordered=false;
        for(int target=4;target<battle.unit_count();++target){
            const Unit *enemy=battle.unit(target);
            if(enemy&&enemy->alive() && battle.QueueAttack(id,target)){
                ordered=true;break;
            }
        }
        if(!ordered)battle.QueueMove(id,8,11+(id%3));
    }
}
int main(){
    // The original first three missions remain in exactly the same order.
    const int sectors[]={10,5,10,9,7,4};
    const uint8_t factions[]={0,1,1,4,2,6};
    const Mission missions[]={Mission::HoldPump,Mission::CaptureOrchard,
        Mission::DisableRelay,Mission::HoldPump,Mission::CaptureOrchard,Mission::CaptureOrchard};
    for(int op=0;op<kHearthfallOperations;++op){
        assert(Frontier::TargetForOperation(op)==sectors[op]);
        assert(Frontier::FactionForOperation(op)==factions[op]);
        assert(Frontier::MissionForOperation(op)==missions[op]);
        Battle one(1),two(1);
        assert(Frontier::BuildBattle(op,900+op,13,&one));
        assert(Frontier::BuildBattle(op,900+op,13,&two));
        assert(one.Checksum()==two.Checksum());
        assert(one.pump_hp()==13);
        assert(one.mission()==missions[op]);
        assert(static_cast<uint8_t>(one.doctrine())==factions[op]);
        for(int i=0;i<6;++i){one.Step();two.Step();assert(one.Checksum()==two.Checksum());}
    }
    Battle untouched(55);
    const auto hash=untouched.Checksum();
    assert(!Frontier::BuildBattle(-1,1,18,&untouched));
    assert(!Frontier::BuildBattle(6,1,18,&untouched));
    assert(!Frontier::BuildBattle(3,1,0,&untouched));
    assert(!Frontier::BuildBattle(3,1,18,nullptr));
    assert(untouched.Checksum()==hash);
    assert(Frontier::TargetForOperation(6)==-1);

    // Starting at an actual post-opening campaign snapshot, the player must
    // STILL march to S010 and spend supplies to deploy Scrapline combat.
    Session scrap=AtOperation(3);
    auto &f=scrap.frontier;
    assert(f.target_sector()==9 && f.operation_faction()==4);
    assert(f.relation(4)==-10);
    assert(!f.StartOperation()); // not at Ashrail Yard
    assert(f.Travel(10) && f.Travel(9));
    assert(f.StartOperation());
    assert(f.current_battle()->doctrine()==Doctrine::Scrapline);
    assert(f.current_battle()->mission()==Mission::HoldPump);
    assert(!f.ResolveOperation());
    const int yard_before=f.sector(9)->wreck_mass;
    const int metal_before=f.salvage();
    for(int t=0;t<160 && f.current_battle()->result()==Result::Active;++t){
        FireRealOrders(*f.current_battle());
        f.current_battle()->Step();
    }
    assert(f.current_battle()->result()==Result::Defended);
    assert(f.ResolveOperation());
    assert(!f.ResolveOperation());
    assert(f.operation()==4);
    assert(f.relation(4)==-22);
    assert(f.sector(9)->owner==4); // The yard is not magically annexed.
    assert(f.sector(9)->wreck_mass==yard_before-18);
    assert(f.salvage()==metal_before+18+2*f.last_report().enemies_lost);
    assert(f.last_report().operation==3);
    assert(f.target_sector()==7);

    // Exact long-term relation, claim, material and mission progress survive
    // a genuine v2 save round trip. No fake replay award occurs on reload.
    CampaignSave::Bytes snapshot{};
    assert(CampaignSave::Encode(scrap,&snapshot));
    assert(snapshot[8]==3);
    Session restored(100);
    assert(CampaignSave::Decode(&restored,snapshot.data(),snapshot.size()));
    assert(restored.Checksum()==scrap.Checksum());
    assert(restored.frontier.relation(4)==-22);
    assert(!restored.frontier.ResolveOperation());

    // A travel sequence across real graph edges reaches Meridian's gate.
    assert(restored.frontier.Travel(5));
    assert(restored.frontier.Travel(6));
    assert(restored.frontier.Travel(7));
    assert(restored.frontier.operation()==4);
    assert(restored.frontier.StartOperation());
    assert(restored.frontier.current_battle()->doctrine()==Doctrine::Meridian);
    assert(restored.frontier.current_battle()->mission()==Mission::CaptureOrchard);
    assert(restored.frontier.current_battle()->unit(4)->role==Class::Sentinel);
    assert(restored.frontier.current_battle()->objective_progress()==0);
    // No issuing orders is a genuine loss of the capture objective, rather
    // than an invented success that grants territory or diplomacy.
    for(int i=0;i<160 && restored.frontier.current_battle()->result()==Result::Active;++i)
        restored.frontier.current_battle()->Step();
    assert(restored.frontier.current_battle()->result()==Result::Overrun);
    const int before_claim=restored.frontier.sector(7)->claims;
    assert(restored.frontier.ResolveOperation());
    assert(restored.frontier.operation()==4);
    assert(restored.frontier.sector(7)->owner==2);
    assert(restored.frontier.sector(7)->claims>=before_claim);
    assert(restored.frontier.relation(2)==-34);

    // F06 is not a Meridian palette variant; the Lost Ninth has a physical
    // shield formation and contested ford win condition, not a free victory.
    Session roman=AtOperation(5);
    assert(roman.frontier.Travel(5));
    assert(roman.frontier.Travel(4));
    assert(roman.frontier.StartOperation());
    Battle *ford=roman.frontier.current_battle();
    assert(ford && ford->doctrine()==Doctrine::LostNinth);
    assert(ford->mission()==Mission::CaptureOrchard);
    assert(ford->unit(4)->role==Class::Legionary);
    assert(ford->unit(4)->ammo==0);
    assert(roman.frontier.sector(1)->owner==6); // Ninth Camp untouched.
    const int roman_relation=roman.frontier.relation(6);
    for(int i=0;i<160 && ford->result()==Result::Active;++i)ford->Step();
    assert(ford->result()==Result::Overrun);
    assert(roman.frontier.ResolveOperation());
    assert(roman.frontier.operation()==5);
    assert(roman.frontier.sector(4)->owner==0); // defeat is not conquest
    assert(roman.frontier.relation(6)==roman_relation-4);

    // A valid prior v1 campaign file (no relations/intelligence fields) must load with the
    // newly established initial faction stances. Schema-2 records include
    // 17 signed relation fields after the nested tactical checkpoint. Legacy
    // v1 also contains no reconnaissance/treaty arrays.
    Session old(414);assert(CampaignSave::Encode(old,&snapshot));
    const size_t relations_offset=55+16*14+Checkpoint::kBytes;
    for(size_t i=relations_offset;i<relations_offset+kMajorFactions+32;++i)snapshot[i]=0;
    snapshot[8]=1;Reseal(snapshot);
    Session migrated(99);
    assert(CampaignSave::Decode(&migrated,snapshot.data(),snapshot.size()));
    assert(migrated.Checksum()==old.Checksum());
    // And a v2 relation outside its legal [-100,100] range is rejected even
    // when somebody reseals the file. No partial live state mutation.
    assert(CampaignSave::Encode(restored,&snapshot));
    const uint64_t stable=restored.Checksum();
    snapshot[relations_offset+3]=101;
    Reseal(snapshot);
    assert(!CampaignSave::Decode(&restored,snapshot.data(),snapshot.size()));
    assert(restored.Checksum()==stable);
    snapshot[28]=7;Reseal(snapshot);
    assert(!CampaignSave::Decode(&restored,snapshot.data(),snapshot.size()));
    std::cout<<"Omega Front six-operation doctrine/ownership/diplomacy replay regressions passed\n";
}
