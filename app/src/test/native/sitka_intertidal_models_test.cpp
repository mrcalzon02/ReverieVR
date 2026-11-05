#include "sitka_beach_discoveries.h"
#include <cassert>
#include <cmath>
#include <functional>
#include <set>
#include <string>
#include <vector>
using namespace reverie::sitka;
static bool Throws(std::function<void()> f){
    try{f();return false;}catch(const std::invalid_argument&){return true;}
}
static TerrainMesh Ground(){
    TerrainMesh t;
    t.vertices={
        {-20.f,0.f,-20.f,0,0,0},{20.f,0.f,-20.f,0,0,0},
        {20.f,0.f,20.f,0,0,0},{-20.f,0.f,20.f,0,0,0}
    };t.indices={0,1,2,0,2,3};return t;
}
int main(){
    static_assert(kShoreSpecimens.size()==25u,"25 species");
    std::set<std::string> tokens;
    for(unsigned i=0;i<kShoreSpecimens.size();++i){
        const auto& def=kShoreSpecimens[i];
        ShoreSpecimen find=ShoreSpecimen::EmptyClamShell;
        assert(ShoreSpecimenByToken(def.token,find)&&unsigned(find)==i);
        assert(tokens.insert(def.token).second);
        assert(def.radius_m>=.020f&&def.radius_m<=.10f);
        assert(def.zones!=0u&&def.rarity>=1u&&def.rarity<=4u);
        TerrainMesh mesh;
        AppendShoreSpecimen(mesh,find,0.f,0.f,0.f,true);
        assert(!mesh.indices.empty()&&mesh.indices.size()<360u);
        assert(mesh.indices.size()==mesh.vertices.size());
        assert(mesh.indices.size()%3u==0u);
        for(const auto& v:mesh.vertices){
            assert(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z));
            assert(std::abs(v.x)<=.2f&&std::abs(v.z)<=.2f);
            assert(v.y>=-.02f&&v.y<=.2f);
        }
        assert(!PackTerrainGles2(mesh).empty());
        if(def.action==ShoreAction::Document){
            assert(def.category==BeachFind::Heritage);
            assert(def.zones==unsigned(ShoreZone::Cultural));
        }
        if(def.action==ShoreAction::Observe)
            assert(def.category==BeachFind::MarineLife||
                   def.category==BeachFind::Crab);
    }
    assert(tokens.size()==25u);
    unsigned zone=0u;
    assert(ShoreZoneByToken("rocky",zone)&&zone==1u);
    assert(!ShoreZoneByToken("inland",zone));
    const std::string base="SITKA-BEACH-1\nP|reviewed_shore|"+
        std::string(64,'a')+"|EPSG32608|reviewed\nO|480000|6300000\n";
    const auto sites=ParseBeachSites(base+
        "D|0|purple_shore_crab|1|0|rocky\n"
        "D|1|empty_clam_shell|4|0|gravel\n"
        "D|2|possible_worked_stone|.5|0|protected_cultural\n"
        "D|3|small_sea_star|10|0|tidepool\n",480000.,6300000.);
    assert(sites.size()==4u&&sites[0].kind==BeachFind::Crab);
    assert(sites[2].kind==BeachFind::Heritage);
    assert(Throws([&]{ParseBeachSites(
        base+"D|5|purple_shore_crab|1|0|gravel\n",
        480000.,6300000.);}));
    assert(Throws([&]{ParseBeachSites(
        base+"D|5|purple_shore_crab|1|0\n",
        480000.,6300000.);}));
    assert(Throws([&]{ParseBeachSites(
        base+"D|5|possible_net_weight|1|0|rocky\n",
        480000.,6300000.);}));
    const auto terrain=Ground();
    const std::vector<Water> no_water;
    const std::vector<ShoreFeature> no_buildings;
    FreeRoamSession roam(terrain,no_water,no_buildings,true,0.,0.);
    Journal journal;
    auto visible=BeachVisibleMask(sites,journal,roam);
    assert(visible==((uint64_t(1)<<0u)|(uint64_t(1)<<2u)));
    auto mesh=BeachMarkerMesh(sites,journal,roam,visible);
    assert(!mesh.indices.empty());
    const size_t before=mesh.indices.size();
    const std::vector<BeachSite> distant={
        {4,BeachFind::Crab,18.f,0.f,ShoreSpecimen::PurpleShoreCrab,
         unsigned(ShoreZone::Rocky)}
    };
    assert(BeachVisibleMask(distant,journal,roam)==0u);
    assert(BeachMarkerMesh(distant,journal,roam,~uint64_t(0)).indices.empty());
    const float source[3]={0.f,1.5f,0.f};
    const float ray[3]={.56f,-.8285f,0.f};
    const auto target=FindBeachTarget(source,ray,0.,0.,roam,sites,0u,0u);
    assert(target.valid&&target.specimen==ShoreSpecimen::PurpleShoreCrab);
    assert(target.action==ShoreAction::Observe);
    Fishing f;
    assert(f.Discover(target.site,target.kind)&&f.journal.credits==0u);
    visible=BeachVisibleMask(sites,f.journal,roam);
    assert(visible&1u); // observed crab stays visible
    mesh=BeachMarkerMesh(sites,f.journal,roam,visible);
    assert(mesh.indices.size()<before); // shine has stopped
    assert(f.Discover(2u,BeachFind::Heritage));
    assert(f.journal.credits==0u);
    visible=BeachVisibleMask(sites,f.journal,roam);
    assert(visible&(uint64_t(1)<<2u)); // possible artifact remains
    assert(!f.Discover(2u,BeachFind::Heritage));
    const float offray[3]={.92f,-.39f,0.f};
    assert(!FindBeachTarget(source,offray,0.,0.,roam,sites,0u,8u,8u).valid);
    const auto legacy=ParseBeachSites(base+
        "D|0|shell|1|0\nD|1|crab|2|0\nD|2|rock|3|0\n"
        "D|3|sea_glass|4|0\nD|4|driftwood|5|0\n",
        480000.,6300000.);
    assert(legacy.size()==5u);
    assert(legacy[0].specimen==ShoreSpecimen::EmptyClamShell);
    assert(legacy[1].kind==BeachFind::Crab);
    const auto high=FindPrerequisites(12u);
    assert(!(high&FindMask(BeachFind::Heritage)));
    assert(!(high&FindMask(BeachFind::MarineLife)));
    return 0;
}
