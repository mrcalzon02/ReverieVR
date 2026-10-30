#include "red_ledger_signage.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace reverie::redledger::signage;
int main() {
    const size_t vertices=VertexCount();
    assert(vertices>60u && vertices<5000u && vertices%6u==0u);
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
    // Geometry now contains ONLY physical plaques. All functional letters
    // must be rendered through the licensed Lato shader path.
    size_t background=0, letters=0;
    for(size_t i=0;i<vertices;++i) {
        const float *v=&a[i*kVertexStride];
        if(v[5]<0.10f)++background;
        if(v[5]>0.90f)++letters;
        assert(v[8]==0.50390625f && v[9]==0.50390625f);
    }
    assert(background>0u && letters==0u);
    // All 24 product cards are baked with the stockroom's fixed lettering.
    // Living quarters contain no product-picker signage at all.
    assert(VertexCount(1)>VertexCount(2)+200u);
    for(int room=1;room<3;++room) {
        const size_t n=VertexCount(room);
        assert(n>20u && n<5000u && n%6u==0u);
        std::vector<float> extra(n*kVertexStride+3u,-777.0f);
        size_t generated=0u;
        assert(Build(extra.data(),n*kVertexStride,&generated,room));
        assert(generated==n);
        for(size_t i=n*kVertexStride;i<extra.size();++i)
            assert(extra[i]==-777.0f);
    }
    const size_t dynamic=EmitLine(nullptr,0u,"DAY 12 CASH 2200",
        0.0f,0.9f,-2.8f,0.014f);
    assert(dynamic==6u);
    std::vector<float> board(dynamic*kVertexStride+5u,-777.0f);
    assert(EmitLine(board.data(),0u,"DAY 12 CASH 2200",
        0.0f,0.9f,-2.8f,0.014f)==dynamic);
    for(size_t i=dynamic*kVertexStride;i<board.size();++i)
        assert(board[i]==-777.0f);
    std::cout<<"Red Ledger signage: "<<vertices
             <<" plaques, no fake glyph geometry\n";
}
