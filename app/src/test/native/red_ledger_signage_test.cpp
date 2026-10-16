#include "red_ledger_signage.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace reverie::redledger::signage;
int main() {
    const size_t vertices=VertexCount();
    assert(vertices>1000u && vertices<24000u && vertices%6u==0u);
    const size_t floats=vertices*kVertexStride;
    std::vector<float> a(floats+10u,-777.0f);
    std::vector<float> b(floats,0.0f);
    size_t out=0;
    assert(!Build(nullptr,floats,&out) && out==vertices);
    assert(!Build(a.data(),floats-1u,&out) && out==vertices);
    for(float value:a) assert(value==-777.0f);
    assert(Build(a.data(),floats,&out) && out==vertices);
    assert(Build(b.data(),floats,&out) && out==vertices);
    for(size_t i=0;i<floats;++i)
        assert(std::isfinite(a[i]) && a[i]==b[i]);
    for(size_t i=floats;i<a.size();++i) assert(a[i]==-777.0f);
    // Both background plaques and raised glyphs must be present.
    size_t background=0, letters=0;
    for(size_t i=0;i<vertices;++i) {
        const float *v=&a[i*kVertexStride];
        if(v[5]<0.10f)++background;
        if(v[5]>0.90f)++letters;
        assert(v[8]==0.50390625f && v[9]==0.50390625f);
    }
    assert(background>0 && letters>background);
    std::cout<<"Red Ledger signage: "<<vertices
             <<" baked vertices, one additional draw/eye\n";
}
