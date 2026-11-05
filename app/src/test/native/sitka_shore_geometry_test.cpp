#include "sitka_shore_geometry.h"
#include <cassert>
#include <stdexcept>
#include <string>
using namespace reverie::sitka;
static bool Reject(const std::string& text){
    try{(void)ParseShoreLayout(text);return false;}
    catch(const std::invalid_argument&){return true;}
}
int main(){
    const std::string source=
        "SITKA-SHORE-1\n"
        "P|lincoln_1|storefront|1.2|5.0|0,0;9,0;9,3;5,3;5,6;0,6\n"
        "P|katlian_1|cannery|0.8|6|20,0;34,0;34,8;20,8\n"
        "P|float_source|pontoon|-0.8|0.5|0,30;11,30;11,34;7,35;0,35\n"
        "B|crescent_pier|pier|5|19|0.9|2|16|0.45|0.0\n"
        "B|crescent_pile|pile|4|25|-3|0.3|0.3|4|0\n"
        "R|oconnell_ramp|bridge_ramp|40|0|5|8|18|1.2|0|4.0\n";
    auto features=ParseShoreLayout(source);
    assert(features.size()==6u && features[0].footprint.size()==6u);
    assert(features[2].polygon && features[2].material=="pontoon");
    assert(features[2].footprint.size()==5u);
    TerrainMesh terrain;
    terrain.vertices={{-50.f,0.f,-50.f,55,55,55},
                      {-45.f,0.f,-50.f,55,55,55},
                      {-50.f,0.f,-45.f,55,55,55}};
    terrain.indices={0,1,2};
    AppendShoreGeometry(terrain,features);
    assert(terrain.vertices.size()>3u && terrain.indices.size()>3u);
    assert(terrain.indices.size()%3u==0);
    auto batch=PackTerrainGles2(terrain);
    assert(!batch.empty());
    assert(Reject("SITKA-SHORE-2\n"));
    assert(Reject("SITKA-SHORE-1\nP|nope|pontoon|0|.5|0,0;4,4;0,4;4,0\n"));
    assert(Reject("SITKA-SHORE-1\nB|bad|house|0|0|0|5|5|5|0\n"));
    assert(Reject("SITKA-SHORE-1\nP|bad|cannery|0|3|0,0;3,3;0,3;3,0\n"));
    assert(Reject("SITKA-SHORE-1\nB|dup|rock|0|0|0|1|1|1|0\nB|dup|rock|1|0|0|1|1|1|0\n"));
    assert(Reject("SITKA-SHORE-1\nB|bad|pier|nan|0|0|2|2|1|0\n"));
    assert(Reject("SITKA-SHORE-1\nR|r|bridge_ramp|0|0|0|2|5|1|0|-2\n"));
    return 0;
}