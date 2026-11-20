#include "sitka_fishing_spots.h"
#include "sitka_boat_angling.h"
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
    // Actual launched-boat fishing uses the very same named, author-approved
    // target and journal; no surveyed/procedural ambiguity in real geography.
    Water bay;bay.id="ocean";bay.shape="SQUARE";
    bay.x=0.f;bay.z=0.f;bay.width=24.f;bay.depth=24.f;bay.y=.15f;
    std::vector<Water> checkedWater{bay};
    BoatPose aboard{0.,0.,0.,0.,.35,true};
    const DepthReading survey{DepthStatus::Safe,4.2,3.85,BedQuality::Survey};
    const DepthReading procedural{DepthStatus::Safe,4.2,3.85,BedQuality::Procedural};
    const auto ready=BoatRodCastTarget(aboard,0.,survey,false,sites,checkedWater,0.,0.);
    assert(ready.status==BoatRodStatus::Ready&&ready.cast.valid);
    assert(ready.cast.depth_metres==sites[0].depth_metres);
    assert(BoatRodCastTarget(aboard,0.,procedural,false,sites,checkedWater,0.,0.)
           .status==BoatRodStatus::NoDepth);
    assert(BoatRodCastTarget(aboard,0.,procedural,true,sites,checkedWater,0.,0.)
           .status==BoatRodStatus::Ready); // explicitly synthetic range only
    assert(BoatRodCastTarget(aboard,.4,survey,false,sites,checkedWater,0.,0.)
           .status==BoatRodStatus::Underway);
    aboard.speed_mps=.16;
    assert(BoatRodCastTarget(aboard,0.,survey,false,sites,checkedWater,0.,0.)
           .status==BoatRodStatus::Underway);
    aboard.speed_mps=0.;
    aboard.east=8.;
    assert(BoatRodCastTarget(aboard,0.,survey,false,sites,checkedWater,0.,0.)
           .status==BoatRodStatus::NoAuthoredSite);
    aboard.east=0.;
    assert(BoatRodCastTarget(aboard,0.,survey,false,sites,
        std::vector<Water>{},0.,0.).status==BoatRodStatus::NoAuthoredSite);
    assert(BoatRodCastTarget(aboard,0.,
        DepthReading{DepthStatus::NoCoverage,0.,0.,BedQuality::Missing},
        false,sites,checkedWater,0.,0.).status==BoatRodStatus::NoDepth);
    aboard.launched=false;
    assert(BoatRodCastTarget(aboard,0.,survey,false,sites,checkedWater,0.,0.)
           .status==BoatRodStatus::NotAboard);
    aboard.launched=true;
    // Use the actual rod session: finite bite, deliberate hook and reel,
    // earned fish inventory, and original checked journal save/reopen.
    FishingSession rod;
    assert(rod.Update(.05f,true,false,ready.cast.valid,ready.cast.depth_metres)
           .event==FishingEvent::Cast);
    rod.Update(.05f,false,false,false,0.f);
    for(int i=0;i<120&&rod.fishing.phase!=FishingPhase::Bite;++i)
        rod.Update(.1f,false,false,false,0.f);
    assert(rod.fishing.phase==FishingPhase::Bite);
    assert(rod.Update(.1f,true,false,false,0.f).event==FishingEvent::ReelStarted);
    for(int i=0;i<1200&&rod.fishing.phase!=FishingPhase::Landed;++i)
        rod.Update(.1f,i%8<4,false,false,0.f);
    assert(rod.fishing.phase==FishingPhase::Landed);
    assert(rod.fishing.journal.total==1u&&rod.dirty);
    const auto journal=SaveJournal(rod.fishing.journal);
    Journal restored;
    assert(LoadJournal(journal,restored)&&SaveJournal(restored)==journal);
    assert(restored.total==1u);
    assert(BoatRodHint(BoatRodStatus::Underway,Fishing{}).find("SLOW")!=
           std::string::npos);
    assert(BoatRodHint(BoatRodStatus::Ready,Fishing{}).find("SELECT")!=
           std::string::npos);
    std::cout<<"Sitka surveyed-site parser, safe stopped-boat rod/catch/save and targeted ray gate PASS\n";
}
