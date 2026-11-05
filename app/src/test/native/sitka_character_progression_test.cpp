#include "sitka_fishing_session.h"
#include "sitka_free_roam.h"
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
using namespace reverie::sitka;
int main(){
    FishingSession session;
    auto& j=session.fishing.journal;
    j.credits=200000u;
    assert(!session.PurchaseCharacter(CharacterSkill::Stride));
    assert(session.Discover(0u,BeachFind::Shell));
    const auto first=NextCharacter(j,CharacterSkill::Stride);
    assert(first.available&&first.unlocked&&first.affordable);
    assert(first.required_beach==1u&&first.required_city==0u);
    assert(session.PurchaseCharacter(CharacterSkill::Stride));
    assert(j.stride_level==1u&&j.credits==200000u-first.credits);
    assert(!session.PurchaseCharacter(CharacterSkill::Stride));
    assert(session.Discover(1u,BeachFind::Crab));
    assert(session.PurchaseCharacter(CharacterSkill::Stride));
    assert(!session.PurchaseCharacter(CharacterSkill::Stride));
    assert(session.Discover(2u,BeachFind::Rock));
    assert(!NextCharacter(j,CharacterSkill::Stride).unlocked);
    assert(session.DiscoverCity(0u,CityFind::Harbor));
    assert(NextCharacter(j,CharacterSkill::Stride).unlocked);
    assert(session.PurchaseCharacter(CharacterSkill::Stride));
    assert(!session.DiscoverCity(0u,CityFind::Harbor));
    assert(session.DiscoverCity(1u,CityFind::Heritage));
    assert(session.DiscoverCity(2u,CityFind::Civic));
    assert(session.DiscoverCity(3u,CityFind::Trailhead));
    assert(session.DiscoverCity(4u,CityFind::PublicArt));
    assert(session.DiscoverCity(5u,CityFind::Harbor));
    for(unsigned i=3u;i<8u;++i)
        assert(session.Discover(i,BeachFind::SeaGlass));
    while(j.stride_level<8u)assert(session.PurchaseCharacter(CharacterSkill::Stride));
    assert(!session.PurchaseCharacter(CharacterSkill::Stride));
    assert(std::abs(CharacterWalkScale(j)-1.4f)<1.e-5f);
    for(unsigned skill=1u;skill<unsigned(CharacterSkill::Count);++skill){
        while(CharacterLevel(j,CharacterSkill(skill))<8u)
            assert(session.PurchaseCharacter(CharacterSkill(skill)));
        assert(!session.PurchaseCharacter(CharacterSkill(skill)));
    }
    assert(j.rod_handling_level==8u&&j.endurance_level==8u);
    assert(j.observation_level==8u&&j.deckhand_level==8u);
    const auto saved=SaveJournal(j);
    assert(saved.size()<384u);
    Journal restored;
    assert(LoadJournal(saved,restored));
    assert(restored.city_sites==j.city_sites);
    assert(restored.stride_level==8u&&restored.deckhand_level==8u);
    assert(SaveJournal(restored)==saved);
    const auto before=SaveJournal(restored);
    assert(!LoadJournal(saved+"garbage",restored));
    assert(SaveJournal(restored)==before);
    const std::string v3="SITKA-FISH-3\n123\n400\n1\n1\n1\n0\n0\n0\n"
                         "3\n2\n2\n2\n2\n2\n1\n1\n";
    Journal legacy;
    assert(LoadJournal(v3,legacy));
    assert(legacy.credits==400u&&legacy.hull_tier==2u);
    assert(legacy.stride_level==0u&&legacy.city_sites==0u);
    TerrainMesh terrain;
    terrain.vertices={
        {-30.f,0.f,-30.f,0,0,0},{30.f,0.f,-30.f,0,0,0},
        {30.f,0.f,30.f,0,0,0},{-30.f,0.f,30.f,0,0,0}
    };terrain.indices={0,1,2,0,2,3};
    std::vector<Water> water;std::vector<ShoreFeature> objects;
    FreeRoamSession roam(terrain,water,objects,true,0.,0.);
    const float right[3]={1.f,0.f,0.f};
    for(unsigned i=0;i<20u;++i)
        assert(roam.Advance(1.f,0.f,right,.05f,CharacterWalkScale(j)));
    assert(std::abs(roam.pose().x-3.92f)<.025f);
    assert(!roam.Advance(.28f,0.f,right,.05f,CharacterWalkScale(j)));
    assert(!roam.Advance(1.f,0.f,right,.05f,9.f));
    assert(!roam.Advance(1.f,0.f,right,.05f,NAN));
    Fishing base;
    Fishing trained;
    trained.journal.rod_handling_level=8u;
    trained.journal.endurance_level=8u;
    base.Cast(true,3.f);trained.Cast(true,3.f);
    for(int i=0;i<100;i++){
        base.Step(.1f,false,false);trained.Step(.1f,false,false);
        if(base.phase==FishingPhase::Bite&&
           trained.phase==FishingPhase::Bite)break;
    }
    assert(base.phase==FishingPhase::Bite);
    assert(trained.phase==FishingPhase::Bite);
    assert(trained.time>base.time);
    base.Step(.05f,true,false);trained.Step(.05f,true,false);
    for(int i=0;i<10;++i){base.Step(.1f,true,false);trained.Step(.1f,true,false);}
    assert(trained.reel>base.reel&&trained.tension<base.tension);
    return 0;
}
