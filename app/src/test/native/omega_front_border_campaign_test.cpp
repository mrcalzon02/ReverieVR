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
int main(){
    Session original(414);
    CampaignSave::Bytes bytes{};
    assert(CampaignSave::Encode(original,&bytes));
    // Controlled fixture begins at the existing Meridian first-contact stage.
    bytes[28]=4;Reseal(bytes);
    Session commander(9);
    assert(CampaignSave::Decode(&commander,bytes.data(),bytes.size()));
    Frontier &f=commander.frontier;
    assert(f.Scout(7) && f.Negotiate(7) && f.ConcludeAccord(7));
    assert(f.operation()==5 && f.obligation_status(4)==Obligations::Due);
    // Force a legitimate resource-shortage state in the test-only snapshot.
    assert(CampaignSave::Encode(commander,&bytes));
    for(int i=34;i<38;++i)bytes[i]=0; // serialized 32-bit salvage stock
    Reseal(bytes);
    Session defaulted(2);
    assert(CampaignSave::Decode(&defaulted,bytes.data(),bytes.size()));
    Frontier &d=defaulted.frontier;
    const int deadline=d.obligation_due_day(4);
    while(d.day()<deadline)assert(d.AdvanceDay());
    assert(d.obligation_status(4)==Obligations::Broken);
    const int before_rations=d.rations(),before_food=d.sector(7)->food;
    const int before_trust=d.trust(),before_relation=d.relation(2);
    const int before_claims=d.sector(7)->claims;
    assert(d.Travel(7)); // 1 movement ration plus <=2 seized by the patrol
    assert(d.rations()==before_rations-3);
    assert(d.sector(7)->food==before_food+2);
    assert(d.trust()==before_trust-2 && d.relation(2)==before_relation-4);
    assert(d.sector(7)->claims==before_claims+8);
    assert(d.border_inspected(4) && d.last_border_inspection()==4);
    assert(CampaignSave::Encode(defaulted,&bytes) && bytes[8]==9);
    Session resumed(3);
    assert(CampaignSave::Decode(&resumed,bytes.data(),bytes.size()));
    assert(resumed.Checksum()==defaulted.Checksum());
    assert(resumed.frontier.Travel(6));
    const int food=resumed.frontier.sector(7)->food;
    assert(resumed.frontier.Travel(7));
    assert(resumed.frontier.sector(7)->food==food); // no repeat confiscation
    // A forged charged treaty without the real broken status must be rejected
    // even when an attacker has recomputed the outer record checksum.
    Session clean(414);
    assert(CampaignSave::Encode(clean,&bytes));
    const size_t obligation_offset=55+16*14+Checkpoint::kBytes+17+32+2;
    const size_t border_offset=obligation_offset+9;
    bytes[border_offset]=0x10;bytes[border_offset+2]=5;Reseal(bytes);
    const uint64_t preserved=resumed.Checksum();
    assert(!CampaignSave::Decode(&resumed,bytes.data(),bytes.size()));
    assert(resumed.Checksum()==preserved);
    // v5 did not have patrol seizures. Existing defaults get grandfathered.
    assert(CampaignSave::Encode(defaulted,&bytes));
    for(size_t i=border_offset;i<border_offset+3;++i)bytes[i]=0;
    bytes[8]=5;Reseal(bytes);
    Session older(14);
    assert(CampaignSave::Decode(&older,bytes.data(),bytes.size()));
    assert(older.frontier.obligation_status(4)==Obligations::Broken);
    assert(!older.frontier.border_inspected(4));
    assert(older.frontier.Travel(6));
    const int retained=older.frontier.sector(7)->food;
    assert(older.frontier.Travel(7));
    assert(older.frontier.sector(7)->food==retained); // grandfathered breach
    std::cout << "Omega Front border patrol campaign/save regressions passed\n";
}
