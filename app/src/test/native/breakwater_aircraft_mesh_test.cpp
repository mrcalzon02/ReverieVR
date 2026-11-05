#include "breakwater_aircraft_mesh.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace reverie::breakwater;
static std::vector<float> Geometry(Enemy enemy) {
    std::vector<float> out;
    EmitAircraftMesh(enemy,[&](auto... args) {
        static_assert(sizeof...(args)==12,"XYZ RGB aircraft triangles");
        const float v[]={args...};
        for(float f:v)assert(std::isfinite(f));
        for(size_t j=9;j<12;++j)assert(v[j]>=0.f&&v[j]<=1.f);
        for(size_t j=0;j<9;j+=3) {
            assert(std::abs(v[j]-enemy.x)<1.6f);
            assert(std::abs(v[j+1]-1.6f)<.9f);
            assert(std::abs(v[j+2]-enemy.z)<1.4f);
        }
        out.insert(out.end(),v,v+12);
    });
    return out;
}
int main() {
    Enemy e{};e.active=true;e.hp=100;e.type=UnitType::Aircraft;
    e.aircraft_pattern=AircraftPattern::Bomber;e.x=.8f;e.z=-6.f;
    const auto bomber=Geometry(e);
    assert(bomber.size()>=45u*12u && bomber.size()<140u*12u);
    assert(Geometry(e)==bomber);
    e.aircraft_pattern=AircraftPattern::Strafer;
    const auto strafer=Geometry(e);
    assert(strafer.size()>=35u*12u && strafer.size()<bomber.size());
    assert(strafer!=bomber);
    e.aircraft_pattern=AircraftPattern::Bomber;e.bank=.22f;
    const auto banked=Geometry(e);
    assert(banked.size()==bomber.size()&&banked!=bomber);
    for(size_t j=0;j<bomber.size();j+=12u)
        for(size_t k=2;k<9;k+=3)assert(bomber[j+k]==banked[j+k]);
    e.active=false;assert(Geometry(e).empty());
    e.active=true;e.type=UnitType::Gunboat;assert(Geometry(e).empty());
    e.type=UnitType::Aircraft;e.aircraft_pattern=AircraftPattern::None;
    assert(Geometry(e).empty());
    e.aircraft_pattern=AircraftPattern::Bomber;e.bank=std::nanf("");
    assert(Geometry(e).empty());
    std::cout<<"PASS: distinct volumetric bomber/strafer with real bank\n";
}
