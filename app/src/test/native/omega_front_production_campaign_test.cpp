#include "omega_front_campaign_save.h"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace reverie::omega;

static void Reseal(CampaignSave::Bytes &bytes) {
    uint64_t hash=UINT64_C(14695981039346656037);
    for(size_t i=0;i<bytes.size()-8;++i){
        hash^=bytes[i];hash*=UINT64_C(1099511628211);
    }
    for(size_t i=bytes.size()-8;i<bytes.size();++i){
        bytes[i]=static_cast<uint8_t>(hash);hash>>=8;
    }
}
int main() {
    Session economy(414);
    assert(economy.frontier.sector(6)->food==44);
    assert(economy.frontier.sector(6)->water==34);
    assert(economy.frontier.sector(10)->water==53);
    assert(economy.frontier.AdvanceDay());
    assert(economy.frontier.sector(6)->food==41); // 4 eaten, 1 farmed
    assert(economy.frontier.sector(6)->water==31); // 4 used, 1 drawn
    assert(economy.frontier.sector(10)->water==59); // working pump +6
    CampaignSave::Bytes bytes{};
    assert(CampaignSave::Encode(economy,&bytes));
    Session restored(8);
    assert(CampaignSave::Decode(&restored,bytes.data(),bytes.size()));
    assert(restored.Checksum()==economy.Checksum());
    // Actual pump destruction stops only the S011 generated water, not
    // unrelated farms, and the campaign codec preserves that boundary.
    Session original(414);
    assert(CampaignSave::Encode(original,&bytes));
    bytes[39]=0; // schema 6 pump health fixture, not a gameplay cheat
    Reseal(bytes);
    Session broken(9);
    assert(CampaignSave::Decode(&broken,bytes.data(),bytes.size()));
    assert(broken.frontier.pump_health()==0);
    const auto oldWater=broken.frontier.sector(10)->water;
    assert(broken.frontier.AdvanceDay());
    assert(broken.frontier.sector(10)->water==oldWater);
    assert(broken.frontier.sector(6)->food==41);
    assert(CampaignSave::Encode(broken,&bytes));
    Session reloaded(10);
    assert(CampaignSave::Decode(&reloaded,bytes.data(),bytes.size()));
    assert(reloaded.Checksum()==broken.Checksum());
    while(reloaded.frontier.day()<4)assert(reloaded.frontier.AdvanceDay());
    assert(reloaded.frontier.sector(8)->residents==0);
    assert(reloaded.frontier.sector(8)->food==0);
    assert(reloaded.frontier.sector(8)->water==0);
    std::cout<<"Omega Front production, pump damage and persistence passed\n";
}
