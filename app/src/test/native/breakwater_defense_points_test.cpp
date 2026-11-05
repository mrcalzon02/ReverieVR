#include "breakwater_defense_points.h"
#include "breakwater_terrain.h"
#include "breakwater_cannon_trajectory.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace reverie::breakwater;
int main(){
    using namespace defense;
    for(uint8_t map=0;map<Campaign::kLocations;++map) {
        assert(At(map,0).x==0.f);
        assert(At(map,0).y==0.f);
        assert(At(map,0).z==0.f);
        assert(At(map,1).x<-4.f && At(map,2).x>4.f);
        assert(At(map,1).y>0.f && At(map,2).y>0.f);
        // The central *combat* floor retains its established height; flanking
        // beach profiles must be real per-theater geometry, not recoloring.
        assert(BeachHeight(map,0.f,-1.40f)>-1.055f);
        assert(BeachHeight(map,0.f,-1.40f)<-1.020f);
        const float origin[]={0.f,0.f,0.f};
        for(int side=1;side<=2;++side) {
            const Position point=At(map,side);
            float target[3];Marker(point,target);
            const float dir[]={target[0],target[1],target[2]};
            assert(Hit(map,0,origin,dir)==side);
            float shifted[3];ShiftOrigin(shifted,origin,point);
            for(int k=0;k<3;++k)assert(shifted[k]==(k==0?point.x:k==1?point.y:point.z));
            const float v[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
            float moved[16];ShiftEyeView(v,point,moved);
            assert(moved[12]==-point.x && moved[13]==-point.y &&
                   moved[14]==-point.z && moved[15]==1.f);
            // A rotated tracked eye must retain rotation and stereo offset;
            // the source view is never mutated and the station translation
            // is always composed in that view's own coordinate basis.
            const float yaw[16]={0,0,-1,0,0,1,0,0,1,0,0,0,
                                 .03f,-.02f,.01f,1};
            float shiftedView[16];
            ShiftEyeView(yaw,point,shiftedView);
            assert(shiftedView[0]==yaw[0] && shiftedView[8]==yaw[8]);
            assert(std::abs(shiftedView[12]-(yaw[12]-point.z))<.0001f);
            assert(std::abs(shiftedView[13]-(yaw[13]-point.y))<.0001f);
            assert(std::abs(shiftedView[14]-(yaw[14]+point.x))<.0001f);
            float center[3];Marker(At(map,0),center);
            const float look[]={center[0]-shifted[0],
                                center[1]-shifted[1],
                                center[2]-shifted[2]};
            assert(Hit(map,side,shifted,look)==0);
        }
        int boxes=0,triangles=0,cladding=0;
        auto box=[&](auto...f){const float values[]={f...};
            assert(sizeof...(f)==9);
            for(float v:values)assert(std::isfinite(v));
            ++boxes;};
        auto tri=[&](auto...f){const float values[]={f...};
            assert(sizeof...(f)==12);
            for(float v:values)assert(std::isfinite(v));
            ++triangles;};
        auto clad=[&](materials::Surface s,float x,float y,float z,float w,float h,float d){
            std::vector<float> mesh;
            assert(materials::AppendBoxCladding(mesh,s,x,y,z,w,h,d));
            ++cladding;};
        EmitPlatforms(map,0,2,box,tri,clad);
        assert(boxes==12 && triangles==4 && cladding==8);
    }
    // Distinct land approaches: all retain the same tested central infantry
    // strip, but the outer shoulder height, stepped quay, basalt and concrete
    // terrain actually have different world-space elevations.
    const float coast0=BeachHeight(0,3.83f,-1.35f);
    const float coast1=BeachHeight(1,3.83f,-1.35f);
    const float coast2=BeachHeight(2,3.83f,-1.35f);
    const float coast3=BeachHeight(3,3.83f,-1.35f);
    assert(std::abs(coast0-coast1)>.03f);
    assert(std::abs(coast2-coast1)>.10f);
    assert(std::abs(coast3-coast1)>.08f);
    assert(At(0,1).x!=At(1,1).x &&
           At(1,1).x!=At(2,1).x &&
           At(2,1).x!=At(3,1).x);
    // The raised platform's actual muzzle and local infantry convergence
    // must not silently keep using the original center gun ray.
    const float downward[]={0.f,-.40f,-1.f};
    trajectory::ShotRay centerRay{},raisedRay{};
    assert(trajectory::CannonShot(WeaponKind::MachineGun,
                                  downward,&centerRay));
    assert(trajectory::CannonShot(WeaponKind::MachineGun,
                                  downward,&raisedRay,.62f));
    assert(std::abs(centerRay.direction[1]-raisedRay.direction[1])>.005f);
    float worldMuzzle[3]={};
    const auto station=At(3,2);
    defense::ShiftOrigin(worldMuzzle,raisedRay.origin,station);
    assert(std::abs(worldMuzzle[0]-station.x)<.0001f);
    assert(std::abs(worldMuzzle[1]-raisedRay.origin[1]-station.y)<.0001f);
    assert(std::abs(worldMuzzle[2]-raisedRay.origin[2]-station.z)<.0001f);
    assert(!Valid(At(Campaign::kLocations,0)));
    assert(!Valid(At(0,-1)));
    float bad[]={NAN,0,0},dir[]={1,0,0};
    assert(Hit(0,0,bad,dir)==-1);
    Selection selected;
    assert(selected.Press(1)==Tap::Armed && selected.current==0);
    selected.Tick(.12f);
    assert(selected.Press(1)==Tap::Transferred && selected.current==1);
    assert(selected.Press(1)==Tap::Ignored);
    assert(selected.Press(2)==Tap::Armed);
    selected.Tick(.46f);
    assert(selected.Press(2)==Tap::Armed && selected.current==1);
    selected.LostFocus();
    assert(selected.Press(2)==Tap::Armed);
    selected.Tick(.2f);
    assert(selected.Press(2)==Tap::Transferred && selected.current==2);
    std::cout<<"PASS: 4 distinct coasts, 3 ray-picked defense points each, double-click gating and 3D pads\n";
}
