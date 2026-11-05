#include "sitka_walk_graph.h"
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
using namespace reverie::sitka;
static const std::string kSource=
"SITKA-WALK-1\n"
"P|osm_sitka|aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa|EPSG32608|reviewed\n"
"O|480000.125|6300000.625\n"
"N|lincoln|0|4|0\n"
"N|cathedral|20|4|0\n"
"N|sea_walk|35|4|0\n"
"N|harbor_approach|35|4|25\n"
"E|lincoln_section|lincoln|cathedral|sidewalk|2.4\n"
"E|seawalk_link|cathedral|sea_walk|seawalk|2.2\n"
"E|harbor_link|sea_walk|harbor_approach|shore_path|1.7\n"
"S|start|lincoln\n"
"F|legal_bank|harbor_approach|approved_fish_site|1.4\n";
template<typename F> bool Invalid(F&& f){
    try{f();return false;}catch(const std::invalid_argument&){return true;}
}
int main(){
    const auto g=WalkGraph::Parse(kSource,480000.125,6300000.625);
    assert(g.nodes.size()==4u&&g.edges.size()==3u&&g.stances.size()==1u);
    assert(g.reachable[3u]&&g.spawn==0u);
    const auto route=g.Route("lincoln","harbor_approach");
    assert((route==std::vector<size_t>{0u,1u,2u,3u}));
    assert(g.Route("lincoln","unknown").empty());
    assert(g.Route("harbor_approach","lincoln").size()==4u);
    const auto p=g.Surface(10.,0.);
    assert(p.valid&&p.edge==0u&&std::abs(p.y-4.f)<.00001f);
    assert(!g.Surface(10.,6.).valid);
    assert(g.Surface(35.,24.).valid);
    assert(!g.Surface(NAN,3.).valid);
    assert(!g.Surface(90.,90.).valid);
    const auto mesh=g.BuildPathMesh();
    assert(mesh.vertices.size()==12u&&mesh.indices.size()==18u);
    const auto packed=PackTerrainGles2(mesh);
    assert(!packed.empty());
    const std::vector<FishingSpot> spots={
        {"approved_fish_site","public_water",40.f,25.f,6.f,8.f},
        {"elsewhere","public_water",40.f,25.f,6.f,8.f}};
    assert(g.CanCastFrom(35.,25.,40.f,25.f,spots));
    assert(!g.CanCastFrom(31.,25.,40.f,25.f,spots)); // not at authored stance
    assert(!g.CanCastFrom(35.,25.,70.f,25.f,spots));
    assert(!g.CanCastFrom(35.,25.,40.f,25.f,{{"elsewhere","water",40,25,6,8}}));
    assert(Invalid([]{WalkGraph::Parse(kSource,480000.125,6300000.6262);}));
    assert(Invalid([]{WalkGraph::Parse("SITKA-WALK-1\n",480000.125,6300000.625);}));
    auto missing=kSource;
    const auto approved=missing.find("reviewed");
    missing.replace(approved,8,"unproved");
    assert(Invalid([&]{WalkGraph::Parse(missing,480000.125,6300000.625);}));
    missing=kSource;
    const auto pos=missing.find("harbor_approach|shore_path");
    assert(pos!=std::string::npos);
    missing.replace(pos,15,"private_property"); // unknown node
    assert(Invalid([&]{WalkGraph::Parse(missing,480000.125,6300000.625);}));
    missing=kSource;
    missing+="E|fake_shortcut|lincoln|harbor_approach|sidewalk|2\n";
    assert(!Invalid([&]{WalkGraph::Parse(missing,480000.125,6300000.625);}));
    // Runtime checks route topology but source/license review is authoring
    // responsibility: don't synthesize edges just because points are close.
    auto disconnected=kSource;
    disconnected+="N|island|200|4|200\n";
    disconnected+="N|island2|210|4|200\n";
    disconnected+="E|water_gap|island|island2|shore_path|1\n";
    disconnected+="F|fake_bank|island2|approved_fish_site|1\n";
    assert(Invalid([&]{WalkGraph::Parse(disconnected,480000.125,6300000.625);}));
    // Exact, stable geometry across repeated parses and simulated context loss.
    const auto repeat=WalkGraph::Parse(kSource,480000.125,6300000.625).BuildPathMesh();
    assert(repeat.indices==mesh.indices);
    for(size_t i=0;i<mesh.vertices.size();++i){
        const auto& a=mesh.vertices[i];const auto& b=repeat.vertices[i];
        assert(a.x==b.x&&a.y==b.y&&a.z==b.z);
        assert(a.r==b.r&&a.g==b.g&&a.b==b.b);
    }
    return 0;
}
