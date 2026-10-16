#include "omega_front_campaign_save.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;
static void Reseal(CampaignSave::Bytes &bytes){
    uint64_t h=UINT64_C(14695981039346656037);
    for(size_t i=0;i<bytes.size()-8;++i){h^=bytes[i];h*=UINT64_C(1099511628211);}
    for(size_t i=bytes.size()-8;i<bytes.size();++i){bytes[i]=static_cast<uint8_t>(h);h>>=8;}
}
static void Tap(Session &s,bool primary,bool secondary=false){
    s.Input({0,0,0,false,false,-1,-1});
    s.Input({0,0,0,primary,secondary,-1,-1});
}
int main(){
    Session campaign(414);
    CampaignSave::Bytes bytes{};
    assert(CampaignSave::Encode(campaign,&bytes));
    Session restored(999);
    assert(CampaignSave::Decode(&restored,bytes.data(),bytes.size()));
    assert(restored.Checksum()==campaign.Checksum());
    // Make the day-four morphology change and persist real accumulated stores.
    campaign.Input({0,0,0,false,false,2,2});Tap(campaign,true);
    assert(campaign.frontier.day()==2);
    for(int i=0;i<4;++i)Tap(campaign,false,true);
    assert(campaign.action==FieldAction::Wait);
    Tap(campaign,true);Tap(campaign,true);
    assert(campaign.frontier.day()==4 && campaign.frontier.morphology_epoch()==1);
    assert(CampaignSave::Encode(campaign,&bytes));
    assert(CampaignSave::Decode(&restored,bytes.data(),bytes.size()));
    assert(restored.Checksum()==campaign.Checksum());
    assert(restored.frontier.sector(8)->food==0);
    // Move into a real operation, issue a tactical move while paused,
    // persist fractional logical turns, and continue deterministically.
    for(int i=0;i<7;++i)Tap(campaign,false,true);
    assert(campaign.action==FieldAction::Wait); // full cycle
    for(int i=0;i<3;++i)Tap(campaign,false,true);
    assert(campaign.action==FieldAction::Deploy);
    Tap(campaign,true);
    assert(campaign.view==FrontMode::Tactical);
    campaign.Input({0,0,0,false,false,3,11});Tap(campaign,true);
    assert(campaign.tactical.selected==0);
    campaign.Input({0,0,0,false,false,6,11});Tap(campaign,true);
    assert(campaign.tactical.battle.queued(0)==1);
    campaign.Input({.10f,0,0,false,false,-1,-1});
    assert(CampaignSave::Encode(campaign,&bytes));
    assert(CampaignSave::Decode(&restored,bytes.data(),bytes.size()));
    assert(restored.Checksum()==campaign.Checksum());
    for(int i=0;i<40;++i){
        ControlsInput in{.05f,0,0,false,false,-1,-1};
        campaign.Input(in);restored.Input(in);
        assert(campaign.Checksum()==restored.Checksum());
    }
    // A completed battle has one stored after-action consequence; resume
    // in map mode must not repeat casualties, claims, or salvage awards.
    Session after(414);
    after.Input({0,0,0,false,false,2,2});Tap(after,true);Tap(after,true);
    for(int i=0;i<160 && after.tactical.battle.result()==Result::Active;++i)
        after.Input({.1f,0,0,false,false,-1,-1});
    assert(after.tactical.battle.result()!=Result::Active);
    Tap(after,true);
    assert(after.view==FrontMode::SectorMap);
    const auto after_hash=after.Checksum();
    assert(CampaignSave::Encode(after,&bytes));
    Session after_load(777);
    assert(CampaignSave::Decode(&after_load,bytes.data(),bytes.size()));
    assert(after_load.Checksum()==after_hash);
    assert(after_load.frontier.last_report().outcome==after.frontier.last_report().outcome);
    assert(after_load.frontier.salvage()==after.frontier.salvage());
    // Use the in-battle checkpoint for tamper cases below.
    assert(CampaignSave::Encode(restored,&bytes));
    // Damaged save never overwrites an already valid campaign.
    const uint64_t before=restored.Checksum();
    auto damage=bytes;damage[75]^=16;
    assert(!CampaignSave::Decode(&restored,damage.data(),damage.size()));
    assert(restored.Checksum()==before);
    assert(!CampaignSave::Decode(&restored,bytes.data(),bytes.size()-1));
    assert(restored.Checksum()==before);
    // A forged but correctly resealed record still fails schema validation.
    damage=bytes;damage[8]=7;Reseal(damage); // incompatible version
    assert(!CampaignSave::Decode(&restored,damage.data(),damage.size()));
    assert(restored.Checksum()==before);
    damage=bytes;damage[16]=99;Reseal(damage); // sector cursor beyond 4x4
    assert(!CampaignSave::Decode(&restored,damage.data(),damage.size()));
    assert(restored.Checksum()==before);
    assert(!CampaignSave::Encode(restored,nullptr));
    assert(!CampaignSave::Decode(nullptr,bytes.data(),bytes.size()));
    std::cout<<"Omega Front strategic campaign save/replay/corruption regressions passed\n";
}
