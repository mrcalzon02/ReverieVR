#include "sitka_city_discoveries.h"
#include <cassert>
#include <cmath>
#include <functional>
#include <string>
#include <vector>
using namespace reverie::sitka;
static bool Bad(std::function<void()> call){
    try{call();return false;}catch(const std::invalid_argument&){return true;}
}
static TerrainMesh Flat(){
    TerrainMesh t;t.vertices={
        {-20.f,0.f,-20.f,0,0,0},{20.f,0.f,-20.f,0,0,0},
        {20.f,0.f,20.f,0,0,0},{-20.f,0.f,20.f,0,0,0}
    };t.indices={0,1,2,0,2,3};return t;
}
int main(){
    const std::string sha(64,'a');
    const std::string header="SITKA-CITY-1\nP|reviewed_city|"+
        sha+"|EPSG32608|reviewed\nO|480000|6300000\n";
    const std::string payload=header+
        "D|0|harbor|1|0\nD|1|heritage|6|0\nD|2|public_art|8|1\n";
    auto sites=ParseCitySites(payload,480000.,6300000.);
    assert(sites.size()==3u&&sites[1].kind==CityFind::Heritage);
    assert(Bad([&]{ParseCitySites(payload,0.,6300000.);}));
    assert(Bad([&]{ParseCitySites(header+"D|0|harbor|1|0\nD|0|civic|2|0\n",
                                480000.,6300000.);}));
    assert(Bad([&]{ParseCitySites(header+"D|64|harbor|1|0\n",
                                480000.,6300000.);}));
    assert(Bad([&]{ParseCitySites(header+"D|0|city_hall|1|0\n",
                                480000.,6300000.);}));
    auto land=Flat();
    std::vector<Water> water;
    std::vector<ShoreFeature> buildings;
    FreeRoamSession roam(land,water,buildings,true,0.,0.);
    auto found=FindCityVisit(roam,sites,0u,0u);
    assert(found.valid&&found.site==0u);
    Fishing fishing;
    assert(fishing.DiscoverCity(found.site,found.kind));
    assert(!fishing.DiscoverCity(found.site,found.kind));
    assert(fishing.journal.city_sites==1u);
    assert(fishing.journal.credits==0u);
    found=FindCityVisit(roam,sites,fishing.journal.city_sites,0u);
    assert(!found.valid);
    assert(!fishing.DiscoverCity(64u,CityFind::Harbor));
    assert(!fishing.DiscoverCity(20u,CityFind::Count));
    return 0;
}
