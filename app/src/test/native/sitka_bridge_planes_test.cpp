#include "sitka_bridge_planes.h"
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
using namespace reverie::sitka;

template<class F> bool Reject(F f){
    try{f();return false;}catch(const std::invalid_argument&){return true;}
}
int main(){
    // Synthetic local axis tests the documented simplified representation.
    // This is NOT a claimed georeferenced bridge placement.
    const std::vector<BridgeAxisPoint> axis={
        {0.f,15.f,0.f},{0.f,15.5f,30.f},
        {0.f,15.8f,68.f},{0.f,15.5f,137.16f}};
    const auto mesh=BuildBridgePlanes(axis,11.5824f,.6f,30.48f);
    assert(mesh.planes.size()==8u);        // 3 road + 3 underside + 2 sides
    assert(mesh.triangle_count()==16u);
    assert(mesh.vertex_count()==32u);
    assert(mesh.Count(BridgePlaneMaterial::RoadConcrete)==3u);
    assert(mesh.Count(BridgePlaneMaterial::UnderdeckConcrete)==3u);
    assert(mesh.Count(BridgePlaneMaterial::SideSilhouette)==2u);
    assert(std::string(BridgePlaneTexturePath(
        BridgePlaneMaterial::RoadConcrete)).find("quay_concrete.png")!=
        std::string::npos);
    assert(std::string(BridgePlaneTexturePath(
        BridgePlaneMaterial::UnderdeckConcrete)).find("reinforced_concrete.png")!=
        std::string::npos);
    assert(std::string(BridgePlaneTexturePath(
        BridgePlaneMaterial::SideSilhouette)).find("oconnell_side.png")!=
        std::string::npos);
    // Road upper face and lower underdeck have the requested separate slots.
    assert(mesh.planes[0].material==BridgePlaneMaterial::RoadConcrete);
    assert(mesh.planes[1].material==BridgePlaneMaterial::UnderdeckConcrete);
    assert(mesh.planes[0].vertex[0].y==15.f);
    assert(mesh.planes[1].vertex[3].y<15.f);
    // UVs tile concrete while each entire side art stretches exactly once.
    assert(mesh.planes[4].vertex[2].v>1.f);
    for(size_t i=6;i<8;++i){
        assert(mesh.planes[i].material==BridgePlaneMaterial::SideSilhouette);
        assert(mesh.planes[i].vertex[0].u==0.f);
        assert(mesh.planes[i].vertex[1].u==1.f);
        assert(mesh.planes[i].vertex[2].v==0.f);
        assert(mesh.planes[i].vertex[0].v==1.f);
    }
    // Repeatability is exact; there is no procedural decoration/noise.
    const auto repeat=BuildBridgePlanes(axis,11.5824f,.6f,30.48f);
    assert(repeat.planes.size()==mesh.planes.size());
    for(size_t i=0;i<mesh.planes.size();++i)
        for(size_t k=0;k<4;++k){
            const auto& a=mesh.planes[i].vertex[k];
            const auto& b=repeat.planes[i].vertex[k];
            assert(a.x==b.x && a.y==b.y && a.z==b.z);
            assert(a.u==b.u && a.v==b.v);
        }
    assert(Reject([&]{BuildBridgePlanes({},11.f,.6f,30.f);}));
    assert(Reject([&]{BuildBridgePlanes(axis,0.f,.6f,30.f);}));
    assert(Reject([&]{BuildBridgePlanes(axis,11.f,0.f,30.f);}));
    assert(Reject([&]{BuildBridgePlanes(axis,11.f,.6f,NAN);}));
    assert(Reject([&]{BuildBridgePlanes({axis[0],axis[0]},11.f,.6f,30.f);}));
    assert(Reject([&]{BuildBridgePlanes({axis[0],axis[2],axis[1]},11.f,.6f,30.f);}));
    assert(Reject([&]{BuildBridgePlanes({axis[0],{5.f,15.f,75.f},axis.back()},
                                      11.f,.6f,30.f);}));
    assert(Reject([&]{BuildBridgePlanes({axis[0],{0.f,150.f,30.f}},11.f,.6f,30.f);}));
    return 0;
}
