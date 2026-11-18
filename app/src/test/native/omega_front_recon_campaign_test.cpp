#include "omega_front_campaign_save.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;
static void Tap(Session &s,bool primary,bool secondary=false){
    s.Input({0,0,0,false,false,-1,-1});
    s.Input({0,0,0,primary,secondary,-1,-1});
}
static void Reseal(CampaignSave::Bytes &bytes){
    uint64_t h=UINT64_C(14695981039346656037);
    for(size_t i=0;i<bytes.size()-8;++i){h^=bytes[i];h*=UINT64_C(1099511628211);}
    for(size_t i=bytes.size()-8;i<bytes.size();++i){bytes[i]=static_cast<uint8_t>(h);h>>=8;}
}
// Rejection of a diplomatic action before surveying a visible neighbor must
// never bill supplies, advance the day, or reveal hidden campaign knowledge.
// Reload must preserve the pending action without replaying the rejected bill.
static void RejectedBeforeSurveyParleyCheckpoint(){
    Session blocked(414);
    blocked.Input({0,0,0,false,false,3,1}); // visible, unsurveyed Charter Gate
    assert(blocked.focused==7 && blocked.frontier.location()==6);
    assert(blocked.frontier.intelligence(7)==Intelligence::Terrain);
    for(int i=0;i<6;++i)Tap(blocked,false,true);
    assert(blocked.action==FieldAction::Parley);
    const uint64_t pristine=blocked.frontier.Checksum();
    const uint32_t orders=blocked.orders;
    const int rations=blocked.frontier.rations();
    const int salvage=blocked.frontier.salvage();
    const int day=blocked.frontier.day();
    const int trust=blocked.frontier.trust();
    const int relation=blocked.frontier.relation(2);
    const int claims=blocked.frontier.sector(7)->claims;
    const int wreck=blocked.frontier.sector(7)->wreck_mass;
    Tap(blocked,true);
    assert(blocked.rejected && blocked.orders==orders);
    assert(blocked.frontier.Checksum()==pristine);
    assert(blocked.frontier.rations()==rations && blocked.frontier.salvage()==salvage);
    assert(blocked.frontier.day()==day && blocked.frontier.trust()==trust);
    assert(blocked.frontier.relation(2)==relation);
    assert(blocked.frontier.sector(7)->claims==claims);
    assert(blocked.frontier.sector(7)->wreck_mass==wreck);
    assert(!blocked.frontier.parleyed(7));
    assert(blocked.frontier.intelligence(0)==Intelligence::Unknown);

    CampaignSave::Bytes data{};
    assert(CampaignSave::Encode(blocked,&data));
    Session reload(999);
    assert(CampaignSave::Decode(&reload,data.data(),data.size()));
    assert(!reload.rejected); // rejected is transient UI feedback, not a bill
    assert(reload.action==FieldAction::Parley && reload.focused==7);
    assert(reload.Checksum()==blocked.Checksum());
    Tap(reload,true); // retry is still rejected, including after restart
    assert(reload.rejected && reload.orders==orders);
    assert(reload.frontier.Checksum()==pristine);
    for(int i=0;i<7;++i)Tap(reload,false,true); // Parley -> Scout
    assert(reload.action==FieldAction::Scout);
    Tap(reload,true);
    assert(!reload.rejected && reload.orders==orders+1);
    assert(reload.frontier.intelligence(7)==Intelligence::Surveyed);
    assert(reload.frontier.rations()==rations-2);
    assert(reload.frontier.day()==day+1);
    Tap(reload,false,true); // Scout -> Parley
    assert(reload.action==FieldAction::Parley);
    Tap(reload,true);
    assert(!reload.rejected && reload.orders==orders+2);
    assert(reload.frontier.parleyed(7));
    assert(reload.frontier.rations()==rations-3);
    assert(reload.frontier.salvage()==salvage-4);
    assert(reload.frontier.relation(2)==relation+8);
    assert(reload.frontier.sector(7)->wreck_mass==wreck+4);
    assert(reload.frontier.day()==day+2);
    assert(reload.frontier.operation()==0);
    assert(reload.frontier.intelligence(0)==Intelligence::Unknown);
    assert(CampaignSave::Encode(reload,&data));
    Session final(1);
    assert(CampaignSave::Decode(&final,data.data(),data.size()));
    assert(final.Checksum()==reload.Checksum());
    std::cout << "Omega Front rejected pre-survey parley save/reload passed\n";
}
int main(){
    RejectedBeforeSurveyParleyCheckpoint();
    Session campaign(414);
    assert(campaign.frontier.intelligence(6)==Intelligence::Surveyed);
    assert(campaign.frontier.intelligence(7)==Intelligence::Terrain);
    assert(campaign.frontier.intelligence(0)==Intelligence::Unknown);
    const int original_rations=campaign.frontier.rations();
    campaign.Input({0,0,0,false,false,3,1}); // Charter Gate S008
    assert(campaign.focused==7 && campaign.frontier.location()==6);
    for(int i=0;i<5;++i)Tap(campaign,false,true);
    assert(campaign.action==FieldAction::Scout);
    Tap(campaign,true);
    assert(!campaign.rejected);
    assert(campaign.frontier.intelligence(7)==Intelligence::Surveyed);
    assert(campaign.frontier.rations()==original_rations-2);
    assert(campaign.frontier.day()==2);
    const auto after_scout=campaign.frontier.Checksum();
    Tap(campaign,true);
    assert(campaign.rejected && campaign.frontier.Checksum()==after_scout);
    Tap(campaign,false,true);
    assert(campaign.action==FieldAction::Parley);
    const auto previous_relation=campaign.frontier.relation(2);
    const auto previous_claims=campaign.frontier.sector(7)->claims;
    const auto previous_wreck=campaign.frontier.sector(7)->wreck_mass;
    const auto previous_salvage=campaign.frontier.salvage();
    Tap(campaign,true);
    assert(!campaign.rejected);
    assert(campaign.frontier.parleyed(7));
    assert(campaign.frontier.relation(2)==previous_relation+8);
    assert(campaign.frontier.sector(7)->claims==previous_claims-6);
    assert(campaign.frontier.sector(7)->wreck_mass==previous_wreck+4);
    assert(campaign.frontier.salvage()==previous_salvage-4);
    assert(campaign.frontier.rations()==original_rations-3);
    assert(campaign.frontier.sector(7)->owner==2);
    assert(campaign.frontier.operation()==0); // no fake battle win
    assert(campaign.frontier.day()==3);
    const auto after_parley=campaign.frontier.Checksum();
    Tap(campaign,true);
    assert(campaign.rejected && campaign.frontier.Checksum()==after_parley);
    CampaignSave::Bytes bytes{};
    assert(CampaignSave::Encode(campaign,&bytes));
    assert(bytes[8]==10); // includes grandfathered border retaliation history
    Session restored(999);
    assert(CampaignSave::Decode(&restored,bytes.data(),bytes.size()));
    assert(restored.Checksum()==campaign.Checksum());
    assert(restored.action==FieldAction::Parley);
    assert(restored.frontier.parleyed(7));
    Tap(restored,true);
    assert(restored.rejected && restored.frontier.Checksum()==after_parley);
    // Save/reload preserves observed terrain, paid surveys and diplomatic
    // history separately. Merely scouting S008 must not expose its neighbors.
    assert(campaign.frontier.intelligence(3)==Intelligence::Unknown);
    assert(campaign.frontier.intelligence(11)==Intelligence::Unknown);
    assert(campaign.frontier.intelligence(0)==Intelligence::Unknown);
    for(int sector=0;sector<kFrontierSectors;++sector){
        assert(restored.frontier.intelligence(sector)==campaign.frontier.intelligence(sector));
        assert(restored.frontier.parleyed(sector)==campaign.frontier.parleyed(sector));
    }
    assert(restored.frontier.intelligence(5)==Intelligence::Terrain);
    assert(restored.frontier.intelligence(7)==Intelligence::Surveyed);
    const auto paid_survey=restored.frontier.Checksum();
    assert(!restored.frontier.Scout(7)); // no free second survey on reload
    assert(restored.frontier.Checksum()==paid_survey);
    // Only a real march to Charter Gate reveals adjacent Old Relay/S012
    // terrain; distant Chalkwatch and the Rust Orchard remain unobserved.
    Session marched(999);
    assert(CampaignSave::Decode(&marched,bytes.data(),bytes.size()));
    assert(marched.frontier.Travel(7));
    assert(marched.frontier.intelligence(3)==Intelligence::Terrain);
    assert(marched.frontier.intelligence(11)==Intelligence::Terrain);
    assert(marched.frontier.intelligence(0)==Intelligence::Unknown);
    assert(marched.frontier.intelligence(8)==Intelligence::Unknown);
    assert(marched.frontier.intelligence(7)==Intelligence::Surveyed);
    assert(marched.frontier.parleyed(7) && !marched.frontier.parleyed(3));
    CampaignSave::Bytes marched_bytes{};
    assert(CampaignSave::Encode(marched,&marched_bytes));
    Session marched_reload(123);
    assert(CampaignSave::Decode(&marched_reload,marched_bytes.data(),marched_bytes.size()));
    assert(marched_reload.Checksum()==marched.Checksum());
    for(int sector=0;sector<kFrontierSectors;++sector)
        assert(marched_reload.frontier.intelligence(sector)==marched.frontier.intelligence(sector));
    // Existing schema 2 imports safely with no fabricated scouting history.
    Session old(414);
    assert(CampaignSave::Encode(old,&bytes));
    const size_t diplomacy_end=55+16*14+Checkpoint::kBytes+kMajorFactions;
    for(size_t i=diplomacy_end;i<diplomacy_end+32;++i)bytes[i]=0;
    bytes[8]=2;Reseal(bytes);
    assert(CampaignSave::Decode(&restored,bytes.data(),bytes.size()));
    assert(restored.Checksum()==old.Checksum());
    assert(restored.frontier.intelligence(6)==Intelligence::Surveyed);
    assert(restored.frontier.intelligence(7)==Intelligence::Terrain);
    // A properly resealed but illegal scout history must be rejected atomically.
    assert(CampaignSave::Encode(campaign,&bytes));
    auto invalid=bytes;
    invalid[diplomacy_end+7]=3;Reseal(invalid);
    assert(!CampaignSave::Decode(&restored,invalid.data(),invalid.size()));
    assert(restored.Checksum()==old.Checksum());
    invalid=bytes;
    invalid[diplomacy_end+16+1]=1;Reseal(invalid); // unscouted treaty
    assert(!CampaignSave::Decode(&restored,invalid.data(),invalid.size()));
    assert(restored.Checksum()==old.Checksum());
    std::cout << "Omega Front linked scouting, parley and v3 save regressions passed\n";
}
