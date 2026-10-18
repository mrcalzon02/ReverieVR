#include "breakwater_font_atlas.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
using namespace reverie::breakwater::font;
int main() {
    std::vector<unsigned char> pixels;
    assert(DecodeAtlas(pixels));
    assert(pixels.size()==static_cast<size_t>(kWidth*kHeight));
    size_t covered=0, partial=0;
    for(unsigned char alpha:pixels) {
        if(alpha>0) ++covered;
        if(alpha>0 && alpha<255) ++partial;
    }
    assert(covered>900 && partial>100); // real antialiased curves, not blocks
    assert(Lookup('B')==Lookup('b'));
    assert(Lookup('W')->advance>Lookup('I')->advance);
    assert(Advance("M",1.f)>Advance("I",1.f));
    const char *labels[]={"BREAKWATER","SELECT DIFFICULTY",
        "POINT AND SELECT","CREDITS 99999","AA 3/3",
        "SIEGE - 5 FOES 18 DAY WAVES","ANTI AIR","START"};
    for(const char *label:labels) {
        const float width=5.70f,scale=FittedScale(label,width,.052f);
        assert(scale>0.f && scale<=.052f);
        assert(Advance(label,scale)<=width-.119f);
        std::vector<float> mesh;
        AppendText(mesh,label,0.f,.5f,scale,1.f,.9f,.8f,-2.97f);
        assert(!mesh.empty() && mesh.size()%48==0);
        for(size_t i=0;i<mesh.size();i+=8){
            for(int j=0;j<8;++j)assert(std::isfinite(mesh[i+j]));
            assert(mesh[i+0]>=-width*.5f-.01f &&
                   mesh[i+0]<=width*.5f+.01f);
            assert(mesh[i+1]>.30f && mesh[i+1]<.70f);
            assert(mesh[i+3]>=0.f && mesh[i+3]<=1.f);
            assert(mesh[i+4]>=0.f && mesh[i+4]<=1.f);
        }
    }
    std::puts("Breakwater Lato font decode, proportional fitting and mesh passed");
}
