#include "omega_front_campaign_save.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;

static void Reseal(Checkpoint::Bytes &data){
    uint64_t h=UINT64_C(14695981039346656037);
    for(size_t i=0;i<data.size()-8;++i){h^=data[i];h*=UINT64_C(1099511628211);}
    for(size_t i=data.size()-8;i<data.size();++i){data[i]=static_cast<uint8_t>(h);h>>=8;}
}
int main(){
    Battle defense(42);
    assert(defense.mission()==Mission::HoldPump);
    assert(defense.objective_goal()==0 && defense.objective_x()==2);
    defense.Step();
    assert(!defense.ConfigureMission(Mission::CaptureOrchard));
    Battle orchard(22);
    orchard.ClearUnits();
    assert(orchard.ConfigureMission(Mission::CaptureOrchard));
    assert(orchard.AddUnit(Side::Hearthwell,Class::Rifle,17,12)==0);
    assert(orchard.AddUnit(Side::Raiders,Class::Rifle,23,23)==1);
    for(int t=0;t<2;++t){orchard.Step();assert(orchard.result()==Result::Active);}
    assert(orchard.objective_progress()==2);
    orchard.Step();
    assert(orchard.result()==Result::Defended && orchard.objective_progress()==3);
    assert(orchard.unit(1)->alive()); // Capture, not extermination
    assert(!orchard.ConfigureMission(Mission::HoldPump));
    const uint64_t captureHash=orchard.Checksum();
    orchard.Step();assert(orchard.Checksum()==captureHash);
    Battle contested(16);
    contested.ClearUnits();
    assert(contested.ConfigureMission(Mission::CaptureOrchard));
    contested.AddUnit(Side::Hearthwell,Class::Heavy,17,12);
    contested.AddUnit(Side::Raiders,Class::Rifle,18,12);
    contested.Step();
    assert(contested.objective_progress()==0);
    Battle noCapture(13);
    noCapture.ClearUnits();
    assert(noCapture.ConfigureMission(Mission::CaptureOrchard));
    noCapture.AddUnit(Side::Hearthwell,Class::Heavy,3,12);
    noCapture.AddUnit(Side::Raiders,Class::Engineer,4,12);
    assert(noCapture.QueueAttack(0,1));
    for(int t=0;t<12 && noCapture.unit(1)->alive();++t)noCapture.Step();
    assert(!noCapture.unit(1)->alive());
    assert(noCapture.result()==Result::Active && noCapture.objective_progress()==0);
    assert(noCapture.QueueMove(0,17,12));
    for(int t=0;t<80 && noCapture.result()==Result::Active;++t)noCapture.Step();
    assert(noCapture.result()==Result::Defended);
    Battle relay(31);
    relay.ClearUnits();
    assert(relay.ConfigureMission(Mission::DisableRelay));
    relay.AddUnit(Side::Hearthwell,Class::Engineer,17,12);
    relay.AddUnit(Side::Raiders,Class::Rifle,23,23);
    const int charges=relay.components();
    relay.Step();
    assert(relay.result()==Result::Active && relay.objective_progress()==1);
    assert(relay.components()==charges-1);
    relay.Step();
    assert(relay.result()==Result::Defended && relay.objective_progress()==2);
    assert(relay.components()==charges-2 && relay.unit(1)->alive());
    Battle blocked(31);
    blocked.ClearUnits();
    assert(blocked.ConfigureMission(Mission::DisableRelay));
    blocked.AddUnit(Side::Hearthwell,Class::Engineer,17,12);
    blocked.AddUnit(Side::Raiders,Class::Engineer,19,12);
    blocked.Step();
    assert(blocked.objective_progress()==0 && blocked.components()==4);
    // The real tactical objective, progress and charged resources survive
    // checkpoint replay, including before the final sabotage pulse.
    Controller saving(333);
    saving.battle=Battle(31);
    saving.battle.ClearUnits();
    assert(saving.battle.ConfigureMission(Mission::DisableRelay));
    saving.battle.AddUnit(Side::Hearthwell,Class::Engineer,17,12);
    saving.battle.AddUnit(Side::Raiders,Class::Rifle,23,23);
    saving.battle.Step();
    Checkpoint::Bytes slot{};
    assert(Checkpoint::Encode(saving,&slot));
    assert(slot[8]==3 && slot[9]==0); // v3 schema includes doctrine
    Controller loaded(23);
    assert(Checkpoint::Decode(&loaded,slot.data(),slot.size()));
    assert(loaded.battle.mission()==Mission::DisableRelay);
    assert(loaded.battle.objective_progress()==1);
    assert(loaded.battle.Checksum()==saving.battle.Checksum());
    loaded.battle.Step();saving.battle.Step();
    assert(loaded.battle.Checksum()==saving.battle.Checksum());
    assert(loaded.battle.result()==Result::Defended);
    auto corrupt=slot;corrupt[1075]=9;Reseal(corrupt);
    assert(!Checkpoint::Decode(&loaded,corrupt.data(),corrupt.size()));
    assert(loaded.battle.result()==Result::Defended);
    // Previous v1 combat saves decode as their original pump-defense mode.
    Controller old(17);
    assert(Checkpoint::Encode(old,&slot));
    slot[8]=1;slot[9]=0;
    slot[1075]=0;slot[1076]=0;Reseal(slot);
    assert(Checkpoint::Decode(&loaded,slot.data(),slot.size()));
    assert(loaded.battle.mission()==Mission::HoldPump);
    assert(loaded.battle.Checksum()==old.battle.Checksum());
    // All three authored operation builders select the corresponding rules.
    Frontier f(414);
    assert(f.Travel(10));assert(f.StartOperation());
    assert(f.current_battle()->mission()==Mission::HoldPump);
    assert(f.current_battle()->objective_goal()==0);
    // All existing campaign saves remain 2048 bytes with a nested v3 combat
    // payload; no silent strategic-progress edits or duplicate reward.
    Session session(414);
    CampaignSave::Bytes save{};
    assert(CampaignSave::Encode(session,&save));
    Session restored(1);
    assert(CampaignSave::Decode(&restored,save.data(),save.size()));
    assert(restored.Checksum()==session.Checksum());
    // A separately resealed v3 nested objective must not impersonate a
    // different strategic operation, even under a valid outer checksum.
    Session stage(414);
    assert(stage.frontier.Travel(10));
    assert(stage.frontier.StartOperation());
    stage.focused=10;stage.view=FrontMode::Tactical;
    stage.tactical.battle=*stage.frontier.current_battle();
    assert(CampaignSave::Encode(stage,&save));
    size_t nested=0;
    const char header[]="OFHBEL01";
    for(size_t i=0;i+8<save.size();++i){
        if(std::memcmp(save.data()+i,header,8)==0){nested=i;break;}
    }
    assert(nested>0 && nested+Checkpoint::kBytes<=save.size());
    Checkpoint::Bytes battleBytes{};
    std::memcpy(battleBytes.data(),save.data()+nested,battleBytes.size());
    assert(battleBytes[8]==3 && battleBytes[1075]==0);
    battleBytes[1075]=1;Reseal(battleBytes);
    std::memcpy(save.data()+nested,battleBytes.data(),battleBytes.size());
    uint64_t outer=UINT64_C(14695981039346656037);
    for(size_t i=0;i<save.size()-8;++i){outer^=save[i];outer*=UINT64_C(1099511628211);}
    for(size_t i=save.size()-8;i<save.size();++i){save[i]=static_cast<uint8_t>(outer);outer>>=8;}
    const uint64_t before=restored.Checksum();
    assert(!CampaignSave::Decode(&restored,save.data(),save.size()));
    assert(restored.Checksum()==before);
    std::cout<<"Omega Front distinctive objectives and v1 checkpoint migration passed\n";
}
