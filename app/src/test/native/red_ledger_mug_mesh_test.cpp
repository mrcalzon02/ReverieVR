#include "red_ledger_mug_mesh.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace reverie::redledger::mug;
int main() {
    static_assert(kVertexCount==180u,"twelve-sided geometry");
    std::vector<float> a(kFloatCount+5u,-123.0f),b(kFloatCount,0.0f);
    assert(!Build(nullptr,kFloatCount));
    assert(!Build(a.data(),kFloatCount-1u));
    for(float value:a) assert(value==-123.0f);
    assert(Build(a.data(),kFloatCount)&&Build(b.data(),kFloatCount));
    for(size_t i=0;i<kFloatCount;++i)
        assert(std::isfinite(a[i])&&a[i]==b[i]);
    for(size_t i=kFloatCount;i<a.size();++i) assert(a[i]==-123.0f);
    size_t top=0,bottom=0;
    for(size_t i=0;i<kVertexCount;++i) {
        const float *p=&a[i*kStride];
        for(size_t c=0;c<5u;++c) assert(std::isfinite(p[c]));
        assert(p[0]>=-0.50001f&&p[0]<=0.50001f);
        assert(p[1]>=-0.5f&&p[1]<=0.5f);
        assert(p[2]>=-0.50001f&&p[2]<=0.50001f);
        assert(p[3]>=0.0f&&p[3]<=1.0f&&p[4]>=0.0f&&p[4]<=1.0f);
        if(p[1]==0.5f)++top;
        if(p[1]==-0.5f)++bottom;
    }
    assert(top>0u&&bottom>0u);
    std::cout<<"Red Ledger mug: "<<kVertexCount
             <<" vertices per reusable 12-sided mesh\n";
}
