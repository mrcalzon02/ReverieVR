#include "omega_front_campaign_save.h"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace reverie::omega;
static void Reseal(CampaignSave::Bytes &bytes){
    uint64_t h=UINT64_C(14695981039346656037);
    for(size_t i=0;i<bytes.size()-8;++i){h^=bytes[i];h*=UINT64_C(1099511628211);}
    for(size_t i=bytes.size()-8;i<bytes.size();++i){bytes[i]=static_cast<uint8_t>(h);h>>=8;}
}
int main(){
    Session initial(607);
    CampaignSave::Bytes bytes{};
    assert(CampaignSave::Encode(initial,&bytes));
    bytes[28]=4;Reseal(bytes); // explicitly a test fixture at Meridian stage
    Session fromSave(999);
    assert(CampaignSave::Decode(&fromSave,bytes.data(),bytes.size()));
    Frontier &f=fromSave.frontier;
    assert(!f.has_transit_right(7));
    assert(f.Scout(7) && f.Negotiate(7) && f.ConcludeAccord(7));
    assert(f.has_transit_right(7));
    assert(!f.has_transit_right(9)); // Scrapline licence is not a pass
    const int before=f.rations();
    assert(f.Travel(7));
    assert(f.rations()==before); // real paid charter saves the movement ration
    assert(f.Travel(6));
    assert(CampaignSave::Encode(fromSave,&bytes));
    Session resumed(1);
    assert(CampaignSave::Decode(&resumed,bytes.data(),bytes.size()));
    assert(resumed.frontier.has_transit_right(7));
    const int conserved=resumed.frontier.rations();
    assert(resumed.frontier.Travel(7));
    assert(resumed.frontier.rations()==conserved);
    // Also verify the fixed-sector Lost Ninth passage agreement.
    Session ninthInitial(608);
    assert(CampaignSave::Encode(ninthInitial,&bytes));
    bytes[28]=5;Reseal(bytes);
    Session ninth(2);
    assert(CampaignSave::Decode(&ninth,bytes.data(),bytes.size()));
    Frontier &n=ninth.frontier;
    assert(n.Travel(5));
    assert(n.Scout(4) && n.Negotiate(4) && n.ConcludeAccord(4));
    assert(n.has_transit_right(4));
    const int saved=n.rations();
    assert(n.Travel(4));
    assert(n.rations()==saved);
    assert(n.sector(4)->owner==0); // neutral ford not annexed
    std::cout<<"Omega Front campaign transit rights regression passed\n";
}
