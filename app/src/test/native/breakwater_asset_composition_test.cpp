#include "../../main/jni/native/breakwater_asset_composition.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <vector>
#include <fstream>
#include <iterator>
#include <string>
using namespace reverie::breakwater;
int main(){
    static_assert(dressing::kEmplacements.size()==11u);
    // Each test triangle passes the actual BWM1 parser and stores three
    // original-format signed positions plus their RGB values.
    std::vector<uint8_t> bytes(8u+27u,0u);
    bytes[0]='B';bytes[1]='W';bytes[2]='M';bytes[3]='1';bytes[4]=1;
    std::array<imported::Model,imported::kAssetPaths.size()> models{};
    for(auto &mesh:models) assert(mesh.Parse(bytes.data(),bytes.size()));
    for(const auto &p:dressing::kEmplacements) {
        assert(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z));
        assert(std::isfinite(p.scale)&&p.scale>0.f);
        assert(std::fabs(p.x)>=2.5f); // No fake cover crossing the gun sight
        assert(p.mesh<=imported::AmmoBox);
    }
    size_t drawn=0u;
    const auto sink=[&drawn](auto...){++drawn;};
    assert(dressing::Emit(models,false,4.f,-4.f,sink)==11u);
    assert(drawn==11u);
    drawn=0u;
    assert(dressing::Emit(models,true,4.f,-4.f,sink)==12u);
    assert(drawn==12u);
    models[imported::AmmoBox]=imported::Model{};
    drawn=0u;
    assert(dressing::Emit(models,true,4.f,-4.f,sink)==9u);
    assert(drawn==9u);
    // Full-world scene acceptance uses the THREE ACTUAL committed mesh blobs,
    // not just synthetic one-triangle fixtures. Tests run at repo root.
    std::array<imported::Model,imported::kAssetPaths.size()> authentic{};
    for(size_t i=0;i<authentic.size();++i){
        std::ifstream file(std::string("app/src/main/assets/")+
                           imported::kAssetPaths[i],std::ios::binary);
        assert(file.is_open());
        const std::vector<uint8_t> source{std::istreambuf_iterator<char>(file),
                                          std::istreambuf_iterator<char>()};
        assert(authentic[i].Parse(source.data(),source.size()));
    }
    assert(authentic[imported::Concrete].triangles==100u);
    assert(authentic[imported::WoodenCrate].triangles==184u);
    assert(authentic[imported::AmmoBox].triangles==96u);
    bool finite=true;
    drawn=0u;
    const auto real=[&](auto... vertices){
        ++drawn;
        const std::array<float,sizeof...(vertices)> numbers{{
            static_cast<float>(vertices)...
        }};
        for(float value:numbers)if(!std::isfinite(value))finite=false;
    };
    assert(dressing::Emit(authentic,false,4.f,-4.f,real)==1344u);
    assert(drawn==1344u&&finite);
    drawn=0u;
    assert(dressing::Emit(authentic,true,4.f,-4.f,real)==1440u);
    assert(drawn==1440u&&finite);
}
