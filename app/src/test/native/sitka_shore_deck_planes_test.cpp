#include "sitka_shore_deck_planes.h"
#include <cassert>
#include <cmath>
#include <stdexcept>
using namespace reverie::sitka;
int main(){
    auto f=ParseShoreLayout(
        "SITKA-SHORE-1\n"
        "P|float1|pontoon|-1|0.2|0,0;9,0;9,2;6,2;6,5;0,5\n"
        "P|float2|pier|-1|0.4|15,0;19,0;19,8;15,8\n"
        "P|frontage|storefront|0|7|30,0;40,0;40,10;30,10\n"
        "B|old_pile|pile|5|10|-2|.5|.5|5|0\n");
    const auto mesh=BuildShoreDeckPlanes(f);
    assert(mesh.by_feature.size()==4u);
    // Only sourced float polygons get decks: no fake rectangular backfill.
    assert(mesh.by_feature[0].count==12u); // 6 points -> 4 triangles
    assert(mesh.by_feature[1].count==6u);  // 4 points -> 2 triangles
    assert(mesh.by_feature[2].count==0u);
    assert(mesh.by_feature[3].count==0u);
    assert(mesh.vertices.size()==18u&&mesh.triangles()==6u);
    for(size_t i=0;i<mesh.by_feature[0].count;++i){
        const auto& v=mesh.vertices[i];
        assert(std::abs(v.y-(-.79f))<.0001f);
        assert(std::isfinite(v.u)&&std::isfinite(v.v));
    }
    bool nonzero_uv=false;
    for(const auto& v:mesh.vertices)
        nonzero_uv|=std::abs(v.u)>0.01||std::abs(v.v)>0.01;
    assert(nonzero_uv);
    const auto repeat=BuildShoreDeckPlanes(f);
    for(size_t i=0;i<mesh.vertices.size();++i){
        const auto& a=mesh.vertices[i],&b=repeat.vertices[i];
        assert(a.x==b.x&&a.y==b.y&&a.z==b.z&&a.u==b.u&&a.v==b.v);
    }
    ShoreFeature bad=f[0];
    bad.footprint[1].x=0.f;
    bad.footprint[1].z=0.f;
    bool rejected=false;
    try{BuildShoreDeckPlanes({bad});}
    catch(const std::invalid_argument&){rejected=true;}
    assert(rejected);
    return 0;
}
