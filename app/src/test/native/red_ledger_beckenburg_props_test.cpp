#include "../../main/jni/native/red_ledger_beckenburg_props.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>
int main(int argc,char **argv) {
    assert(argc==2);
    using namespace reverie::redledger::beckenburg;
    assert(kCount==14&&kPlacementCount==25);
    std::set<int> used;
    for(const Placement &p:kPlacements) {
        assert(p.asset>=0&&p.asset<kCount&&p.room>=0&&p.room<=2);
        assert(p.quarter_turns>=0&&p.quarter_turns<4);
        assert(p.sx>0&&p.sy>0&&p.sz>0);
        assert(p.min_tier>=0&&p.max_tier<=5&&p.min_tier<=p.max_tier);
        float model[16]; MakePlacementTransform(p,model);
        for(float v:model)assert(std::isfinite(v));
        assert(model[12]==p.x&&model[13]==p.y&&model[14]==p.z);
        used.insert(p.asset);
    }
    assert(used.size()==kCount);
    assert(!Visible(kPlacements[5],0,0,0,0,0)); // upgraded floor
    assert(Visible(kPlacements[5],0,0,1,0,0));
    assert(Visible(kPlacements[9],0,0,0,0,0)); // old lamp
    assert(!Visible(kPlacements[9],0,0,0,1,0));
    assert(Visible(kPlacements[10],0,0,0,1,0)); // new lamp
    assert(!Visible(kPlacements[10],0,0,0,0,0));
    assert(!Visible(kPlacements[20],2,5,5,5,5));
    for(int i=0;i<kCount;++i) {
        const std::string path=kAssets[i].path;
        const std::string name=path.substr(path.find_last_of('/')+1);
        std::ifstream stream(std::string(argv[1])+"/"+name,std::ios::binary);
        assert(stream.good());
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(stream)),{});
        std::vector<float> vertices;
        const uint8_t *pixels=nullptr;
        assert(Decode(data.data(),data.size(),&vertices,&pixels));
        assert(vertices.size()%kStride==0u);
        assert(static_cast<size_t>(pixels-data.data())+kTextureBytes==data.size());
        assert(!Decode(data.data(),data.size()-1u,&vertices,&pixels));
        data[0]=0;
        assert(!Decode(data.data(),data.size(),&vertices,&pixels));
    }
    return 0;
}
