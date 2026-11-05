#include "reverie_native_horizon_geometry.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    std::vector<reverie::native::horizon::Vertex> v;
    using namespace reverie::native::horizon;
    Sphere(v);
    assert(v.size()==kFullVertices);
    for(size_t i=0;i<v.size();++i){
        const auto& p=v[i];
        assert(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z));
        assert(p.u>=0&&p.u<=1&&p.v>=0&&p.v<=1);
        assert(std::fabs(p.x*p.x+p.y*p.y+p.z*p.z-1)<0.00001f);
        if(i<kUpperVertices)assert(p.y>=-0.00001f);
    }
    for(int row=0;row<kBands;++row){
      const auto& first=v[row*kSegments*6+0];
      const auto& last=v[row*kSegments*6+(kSegments-1)*6+1];
      assert(std::abs(first.x-last.x)<.000001f);
      assert(std::abs(first.z-last.z)<.000001f);
      assert(first.u==0&&last.u==1);
    }
    std::cout<<"PASS shared 360 sky mesh 2304 vertices + upper hemisphere"<<std::endl;
}
