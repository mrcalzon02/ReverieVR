#include "sitka_boat_mesh.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace reverie::sitka;
static bool near(double a,double b){return std::abs(a-b)<1.e-5;}
int main(){
    HelmProgress progress;
    const auto first=BuildBoatMesh(progress);
    const auto again=BuildBoatMesh(progress);
    assert(!first.indices.empty()&&first.vertices.size()<4096u);
    assert(first.indices==again.indices);
    assert(first.vertices.size()==again.vertices.size());
    assert(PackTerrainGles2(first).size()==1u);
    auto mounted=AssembleHelm(progress);
    assert(mounted.count==3u);
    progress.owned=HelmMask(HelmItem::Battery)|HelmMask(HelmItem::Sonar);
    const auto electronics=BuildBoatMesh(progress);
    assert(electronics.vertices.size()>first.vertices.size());
    assert(PackTerrainGles2(electronics).size()==1u);
    progress.hull=1u;
    progress.owned|=HelmMask(HelmItem::Roof)|HelmMask(HelmItem::Radar);
    const auto skiff=BuildBoatMesh(progress);
    assert(skiff.vertices.size()>electronics.vertices.size());
    DeckPassenger walker;
    assert(walker.Walk(.7,-1.3,kDecks[1]));
    progress.hull=2u;
    progress.owned|=HelmMask(HelmItem::RodRacks)|
                    HelmMask(HelmItem::Outriggers);
    const auto pilot=BuildBoatMesh(progress);
    assert(!pilot.indices.empty());
    progress.hull=3u;
    progress.owned=65535u;
    const auto trawler=BuildBoatMesh(progress);
    assert(trawler.vertices.size()>skiff.vertices.size());
    assert(PackTerrainGles2(trawler).size()==1u);
    // All geometry must be valid, indexed and bounded for GLES2.
    for(const auto* mesh:{&first,&electronics,&skiff,&pilot,&trawler}){
        assert(mesh->indices.size()%3u==0u);
        for(uint32_t index:mesh->indices)assert(index<mesh->vertices.size());
        for(const auto& v:mesh->vertices)
            assert(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z));
    }
    BoatPose pose;
    pose.launched=true;pose.east=430010.;pose.north=6379990.;
    pose.heading=1.5707963267948966;
    const auto model=BoatModelMatrix(pose,430000.,6380000.,0.);
    assert(near(model[12],10.)&&near(model[13],.4)&&near(model[14],-10.));
    assert(near(model[0],0.)&&near(model[8],1.)&&near(model[2],-1.));
    const float x=2.f,z=3.f;
    const float moved_x=model[0]*x+model[8]*z+model[12];
    const float moved_z=model[2]*x+model[10]*z+model[14];
    assert(near(moved_x,13.)&&near(moved_z,-12.));
    bool rejected=false;
    try{BoatModelMatrix(BoatPose{},0.,0.,0.);}catch(const std::invalid_argument&){rejected=true;}
    assert(rejected);
    rejected=false;
    try{HelmProgress bad;bad.hull=5u;BuildBoatMesh(bad);}
    catch(const std::invalid_argument&){rejected=true;}
    assert(rejected);
    std::cout<<"PASS Sitka authored modular boat mesh, GLES2 batches, hull growth and boat-local transform\n";
}
