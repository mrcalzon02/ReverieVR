#include "sitka_harbor_shop.h"
#include <cassert>
#include <cmath>
#include <functional>
#include <string>
#include <vector>
using namespace reverie::sitka;
static bool Bad(std::function<void()> f){
    try{f();return false;}catch(const std::invalid_argument&){return true;}
}
static TerrainMesh Flat(){
    TerrainMesh t;
    t.vertices={
        {-20.f,0.f,-20.f,0,0,0},{20.f,0.f,-20.f,0,0,0},
        {20.f,0.f,20.f,0,0,0},{-20.f,0.f,20.f,0,0,0}};
    t.indices={0u,1u,2u,0u,2u,3u};return t;
}
int main(){
    assert(kShopItems==27u);
    const std::string source="SITKA-HARBOR-SHOPS-1\nP|reviewed_harbor|"+
        std::string(64,'a')+"|EPSG32608|reviewed\nO|480000|6300000\n";
    const std::string data=source+
        "H|thomsen|office|existing_harbor_building|1|0\n"
        "H|crescent|game_desk|real_pier|4|0\n";
    auto sites=ParseHarborShops(data,480000.,6300000.);
    assert(sites.size()==2u&&sites[0].kind==HarborLocationKind::RealOffice);
    assert(Bad([&]{ParseHarborShops(data,0.,0.);}));
    assert(Bad([&]{ParseHarborShops(source+
        "H|crescent|office|fake_office|1|0\n",480000.,6300000.);}));
    assert(Bad([&]{ParseHarborShops(source+
        "H|thomsen|office|existing_harbor_building|1|0\n"
        "H|thomsen|office|other|2|0\n",480000.,6300000.);}));
    assert(Bad([&]{ParseHarborShops(source+
        "H|madeup|game_desk|existing_harbor_building|1|0\n",
        480000.,6300000.);}));
    const auto terrain=Flat();
    std::vector<Water> water;
    std::vector<ShoreFeature> structures;
    ShoreFeature office;
    office.id="existing_harbor_building";
    office.material="harbor_shack";
    office.x=2.f;office.z=0.f;office.width=1.f;office.depth=1.f;
    office.base=0.f;office.height=3.f;
    structures.push_back(office);
    ShoreFeature pier;
    pier.id="real_pier";pier.material="pier";pier.x=4.f;pier.z=0.f;
    pier.width=2.f;pier.depth=2.f;pier.base=0.f;pier.height=1.f;
    structures.push_back(pier);
    FreeRoamSession roam(terrain,water,structures,true,0.,0.);
    assert(VerifiedHarborSite(sites[0],roam,structures));
    assert(VerifiedHarborSite(sites[1],roam,structures));
    auto absent=sites[0];absent.feature_id="invented_office";
    assert(!VerifiedHarborSite(absent,roam,structures));
    absent=sites[0];absent.x=18.f;
    assert(!VerifiedHarborSite(absent,roam,structures));
    auto signs=HarborShopSigns(sites,roam);
    assert(signs.indices.size()==24u);
    assert(!PackTerrainGles2(signs).empty());
    const float origin[3]={0.f,1.4f,0.f};
    const float forward[3]={.6247f,-.0937f,-.775f};
    assert(HarborShopTarget(sites,roam,origin,forward)==0);
    const float away[3]={-1.f,0.f,0.f};
    assert(HarborShopTarget(sites,roam,origin,away)==-1);
    FishingSession journal;
    auto& j=journal.fishing.journal;
    // Cash alone never bypasses a beach discovery.
    j.credits=10000u;
    HarborShopSession shop;
    assert(shop.Update(true,false,0.f,0,journal,false)==ShopEvent::Open);
    assert(shop.open()&&shop.consumes_primary());
    assert(shop.Update(true,false,0.f,0,journal,false)==ShopEvent::None);
    shop.Update(false,false,0.f,-1,journal,false);
    assert(shop.Update(true,false,0.f,-1,journal,false)==ShopEvent::Denied);
    assert(j.hull_tier==0u&&j.credits==10000u);
    shop.Update(false,false,0.f,-1,journal,false);
    assert(journal.Discover(0u,BeachFind::Shell));
    assert(shop.Update(true,false,0.f,-1,journal,false)==ShopEvent::Purchased);
    assert(j.hull_tier==1u&&j.credits==9700u);
    assert(journal.dirty);
    assert(shop.Update(false,true,0.f,-1,journal,false)==ShopEvent::Close);
    assert(!shop.open());
    assert(shop.Update(true,true,0.f,0,journal,true)==ShopEvent::None);
    shop.Update(false,false,0.f,-1,journal,false);
    assert(shop.Update(true,false,0.f,1,journal,false)==ShopEvent::Open);
    // Same persistent wallet from another harbor.
    assert(shop.Update(false,false,-1.f,-1,journal,false)==ShopEvent::Next);
    assert(shop.selected()==1u);
    assert(shop.Update(false,false,-1.f,-1,journal,false)==ShopEvent::None);
    assert(shop.Update(false,false,0.f,-1,journal,false)==ShopEvent::None);
    assert(shop.Update(true,false,0.f,-1,journal,false)==ShopEvent::Purchased);
    assert(j.helm_upgrades&HelmMask(HelmItem::Battery));
    assert(!shop.open()==false);
    const auto summary=HarborShopPanel(shop,journal);
    assert(summary[0].find("HARBOR")!=std::string::npos);
    assert(summary[1].find("9700")==std::string::npos);
    const auto save=SaveJournal(j);
    Journal replay;
    assert(LoadJournal(save,replay)&&
           replay.hull_tier==1u&&
           (replay.helm_upgrades&HelmMask(HelmItem::Battery)));
    // Browse all 27 rows and buy an existing reel from a real shop.
    for(int i=0;i<16;++i){
        shop.Update(false,false,0.f,-1,journal,false);
        shop.Update(false,false,-1.f,-1,journal,false);
    }
    assert(shop.selected()==17u);
    assert(HarborOffer(j,17u).title.find("Fishing rod")!=std::string::npos);
    // Existing equipment and skill gates are reused, not duplicated.
    assert(!HarborOffer(j,17u).unlocked); // 1 shell site only, already level 0?
    assert(NextEquipment(j,Equipment::Rod).unlocked);
    return 0;
}
