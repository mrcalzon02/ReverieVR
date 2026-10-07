#include "red_ledger_static_geometry.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>
using namespace reverie::redledger::geometry;
int main() {
    static_assert(kStaticCubeCount==11u,"cube budget");
    static_assert(kStaticVertexCount==396u,"vertex count");
    static_assert(kStaticVertexBytes==15840u,"GPU bytes");
    std::vector<float> a(kStaticVertexFloats+8u,-777.0f);
    std::vector<float> b(kStaticVertexFloats,0.0f);
    assert(!BuildStaticRoomVertices(nullptr,kStaticVertexFloats));
    assert(!BuildStaticRoomVertices(a.data(),kStaticVertexFloats-1u));
    for(float v:a)assert(v==-777.0f);
    assert(BuildStaticRoomVertices(a.data(),kStaticVertexFloats));
    assert(BuildStaticRoomVertices(b.data(),kStaticVertexFloats));
    for(size_t i=0;i<kStaticVertexFloats;++i)
        assert(a[i]==b[i]&&std::isfinite(a[i]));
    for(size_t i=kStaticVertexFloats;i<a.size();++i)
        assert(a[i]==-777.0f);
    bool stone=false,wood=false,metal=false;
    float min_y=1000.0f,max_y=-1000.0f;
    for(size_t i=0;i<kStaticVertexCount;++i) {
        const float *v=&a[i*kStaticVertexStride];
        min_y=std::min(min_y,v[1]);max_y=std::max(max_y,v[1]);
        assert(v[3]>=0.0f&&v[3]<=1.0f&&v[4]>=0.0f&&v[4]<=1.0f);
        for(int c=5;c<8;++c)assert(v[c]>0.0f&&v[c]<=1.0f);
        assert(v[8]==0.00390625f||v[8]==0.50390625f);
        assert(v[9]==0.00390625f||v[9]==0.50390625f);
        if(v[8]<0.5f&&v[9]<0.5f)stone=true;
        if(v[8]>0.5f&&v[9]<0.5f)wood=true;
        if(v[8]<0.5f&&v[9]>0.5f)metal=true;
    }
    assert(stone&&wood&&metal&&min_y< -1.5f&&max_y>1.5f);
    std::cout<<"static room: 11 cubes -> 1 draw/eye; "
        <<kStaticVertexBytes<<" bytes; deterministic\n";
}
