#include "breakwater_gun_mesh.h"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <vector>
using namespace reverie::breakwater;
struct Geometry {
    std::vector<float> numbers;
    float min_x=999.f,max_x=-999.f,min_y=999.f,max_y=-999.f,
          min_z=999.f,max_z=-999.f;
    bool all_valid=true;
    void operator()(float x0,float y0,float z0,float x1,float y1,float z1,
                    float x2,float y2,float z2,float r,float g,float b){
        const float values[]={x0,y0,z0,x1,y1,z1,x2,y2,z2,r,g,b};
        for(float value:values){if(!std::isfinite(value))all_valid=false;
            numbers.push_back(value);}
        const float x[]={x0,x1,x2},y[]={y0,y1,y2},z[]={z0,z1,z2};
        for(int i=0;i<3;++i){
            min_x=std::min(min_x,x[i]);max_x=std::max(max_x,x[i]);
            min_y=std::min(min_y,y[i]);max_y=std::max(max_y,y[i]);
            min_z=std::min(min_z,z[i]);max_z=std::max(max_z,z[i]);
        }
    }
};
int main(){
    const float forwards[3]={0.f,0.f,-1.f};
    for(int weapon=0;weapon<kWeaponCount;++weapon){
        Geometry mesh;
        gunmesh::Build([&](auto... v){mesh(v...);},
                        static_cast<WeaponKind>(weapon),forwards);
        assert(mesh.all_valid && mesh.numbers.size()%12u==0u);
        assert(mesh.numbers.size()/12u>240u);
        assert(mesh.numbers.size()/12u<700u);
        assert(mesh.min_z< -1.90f && mesh.max_z< -.54f);
        assert(mesh.min_x< -.50f && mesh.max_x>.50f); // 3D wheels and shield
        assert(mesh.min_y< -1.20f && mesh.max_y> -.40f);
        Geometry replay;
        gunmesh::Build([&](auto... v){replay(v...);},
                       static_cast<WeaponKind>(weapon),forwards);
        assert(replay.numbers==mesh.numbers);
    }
    const float aimed[3]={.50f,.27f,-.82f};
    Geometry aimedMesh;
    gunmesh::Build([&](auto... v){aimedMesh(v...);},
                    WeaponKind::DirectCannon,aimed);
    assert(aimedMesh.all_valid && aimedMesh.max_x>.7f);
    assert(aimedMesh.max_y> -.20f);
    const float nonfinite[3]={NAN,0.f,-1.f};
    Geometry rejected;
    gunmesh::Build([&](auto... v){rejected(v...);},
                    WeaponKind::MachineGun,nonfinite);
    assert(rejected.numbers.empty());
    std::cout<<"Breakwater 3D cannon: 5 mounted shapes, directional elevation, "
             <<"finite deterministic solid mesh passed\n";
}
