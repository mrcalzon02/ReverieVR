#include "omega_front_campaign_save.h"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace reverie::omega;
static void Reseal(CampaignSave::Bytes &bytes){
    uint64_t hash=UINT64_C(14695981039346656037);
    for(size_t i=0;i<bytes.size()-8;++i){hash^=bytes[i];hash*=UINT64_C(1099511628211);}
    for(size_t i=bytes.size()-8;i<bytes.size();++i){bytes[i]=static_cast<uint8_t>(hash);hash>>=8;}
}
static Session AtOperation(int op){
    Session session(414);
    CampaignSave::Bytes bytes{};
    assert(CampaignSave::Encode(session,&bytes));
    bytes[28]=static_cast<uint8_t>(op);
    Reseal(bytes);
    Session ready(9);
    assert(CampaignSave::Decode(&ready,bytes.data(),bytes.size()));
    return ready;
}
int main(){
    Session source=AtOperation(3);
    Frontier &f=source.frontier;
    assert(f.Travel(10));
    assert(f.Scout(9));
    assert(f.Negotiate(9));
    assert(f.ConcludeAccord(9));
    assert(f.operation()==4 && f.obligation_status(3)==Obligations::Due);
    const int deadline=f.obligation_due_day(3);
    CampaignSave::Bytes bytes{};
    assert(CampaignSave::Encode(source,&bytes));
    assert(bytes[8]==9); // current v9, still fixed 2048-byte slot
    Session restored(7);
    assert(CampaignSave::Decode(&restored,bytes.data(),bytes.size()));
    assert(restored.Checksum()==source.Checksum());
    const int before=restored.frontier.salvage();
    const int beforeYard=restored.frontier.sector(9)->wreck_mass;
    while(restored.frontier.day()<deadline)assert(restored.frontier.AdvanceDay());
    assert(restored.frontier.obligation_status(3)==Obligations::Honored);
    assert(restored.frontier.salvage()==before-2);
    assert(restored.frontier.sector(9)->wreck_mass==beforeYard+2);
    assert(CampaignSave::Encode(restored,&bytes));
    Session finished(1);
    assert(CampaignSave::Decode(&finished,bytes.data(),bytes.size()));
    assert(finished.Checksum()==restored.Checksum());
    for(int i=0;i<10;++i)assert(finished.frontier.AdvanceDay());
    assert(finished.frontier.sector(9)->wreck_mass==beforeYard+2);
    // Version 4 contains no obligation terms. Migrating cannot demand a
    // payment that was never part of that historic agreement.
    CampaignSave::Bytes old{};
    assert(CampaignSave::Encode(source,&old));
    // Schema-4 representation stops immediately before the nine obligation
    // bytes, previously zero padding. Remove only those new fields.
    const size_t obligation_offset=55+16*14+Checkpoint::kBytes+17+32+2;
    for(size_t i=obligation_offset;i<obligation_offset+9;++i)old[i]=0;
    old[8]=4;Reseal(old);
    Session legacy(17);
    assert(CampaignSave::Decode(&legacy,old.data(),old.size()));
    assert(legacy.frontier.obligation_status(3)==Obligations::Legacy);
    for(int i=0;i<5;++i)assert(legacy.frontier.AdvanceDay());
    assert(legacy.frontier.obligation_status(3)==Obligations::Legacy);
    // A forged pending record cannot be overdue, even with a valid checksum.
    assert(CampaignSave::Encode(restored,&bytes));
    const uint64_t intact=finished.Checksum();
    bytes[obligation_offset]=1; // claim still due, despite elapsed deadline
    Reseal(bytes);
    assert(!CampaignSave::Decode(&finished,bytes.data(),bytes.size()));
    assert(finished.Checksum()==intact);
    std::cout<<"Omega Front dated accord obligations campaign/save tests passed\n";
}
