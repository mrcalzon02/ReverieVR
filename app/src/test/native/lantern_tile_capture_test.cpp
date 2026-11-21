#include "lantern_tile_capture.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>
using namespace reverie::lantern::tiles;
static Tile Load(uint16_t id){
    char name[64];assert(FileNameFor(id,name,sizeof(name)));
    std::ifstream f(std::string("app/src/main/assets/")+name,std::ios::binary);
    assert(f);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),
                                std::istreambuf_iterator<char>());
    Tile tile;assert(ReadTile(bytes.data(),bytes.size(),id,&tile));return tile;
}
static uint64_t Difference(const std::vector<uint8_t>& a,
                           const std::vector<uint8_t>& b){
    assert(a.size()==b.size());uint64_t n=0;
    for(size_t i=0;i<a.size();++i)if(a[i]!=b[i])++n;
    return n;
}
static uint64_t Sum(const std::vector<uint8_t>& p){
    return std::accumulate(p.begin(),p.end(),uint64_t(0));
}
int main(){
    const auto layout=AssignWarfront(0x518bc7u);
    const uint16_t loc=127;
    auto tile=Load(layout[loc]);
    Celestial day{},dusk=day,night=day,fullMoon=day,clouded=day;
    day.day_of_year=179;day.local_hour=12.4f;
    dusk.local_hour=18.8f;
    night.local_hour=0.0f;night.moon_phase=0.f;night.star_irradiance=.003f;
    fullMoon=night;fullMoon.moon_phase=.5f;
    clouded=day;clouded.cloud_attenuation=.85f;
    assert(Valid(day)&&Valid(night)&&!Valid(Celestial{0}));
    const auto noon=Illuminate(day),midnight=Illuminate(night);
    assert(noon.sun_altitude>0.f&&noon.direct_solar>0.1f);
    assert(midnight.direct_solar==0.f);
    assert(Illuminate(fullMoon).moonlight>Illuminate(night).moonlight);
    assert(Illuminate(clouded).direct_solar<noon.direct_solar);
    CaptureLayers original,replay,shadow,starlight,moonlight,houselight;
    assert(MakePhoto(tile,{},day,&original));
    assert(MakePhoto(tile,{},day,&replay));
    assert(original.Valid()&&replay.Valid()&&
           original.photo.pixels==replay.photo.pixels);
    assert(original.building_mask==replay.building_mask&&
           original.foliage_mask==replay.foliage_mask);
    assert(Sum(original.building_mask)>0&&Sum(original.foliage_mask)>0);
    assert(Sum(original.unit_mask)==0u);
    assert(Sum(original.shadow_mask)>0u);
    assert(MakePhoto(tile,{},clouded,&shadow));
    assert(Difference(shadow.photo.pixels,original.photo.pixels)>0u);
    assert(original.foliage_mask==shadow.foliage_mask&&
           original.building_mask==shadow.building_mask);
    assert(MakePhoto(tile,{},night,&starlight));
    assert(MakePhoto(tile,{},fullMoon,&moonlight));
    assert(Sum(moonlight.photo.pixels)>Sum(starlight.photo.pixels));
    Celestial lit=night;lit.buildings_lit=true;
    assert(MakePhoto(tile,{},lit,&houselight));
    assert(Sum(houselight.photo.pixels)>Sum(starlight.photo.pixels));

    const int64_t epoch=172800; // the actual clock passed by war simulation
    std::vector<WarOrder> order={
      {1001u,loc,UnitType::Tank,Activity::Conceal,
       Doctrine::UnderFoliage,epoch,epoch+600,7,8,1,1},
      {1002u,loc,UnitType::CampTent,Activity::Camp,
       Doctrine::BehindBuildings,epoch+120,epoch+1000,3,4,18,1}
    };
    std::vector<PlacedUnit> early,encamped,cleared;
    assert(ResolveAt(tile,loc,epoch+1,order,&early));
    assert(ResolveAt(tile,loc,epoch+250,order,&encamped));
    assert(ResolveAt(tile,loc,epoch+1100,order,&cleared));
    assert(early.size()==1&&encamped.size()==2&&cleared.empty());
    CaptureLayers tank,camp,empty;
    assert(MakePhoto(tile,early,day,&tank));
    assert(MakePhoto(tile,encamped,day,&camp));
    assert(MakePhoto(tile,cleared,day,&empty));
    assert(empty.photo.pixels==original.photo.pixels);
    assert(tank.photo.pixels!=original.photo.pixels);
    assert(camp.photo.pixels!=tank.photo.pixels);
    assert(Sum(tank.unit_mask)>0&&Sum(camp.unit_mask)>Sum(tank.unit_mask));
    assert(original.building_mask==camp.building_mask&&
           original.foliage_mask==camp.foliage_mask);
    uint64_t unitBehindCover=0;
    for(size_t i=0;i<camp.unit_mask.size();++i)
        if(camp.foliage_mask[i]&&camp.unit_mask[i])++unitBehindCover;
    // Masks expose what survives visually after true canopy occlusion;
    // no World Truth label is exposed in the developed photograph.
    assert(unitBehindCover<Sum(camp.unit_mask));
    // Reopen this exact war position: imagery and static terrain repeat.
    Tile reopened=Load(layout[loc]);
    std::vector<PlacedUnit> restart;
    assert(ResolveAt(reopened,loc,epoch+250,order,&restart));
    CaptureLayers repeat;
    assert(MakePhoto(reopened,restart,day,&repeat));
    assert(repeat.photo.pixels==camp.photo.pixels);
    // Changing only celestial conditions changes shadows, NOT foliage,
    // roads, buildings, unit position, hill profile or tile assignment.
    CaptureLayers otherTime;
    assert(MakePhoto(reopened,restart,dusk,&otherTime));
    assert(otherTime.photo.pixels!=repeat.photo.pixels);
    assert(otherTime.unit_mask==repeat.unit_mask&&
           otherTime.foliage_mask==repeat.foliage_mask);
    // Invalid time must not mutate the caller's accepted photograph.
    Celestial bad=day;bad.moon_phase=-.2f;
    const auto preserved=repeat.photo.pixels;
    assert(!MakePhoto(tile,restart,bad,&repeat));
    assert(repeat.photo.pixels==preserved);
    std::cout<<"PASS Lantern terrain snapshot capture: fixed ground/structures, time-varying shadow, starlight/full moon/night lamps, 3D canopy covering units, late tents, persisted tile identity, capture-time deterministic repetition\n";
}
