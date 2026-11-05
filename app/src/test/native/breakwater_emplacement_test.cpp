#include "breakwater_terrain.h"
#include "breakwater_emplacement.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace reverie::breakwater;

int main() {
    assert(kBeachCrossSections>=16 && kBeachLengthSections>=12);
    assert(BeachHeight(0,0.f,-1.5f)<-.98f);
    // The offshore shoulder still rises, but its rear 44 cm now welds into
    // the actual surrounding landscape rather than ending as a raised lip.
    assert(BeachHeight(0,3.8f,-.70f)>-.90f);
    const float rearEdge=RearGroundHeight(
        EnvironmentFor(0,false),3.8f,-.26f);
    assert(std::abs(BeachHeight(0,3.8f,-.30f)-rearEdge)<.02f);
    assert(std::abs(BeachHeight(0,3.8f,-.26f)-rearEdge)<.00001f);
    for(uint8_t location=0;location<Campaign::kLocations;++location) {
        int boxes=0,faces=0;
        float minDepth=100.f,maxDepth=-100.f;
        auto box=[&](float x,float y,float z,float w,float h,float d,
                     float r,float g,float b) {
            ++boxes;
            assert(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z));
            assert(w>0.f&&h>0.f&&d>0.f);
            assert(r>=0.f&&g>=0.f&&b>=0.f);
            // The front-center shooting slit must remain open.
            if(z<-.20f && std::abs(x)<1.49f)
                assert(y+h*.5f<-.4f);
            minDepth=std::min(minDepth,z-d*.5f);
            maxDepth=std::max(maxDepth,z+d*.5f);
        };
        auto triangle=[&](float x0,float y0,float z0,
                          float x1,float y1,float z1,
                          float x2,float y2,float z2,
                          float r,float g,float b) {
            ++faces;
            for(float v:{x0,y0,z0,x1,y1,z1,x2,y2,z2,r,g,b})
                assert(std::isfinite(v));
            assert(r>=0.f&&g>=0.f&&b>=0.f);
            // No barrier spans the entire gun's forward traverse.
            assert(!(x0<-1.f && x1>1.f && z0<-.40f && z1<-.40f &&
                     y0>-.45f && y1>-.45f));
        };
        EmitBatteryEmplacement(location,false,box,triangle);
        // The emitter now builds 50 solid 3D elements plus six actual
        // ramp/cheek faces; the former ten-triangle quota counted surfaces
        // that were replaced by solid geometry and textured cladding.
        assert(boxes>=50 && faces>=6);
        assert(minDepth<-.6f && maxDepth>2.0f);
        const int dayBoxes=boxes,dayFaces=faces;
        int cladding=0,sandbags=0,metalwork=0,crates=0,tarps=0,roofs=0,deck=0;
        const auto present=[&](materials::Surface surface){
            if(surface==materials::Surface::CratePlanks)++crates;
            if(surface==materials::Surface::CamoNetting)++tarps;
            if(surface==materials::Surface::CorrugatedRoof)++roofs;
            if(surface==materials::Surface::DiamondPlate)++deck;
        };
        EmitBatteryEmplacement(location,false,
            [](auto...){},[](auto...){},
            [&](materials::Surface surface,float x,float y,float z,
                float w,float h,float d) {
                ++cladding;
                present(surface);
                for(float value:{x,y,z,w,h,d})assert(std::isfinite(value));
                assert(w>0.f && h>0.f && d>0.f);
                if(surface==materials::Surface::Sandbag)++sandbags;
                if(surface==materials::Surface::Steel ||
                   surface==materials::Surface::OlivePaint)++metalwork;
            });
        // Twelve real sandbags, concrete/basalt defensive works and
        // shore stakes/door/store housings all receive textured UV faces.
        assert(cladding>=34 && sandbags==12 && metalwork>=10);
        assert(crates==2 && tarps==2 && roofs==4 && deck==8);
        boxes=0;faces=0;
        EmitBatteryEmplacement(location,true,box,triangle);
        assert(boxes==dayBoxes && faces==dayFaces);
    }
    int invalid=0;
    EmitBatteryEmplacement(Campaign::kLocations,false,
                           [&](auto...){++invalid;},[&](auto...){++invalid;});
    assert(invalid==0);
    std::cout<<"Breakwater 3D bunker, clear embrasure and contiguous terrain tests passed\n";
}
