#include "breakwater_terrain.h"
#include "breakwater_environment.h"
#include "breakwater_materials.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using namespace reverie::breakwater;

// Verify the material assignments on the exact production terrain/landscape
// geometry callbacks. This does not invent collision: the source Box/Triangle
// sinks are untouched, and UV geometry uses only the preexisting sprite VBO.
struct ArtSink {
    std::array<unsigned,materials::kMaterialCount> counts{};
    std::vector<float> textured{};
    void operator()(materials::Surface surface,float x,float y,float z,
                    float w,float h,float d) {
        const size_t id=static_cast<size_t>(surface);
        assert(id<counts.size());
        for(float f:{x,y,z,w,h,d})assert(std::isfinite(f));
        assert(materials::AppendBoxCladding(textured,surface,x,y,z,w,h,d));
        ++counts[id];
    }
    unsigned total()const {
        unsigned n=0u;for(unsigned c:counts)n+=c;return n;
    }
    unsigned count(materials::Surface s)const {
        return counts[static_cast<size_t>(s)];
    }
    void validate()const {
        assert(textured.size()==static_cast<size_t>(total())*36u*5u);
        for(size_t i=0;i<textured.size();i+=5u) {
            for(size_t j=0;j<5u;++j)assert(std::isfinite(textured[i+j]));
            assert(textured[i+3u]>=0.f&&textured[i+3u]<=1.f);
            assert(textured[i+4u]>=0.f&&textured[i+4u]<=1.f);
        }
    }
};

int main(){
    for(uint8_t location=0;location<Campaign::kLocations;++location) {
        ArtSink referenceCoast,referenceRear;
        for(bool night:{false,true}) {
            ArtSink coast,rear;
            EmitCoastalTerrain(location,night,
                [](auto...){},[](auto...){},false,
                [&](auto...v){coast(v...);});
            EmitSurroundingLandscape(location,night,
                [](auto...){},[](auto...){},
                [&](auto...v){rear(v...);});
            coast.validate();rear.validate();
            if(location==0u) {
                assert(coast.count(materials::Surface::PebbleGround)==18u);
                assert(coast.count(materials::Surface::CratePlanks)==2u);
                assert(coast.total()==20u);
                assert(rear.count(materials::Surface::DockPlanks)==6u);
                assert(rear.total()==6u);
            } else if(location==1u) {
                assert(coast.count(materials::Surface::DockPlanks)==16u);
                assert(coast.count(materials::Surface::MossySeawall)==18u);
                assert(coast.count(materials::Surface::Steel)==34u);
                assert(coast.total()==68u);
                assert(rear.count(materials::Surface::ReinforcedConcrete)==2u);
                assert(rear.count(materials::Surface::CorrugatedRoof)==2u);
                assert(rear.count(materials::Surface::RivetedBulkhead)==2u);
                assert(rear.total()==6u);
            } else if(location==2u) {
                assert(coast.count(materials::Surface::Basalt)==26u);
                assert(coast.total()==26u);
                assert(rear.total()==0u);
            } else {
                assert(location==3u);
                assert(coast.count(materials::Surface::MossySeawall)==12u);
                assert(coast.count(materials::Surface::Steel)==8u);
                assert(coast.count(materials::Surface::ReinforcedConcrete)==6u);
                assert(coast.total()==26u);
                assert(rear.count(materials::Surface::ReinforcedConcrete)==2u);
                assert(rear.count(materials::Surface::CorrugatedRoof)==2u);
                assert(rear.count(materials::Surface::Steel)==4u);
                assert(rear.count(materials::Surface::OlivePaint)==2u);
                assert(rear.count(materials::Surface::RivetedBulkhead)==2u);
                assert(rear.total()==12u);
            }
            if(!night){referenceCoast=coast;referenceRear=rear;}
            else {
                // Stereo/day-night reuse identical immutable UV vertices;
                // the existing shader handles the global night tint.
                assert(coast.textured==referenceCoast.textured);
                assert(rear.textured==referenceRear.textured);
            }
        }
    }
    ArtSink invalid;
    EmitCoastalTerrain(Campaign::kLocations,false,
        [](auto...){},[](auto...){},false,
        [&](auto...v){invalid(v...);});
    EmitSurroundingLandscape(Campaign::kLocations,false,
        [](auto...){},[](auto...){},
        [&](auto...v){invalid(v...);});
    assert(invalid.total()==0u);
    std::cout<<"PASS: 4 Breakwater locations textured across actual outer "
                "shorelines, piers, sheds, service track, and moorings\n";
}
