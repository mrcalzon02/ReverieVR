#include "sitka_beach_discoveries.h"
#include <cassert>
#include <cmath>
#include <functional>
#include <string>
#include <vector>
using namespace reverie::sitka;
static bool Rejects(std::function<void()> act){
    try{act();return false;}catch(const std::invalid_argument&){return true;}
}
static TerrainMesh Land(){
    TerrainMesh m;
    m.vertices={
        {-10.f,1.f,-10.f,75,75,75},{10.f,1.f,-10.f,75,75,75},
        {10.f,1.f,10.f,75,75,75},{-10.f,1.f,10.f,75,75,75}};
    m.indices={0,1,2,0,2,3};
    return m;
}
int main(){
    const std::string provenance(64u,'a');
    const std::string prefix="SITKA-BEACH-1\n"
        "P|reviewed_shore|"+provenance+"|EPSG32608|reviewed\n"
        "O|480000.125|6300000.625\n";
    const auto records=prefix+
        "D|0|shell|1|0\n"
        "D|3|crab|2|0\n"
        "D|4|rock|3|0\n"
        "D|7|sea_glass|4|0\n"
        "D|9|driftwood|5|0\n";
    auto sites=ParseBeachSites(records,480000.125,6300000.625);
    assert(sites.size()==5u&&sites[0].id==0u);
    assert(sites[1].kind==BeachFind::Crab);
    // Only an explicit six-field authored supply cache can yield sale stock.
    const auto supplies=ParseBeachSites(
        prefix+"D|2|shellfish_bundle|1|0|gravel\n",
        480000.125,6300000.625);
    assert(supplies.size()==1u&&supplies[0].shellfish_bundle);
    assert(supplies[0].specimen==ShoreSpecimen::EmptyClamShell);
    assert(!sites[0].shellfish_bundle);
    assert(Rejects([&]{ParseBeachSites(
        prefix+"D|2|shellfish_bundle|1|0\n",
        480000.125,6300000.625);}));
    assert(Rejects([&]{ParseBeachSites(
        prefix+"D|2|shellfish_bundle|1|0|rocky\n",
        480000.125,6300000.625);}));
    assert(Rejects([&]{ParseBeachSites(records,0.,0.);}));
    assert(Rejects([&]{ParseBeachSites(prefix+"D|0|shell|1|0\nD|0|crab|2|0\n",
                                    480000.125,6300000.625);}));
    assert(Rejects([&]{ParseBeachSites(prefix+"D|64|shell|1|0\n",
                                    480000.125,6300000.625);}));
    assert(Rejects([&]{ParseBeachSites(prefix+"D|1|magic|1|0\n",
                                    480000.125,6300000.625);}));
    std::vector<Water> water;
    std::vector<ShoreFeature> buildings;
    const auto land=Land();
    FreeRoamSession roam(land,water,buildings,true,0.,0.);
    Journal journal;
    auto near=BeachVisibleMask(sites,journal,roam);
    assert(near==uint64_t(1)); // no far markers
    auto markers=BeachMarkerMesh(sites,journal,roam,near);
    assert(!markers.vertices.empty()&&!markers.indices.empty());
    assert(markers.vertices.size()<360u);
    const float source[3]={0.f,2.f,0.f};
    const float ray[3]={1.f,0.f,0.f};
    auto found=FindBeachTarget(source,ray,0.,0.,roam,sites,0u,0u);
    assert(!found.valid); // horizontal ray cannot hit lower ground props
    const float toward_shell[3]={.71f,-.71f,0.f};
    found=FindBeachTarget(source,toward_shell,0.,0.,roam,sites,0u,0u);
    assert(found.valid&&found.site==0u&&found.kind==BeachFind::Shell);
    Fishing f;
    f.journal.credits=200000u;
    assert(!f.Purchase(Equipment::Rod));
    assert(f.Discover(found.site,found.kind));
    assert(f.SubmitFinds());
    assert(f.Purchase(Equipment::Rod));
    assert(f.journal.gear==1u);
    assert(f.journal.credits<200000u);
    const uint32_t money=f.journal.credits;
    assert(!f.Discover(found.site,found.kind));
    assert(f.journal.credits==money);
    found=FindBeachTarget(source,toward_shell,0.,0.,roam,sites,
                          f.journal.beach_sites,0u);
    assert(!found.valid);
    near=BeachVisibleMask(sites,f.journal,roam);
    assert(near==0u); // collected shell leaves shoreline
    markers=BeachMarkerMesh(sites,f.journal,roam,near);
    assert(markers.vertices.empty()&&markers.indices.empty());
    for(uint32_t i=10u;i<20u;++i)assert(f.Discover(i,BeachFind::Shell));
    assert(f.SubmitFinds());
    while(f.journal.gear<4u)assert(f.Purchase(Equipment::Rod));
    assert(!f.Purchase(Equipment::Rod)); // crab required from level five
    assert(f.Discover(3u,BeachFind::Crab));
    assert(f.Discover(4u,BeachFind::Rock));
    assert(f.Discover(7u,BeachFind::SeaGlass));
    assert(f.Discover(9u,BeachFind::Driftwood));
    assert(f.SubmitFinds());
    // There are 12 named rod levels and 48 purchasable equipment stages.
    assert(kRodNames.size()==13u);
    unsigned stages=0;
    for(unsigned n:kEquipmentMax)stages+=n;
    assert(stages==48u);
    while(f.journal.gear<12u)assert(f.Purchase(Equipment::Rod));
    assert(!f.Purchase(Equipment::Rod));
    assert(f.journal.gear==12u);
    const auto save=SaveJournal(f.journal);
    assert(save.size()<=256u);
    Journal again;
    assert(LoadJournal(save,again)&&SaveJournal(again)==save);
    assert(again.beach_sites==f.journal.beach_sites);
    assert(again.discoveries==f.journal.discoveries);
    assert(!LoadJournal(save+"junk",again));
    assert(SaveJournal(again)==save);
    return 0;
}
