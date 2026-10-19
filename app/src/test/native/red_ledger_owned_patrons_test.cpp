#include "../../main/jni/native/red_ledger_owned_patrons.h"
#include "../../main/jni/native/red_ledger_beckenburg_props.h"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
using reverie::redledger::owned_patrons::ForPatron;
using reverie::redledger::owned_patrons::Gopnick;
using reverie::redledger::owned_patrons::PinkDress;
using reverie::redledger::PatronKind;
int main(int argc,char **argv) {
    assert(argc==2);
    assert(ForPatron(PatronKind::NightClerk)==PinkDress);
    assert(ForPatron(PatronKind::DirectorateCourier)==PinkDress);
    assert(ForPatron(PatronKind::SalihorskHauler)==Gopnick);
    const int triangles[2]={6792,6918};
    for(size_t i=0;i<2u;++i) {
        const std::string path=std::string(argv[1])+"/app/src/main/assets/"+
                               reverie::redledger::owned_patrons::kPaths[i];
        std::ifstream stream(path,std::ios::binary);
        assert(stream.good());
        const std::vector<uint8_t> raw{
            std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>()};
        std::vector<float> geometry;
        const uint8_t *texture=nullptr;
        assert(reverie::redledger::beckenburg::Decode(
            raw.data(),raw.size(),&geometry,&texture));
        assert(geometry.size()==static_cast<size_t>(triangles[i])*3u*10u);
        assert(texture!=nullptr);
        bool color_varies=false;
        for(size_t k=2;k<reverie::redledger::beckenburg::kTextureBytes;k+=2)
            if(texture[k]!=texture[0]||texture[k+1]!=texture[1]) {
                color_varies=true;break;
            }
        assert(color_varies); // real source atlas, not monochrome stand-in
        assert(!reverie::redledger::beckenburg::Decode(
            raw.data(),raw.size()-1u,&geometry,&texture));
    }
}
