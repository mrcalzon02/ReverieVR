#include "sitka_fishing_spots.h"
#include <cassert>
#include <functional>
#include <iostream>
using namespace reverie::sitka;
namespace reverie { namespace sitka {
struct TestWater {std::string id;float x,y,z,radius;};
inline bool FootprintContains(const TestWater& w,float x,float z){
    return (x-w.x)*(x-w.x)+(z-w.z)*(z-w.z)<=w.radius*w.radius;
}
}}
static bool rejects(std::function<void()> f){try{f();return false;}catch(const std::invalid_argument&){return true;}}
int main(){
    const auto sites=ParseFishingSpots("SITKA-FISH-SITES-1\n# survey export\nberth_1 ocean 0 0 3 4.2\n");
    assert(sites.size()==1&&sites[0].depth_metres==4.2f);
    std::vector<TestWater> water={{"ocean",0,0,0,12}};
    const auto target=FindFishingTarget(FishingRay{0,2,0,0,-1,0},sites,water);
    assert(target.valid&&target.depth_metres==4.2f);
    assert(!FindFishingTarget(FishingRay{8,2,0,0,-1,0},sites,water).valid);
    assert(!FindFishingTarget(FishingRay{0,2,0,0,1,0},sites,water).valid);
    assert(!FindFishingTarget(FishingRay{0,2,0,0,0,0},sites,water).valid);
    assert(!FindFishingTarget(FishingRay{0,2,0,0,-1,0},sites,std::vector<TestWater>{{"other",0,0,0,12}}).valid);
    assert(!FindFishingTarget(FishingRay{0,2,0,0,-1,0},sites,std::vector<TestWater>{{"ocean",100,0,0,12}}).valid);
    assert(rejects([]{ParseFishingSpots("SITKA-FISH-SITES-1\n");}));
    assert(rejects([]{ParseFishingSpots("SITKA-FISH-SITES-0\n");}));
    assert(rejects([]{ParseFishingSpots("SITKA-FISH-SITES-1\nberth ocean nan 0 3 4\n");}));
    assert(rejects([]{ParseFishingSpots("SITKA-FISH-SITES-1\nberth ocean 0 0 15 4\n");}));
    assert(rejects([]{ParseFishingSpots("SITKA-FISH-SITES-1\nberth ocean 0 0 3 4\nberth ocean 0 0 3 4\n");}));
    std::cout<<"Sitka surveyed-site parser and targeted ray gate PASS\n";
}
