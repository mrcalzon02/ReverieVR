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
int main(){
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
    assert(bytes[8]==9); // v9 retains grandfathered border retaliation history
    Session restored(999);
    assert(CampaignSave::Decode(&restored,bytes.data(),bytes.size()));
    assert(restored.Checksum()==campaign.Checksum());
    assert(restored.action==FieldAction::Parley);
    assert(restored.frontier.parleyed(7));
    Tap(restored,true);
    assert(restored.rejected && restored.frontier.Checksum()==after_parley);
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
