#include "sitka_bridge_layout.h"
#include <cassert>
#include <stdexcept>
#include <string>
using namespace reverie::sitka;

static const std::string kLayout=
    "SITKA-BRIDGE-1\n"
    "site_id=oconnell_bridge\n"
    "geometry_reviewed=true\n"
    "coordinate_system=EPSG:32608\n"
    "source_feature_id=osm:way:12345\n"
    "source_sha256=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "origin_east=480001.5\n"
    "origin_north=6299911.5\n"
    "width_m=11.5824\n"
    "deck_thickness_m=.6\n"
    "side_height_m=30.48\n"
    "axis_count=3\n"
    "axis.0=2,15,3\n"
    "axis.1=2,16,63\n"
    "axis.2=2,15,123\n";
template<class F>bool Throws(F f){
    try{f();return false;}catch(const std::invalid_argument&){return true;}
}
int main(){
    auto m=ParseBridgeLayout(kLayout,480001.5,6299911.5);
    assert(m.planes.size()==6u);
    assert(m.Count(BridgePlaneMaterial::SideSilhouette)==2u);
    assert(m.Count(BridgePlaneMaterial::RoadConcrete)==2u);
    assert(m.Count(BridgePlaneMaterial::UnderdeckConcrete)==2u);
    assert(Throws([]{ParseBridgeLayout(kLayout,480001.5019,6299911.5);}));
    assert(Throws([]{ParseBridgeLayout(
        "SITKA-BRIDGE-1\nsite_id=oconnell_bridge\n",480001.5,6299911.5);}));
    auto wrong=kLayout;
    wrong.replace(wrong.find("true"),4,"false");
    assert(Throws([&]{ParseBridgeLayout(wrong,480001.5,6299911.5);}));
    wrong=kLayout;
    wrong.replace(wrong.find("osm:way:12345"),13,"unverified/path");
    assert(Throws([&]{ParseBridgeLayout(wrong,480001.5,6299911.5);}));
    wrong=kLayout;
    wrong+="axis.0=1,1,1\n";
    assert(Throws([&]{ParseBridgeLayout(wrong,480001.5,6299911.5);}));
    wrong=kLayout;
    wrong+="surprise=value\n";
    assert(Throws([&]{ParseBridgeLayout(wrong,480001.5,6299911.5);}));
    return 0;
}
