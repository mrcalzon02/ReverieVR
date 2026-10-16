#include "omega_front_campaign_save.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;
static void Tap(Session &s,bool primary=false,bool secondary=false){
    s.Input({0,0,0,false,false,-1,-1});
    s.Input({0,0,0,primary,secondary,-1,-1});
}
static void Reseal(CampaignSave::Bytes &b){
    uint64_t h=UINT64_C(14695981039346656037);
    for(size_t i=0;i<b.size()-8;++i){h^=b[i];h*=UINT64_C(1099511628211);}
    for(size_t i=b.size()-8;i<b.size();++i){b[i]=static_cast<uint8_t>(h);h>>=8;}
}
// Test-fixture-only legal operation progress, not an invented battle win.
static Session AtOperation(int op){
    Session initial(414); CampaignSave::Bytes saved{};
    assert(CampaignSave::Encode(initial,&saved));
    saved[28]=static_cast<uint8_t>(op);Reseal(saved);
    Session result(9);
    assert(CampaignSave::Decode(&result,saved.data(),saved.size()));
    return result;
}
int main(){
    Session scrap=AtOperation(3);
    assert(scrap.frontier.Travel(10));
    const int initialPump=scrap.frontier.pump_health();
    const FrontierReport before=scrap.frontier.last_report();
    assert(!scrap.frontier.ConcludeAccord(9));
    assert(scrap.frontier.Scout(9));
    assert(!scrap.frontier.ConcludeAccord(9));
    assert(scrap.frontier.Negotiate(9));
    const int rations=scrap.frontier.rations(),salvage=scrap.frontier.salvage();
    const uint16_t wreck=scrap.frontier.sector(9)->wreck_mass;
    assert(scrap.frontier.ConcludeAccord(9));
    assert(scrap.frontier.operation()==4);
    assert(scrap.frontier.accords()==8 && scrap.frontier.last_accord()==3);
    assert(scrap.frontier.rations()==rations-2);
    assert(scrap.frontier.salvage()==salvage+6);
    assert(scrap.frontier.sector(9)->wreck_mass==wreck-6);
    assert(scrap.frontier.sector(9)->owner==4);
    assert(scrap.frontier.guards()==12 && scrap.frontier.pump_health()==initialPump);
    assert(scrap.frontier.last_report().operation==before.operation);
    assert(scrap.frontier.last_report().outcome==before.outcome);
    assert(!scrap.frontier.ConcludeAccord(9));
    CampaignSave::Bytes bytes{};
    assert(CampaignSave::Encode(scrap,&bytes) && bytes[8]==5);
    Session loaded(8);
    assert(CampaignSave::Decode(&loaded,bytes.data(),bytes.size()));
    assert(loaded.Checksum()==scrap.Checksum());
    assert(loaded.frontier.accords()==8);

    Session meridian=AtOperation(4);
    assert(meridian.frontier.Scout(7));
    assert(meridian.frontier.Negotiate(7));
    // Prove the actual eight-position Daydream action cycle reaches Accord.
    meridian.Input({0,0,0,false,false,3,1});
    for(int i=0;i<7;++i)Tap(meridian,false,true);
    assert(meridian.action==FieldAction::Accord);
    const auto initialOwner=meridian.frontier.sector(7)->owner;
    Tap(meridian,true);
    assert(!meridian.rejected && meridian.frontier.operation()==5);
    assert(meridian.frontier.sector(7)->owner==initialOwner && initialOwner==2);
    assert(meridian.frontier.last_report().operation==-1); // no fake combat
    assert(meridian.frontier.accords()==16);

    Session legion=AtOperation(5);
    assert(legion.frontier.Travel(5));
    assert(legion.frontier.Scout(4));
    assert(legion.frontier.Negotiate(4)); // F06 controls neutral ford militarily
    assert(legion.frontier.relation(6)==3);
    assert(legion.frontier.ConcludeAccord(4));
    assert(legion.frontier.operation()==kHearthfallOperations);
    assert(legion.frontier.sector(4)->owner==0);
    assert(legion.frontier.sector(1)->owner==6);
    assert(legion.frontier.accords()==32);

    // Combat remains available after a parley if the commander marches in.
    Session assault=AtOperation(4);
    assert(assault.frontier.Scout(7) && assault.frontier.Negotiate(7));
    assert(assault.frontier.Travel(7));
    assert(assault.frontier.StartOperation());
    assert(assault.frontier.current_battle()->doctrine()==Doctrine::Meridian);
    assert(assault.frontier.accords()==0);

    // Restored treaties must retain prerequisites and schema consistency.
    const size_t intel=55+16*14+Checkpoint::kBytes+kMajorFactions;
    assert(CampaignSave::Encode(scrap,&bytes));
    const uint64_t preserved=loaded.Checksum();
    auto tampered=bytes;
    tampered[intel+16+9]=0;Reseal(tampered); // remove parley for M04
    assert(!CampaignSave::Decode(&loaded,tampered.data(),tampered.size()));
    assert(loaded.Checksum()==preserved);
    tampered=bytes;
    tampered[intel+32]=0x80;Reseal(tampered); // invalid treaty bit
    assert(!CampaignSave::Decode(&loaded,tampered.data(),tampered.size()));
    assert(loaded.Checksum()==preserved);
    tampered=bytes;
    tampered[intel+33]=0;Reseal(tampered); // no latest treaty marker
    assert(!CampaignSave::Decode(&loaded,tampered.data(),tampered.size()));
    assert(loaded.Checksum()==preserved);
    Session legacy(414);
    assert(CampaignSave::Encode(legacy,&bytes));
    bytes[intel+32]=bytes[intel+33]=0;
    bytes[8]=3;Reseal(bytes);
    assert(CampaignSave::Decode(&loaded,bytes.data(),bytes.size()));
    assert(loaded.Checksum()==legacy.Checksum());
    std::cout<<"Omega Front real alternate mission resolution and v5 save tests passed\n";
}
