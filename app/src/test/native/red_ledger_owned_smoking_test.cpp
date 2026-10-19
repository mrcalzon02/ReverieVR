#include "../../main/jni/native/red_ledger_owned_smoking.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
using namespace reverie::redledger::smoking;
int main(int argc,char **argv) {
    assert(argc==2);
    for(size_t i=0;i<kCount;++i) {
        const std::string path=std::string(argv[1])+"/app/src/main/assets/"+
                               kModels[i].path;
        std::ifstream stream(path,std::ios::binary);
        assert(stream.good());
        const std::vector<uint8_t> raw{
            std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>()};
        std::vector<float> vertices;
        assert(Build(raw.data(),raw.size(),static_cast<ModelIndex>(i),&vertices));
        assert(!vertices.empty()&&vertices.size()%kStride==0u);
        assert(vertices.size()/kStride<=6000u);
        float lo[3]={1e6f,1e6f,1e6f},hi[3]={-1e6f,-1e6f,-1e6f};
        for(size_t n=0;n<vertices.size();n+=kStride) {
            for(int j=0;j<3;++j) {
                const float v=vertices[n+j];
                assert(std::isfinite(v));
                lo[j]=std::min(lo[j],v);
                hi[j]=std::max(hi[j],v);
            }
            for(size_t j=3;j<8;++j)
                assert(std::isfinite(vertices[n+j]) &&
                       vertices[n+j]>=0.f && vertices[n+j]<=1.f);
        }
        for(int axis=0;axis<3;++axis)
            assert(hi[axis]-lo[axis]>kModels[i].size[axis]*.95f &&
                   hi[axis]-lo[axis]<kModels[i].size[axis]*1.05f);
        assert(!Build(raw.data(),raw.size()-3u,
                      static_cast<ModelIndex>(i),&vertices));
    }
    assert(Visible(kBarProps[0],0,false));
    assert(Visible(kBarProps[0],0,true));
    assert(!Visible(kBarProps[1],0,false));
    assert(Visible(kBarProps[1],0,true));
    assert(Visible(kBarProps[2],0,true));
    assert(!Visible(kBarProps[2],0,false));
    assert(Visible(kBarProps[3],0,false));
    assert(!Visible(kBarProps[3],0,true));
    for(const Placement &p:kBarProps)assert(!Visible(p,1,true));
}
