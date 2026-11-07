#include "breakwater_trajectory_guide.h"
#include <cassert>
#include <iostream>
using namespace reverie::breakwater;
int main() {
    trajectory::ShotRay ray{};ray.origin[1]=-.53f;ray.origin[2]=-.77f;
    ray.direction[1]=-.08f;ray.direction[2]=-.9967949f;
    for(int station=0;station<3;++station) {
        const auto viewer=defense::At(0,station);
        size_t count=0;float minY=0.f,minZ=0.f;
        const int triangles=guide::Emit(ray,battlefield::Gravity(.014f),180.f,true,viewer,
            [&](auto... values){const float v[]={values...};
                for(float f:v)assert(std::isfinite(f));
                for(int i=0;i<9;i+=3){minY=std::min(minY,v[i+1]);minZ=std::min(minZ,v[i+2]);}
                ++count;
            });
        assert(triangles==int(count) && count==guide::kDashes*12+8);
        assert(minY< -14.f && minZ< -179.f);
    }
    assert(guide::HalfWidth(500.f)>=.85f);
    assert(guide::HalfWidth(2.f)>=.025f);
    assert(guide::HalfWidth(2400.f)<=3.2f);
    size_t emitted=0;
    assert(guide::Emit(ray,0.f,NAN,false,defense::At(0,0),[&](auto...){++emitted;})==0);
    assert(emitted==0);
    // Real drop must put the endpoint exactly on the projected parabola.
    float end[3];assert(battlefield::Position(ray.origin,ray.direction,180.f,
        battlefield::Gravity(.014f),end));assert(end[1]<ray.origin[1]+180.f*ray.direction[1]);
    std::cout<<"PASS visible bounded 3D ballistic guide, contact marker and distance-scaled width\n";
}
