#include "../../main/jni/native/red_ledger_beckenburg_props.h"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
// Run with a private directory containing the five curated RLBP outputs.
// No GL context or purchased originals are needed in the public test tree.
int main(int argc,char **argv) {
    assert(argc==2);
    const char *const names[]={"switch","lamp_off","door","shelf","chair"};
    using namespace reverie::redledger::beckenburg;
    for(const char *name:names){
        std::ifstream stream(std::string(argv[1])+"/"+name+".rlprop",
                             std::ios::binary);
        assert(stream.good());
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(stream)),{});
        std::vector<float> vertices;
        const uint8_t *pixels=nullptr;
        assert(Decode(data.data(),data.size(),&vertices,&pixels));
        assert(vertices.size()%kStride==0u);
        assert(static_cast<size_t>(pixels-data.data())+kTextureBytes==data.size());
        assert(!Decode(data.data(),data.size()-1u,&vertices,&pixels));
        data[0]=0u;
        assert(!Decode(data.data(),data.size(),&vertices,&pixels));
        data[0]='R';
        data[16]=data[17]=data[18]=data[19]=0xffu;
        assert(!Decode(data.data(),data.size(),&vertices,&pixels));
    }
    return 0;
}
