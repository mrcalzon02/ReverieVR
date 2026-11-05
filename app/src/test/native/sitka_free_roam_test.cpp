#include "sitka_free_roam.h"
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <vector>
using namespace reverie::sitka;

static TerrainMesh Terrain(){
    TerrainMesh mesh;
    mesh.vertices={
        {-20.f,0.f,-20.f,80,90,80},
        { 20.f,0.f,-20.f,80,90,80},
        { 20.f,0.f, 20.f,80,90,80},
        {-20.f,0.f, 20.f,80,90,80}
    };
    mesh.indices={0,1,2,0,2,3};
    return mesh;
}
static bool Near(double a,double b,double margin=.015){
    return std::abs(a-b)<margin;
}
int main(){
    const auto mesh=Terrain();
    const float right[3]={1.f,0.f,0.f};
    std::vector<Water> none;
    std::vector<ShoreFeature> no_buildings;
    FreeRoamSession walking(mesh,none,no_buildings,true,0.,0.);
    assert(!walking.Advance(0.f,0.f,right,.05f));
    assert(!walking.Advance(.28f,0.f,right,.05f));
    assert(Near(walking.pose().x,0.));
    for(int n=0;n<20;++n)
        assert(walking.Advance(.64f,0.f,right,.05f));
    assert(Near(walking.pose().x,1.4,.025));
    FreeRoamSession fast(mesh,none,no_buildings,true,0.,0.);
    for(int n=0;n<20;++n)
        assert(fast.Advance(1.f,0.f,right,.05f));
    assert(Near(fast.pose().x,2.8));
    assert(Near(fast.pose().z,0.));
    // Diagonal deflection is normalized and cannot exceed full stick speed.
    FreeRoamSession diagonal(mesh,none,no_buildings,true,0.,0.);
    for(int n=0;n<20;++n)
        assert(diagonal.Advance(1.f,1.f,right,.05f));
    assert(Near(std::hypot(diagonal.pose().x,diagonal.pose().z),2.8));
    assert(diagonal.pose().x>0.&&diagonal.pose().z<0.);
    const auto at_rest=fast.pose();
    assert(!fast.Advance(0.f,0.f,right,.05f));
    assert(Near(fast.pose().x,at_rest.x));
    assert(!fast.Advance(NAN,1.f,right,.05f));
    assert(!fast.Advance(1.f,0.f,right,INFINITY));
    // Headset eye transforms are copies, never moved once per eye.
    ReverieNativeEyeV1 eye{};
    eye.struct_size=sizeof(eye);eye.eye_index=0u;
    for(int i=0;i<16;i+=5){eye.view[i]=1.f;eye.projection[i]=1.f;}
    ReverieNativeEyeV1 shifted{};
    assert(fast.ShiftEye(&eye,shifted));
    assert(Near(shifted.view[12],-fast.pose().x));
    assert(Near(eye.view[12],0.));
    eye.eye_index=1u;
    assert(fast.ShiftEye(&eye,shifted));
    eye.eye_index=2u;
    assert(!fast.ShiftEye(&eye,shifted));

    // The old pedestrian-path graph is not required for a terrain walk.
    // A real water footprint stops feet crossing below sea level.
    Water sea;sea.id="sea";sea.shape="SQUARE";
    sea.x=9.f;sea.z=0.f;sea.width=4.f;sea.depth=30.f;sea.y=1.f;
    std::vector<Water> coast{sea};
    FreeRoamSession shore(mesh,coast,no_buildings,true,0.,0.);
    for(int n=0;n<140;++n)shore.Advance(1.f,0.f,right,.05f);
    assert(shore.pose().x>=6.5f&&shore.pose().x<7.1f);
    assert(shore.blocked_steps()>0u);
    assert(shore.Advance(0.f,1.f,right,.05f)); // free movement along beach
    assert(shore.pose().z<0.f);
    assert(!shore.Advance(1.f,0.f,right,99.f)||
           shore.pose().x<7.1f); // large frame does not teleport

    // Existing source footprint, not an invisible pedestrian corridor,
    // blocks walking THROUGH a building; walk around it instead.
    ShoreFeature building;
    building.id="source_house";building.material="storefront";
    building.polygon=true;building.base=0.f;building.height=5.f;
    building.footprint={{3.f,-1.f},{5.f,-1.f},{5.f,1.f},{3.f,1.f}};
    std::vector<ShoreFeature> blocks{building};
    FreeRoamSession city(mesh,none,blocks,true,0.,0.);
    for(int n=0;n<70;++n)city.Advance(1.f,0.f,right,.05f);
    assert(city.pose().x<2.77f);
    assert(city.blocked_steps()>0u);
    for(int n=0;n<22;++n)city.Advance(0.f,1.f,right,.05f);
    assert(city.pose().z< -2.f);
    for(int n=0;n<30;++n)city.Advance(1.f,0.f,right,.05f);
    assert(city.pose().x>3.5f);
    // Missing geography cannot be transformed into a phantom walkable area.
    bool rejected=false;
    try{
        auto invalid=Terrain();
        invalid.indices.clear();
        FreeRoamSession broken(invalid,none,no_buildings);
        (void)broken;
    }catch(const std::invalid_argument&){rejected=true;}
    assert(rejected);
    return 0;
}
