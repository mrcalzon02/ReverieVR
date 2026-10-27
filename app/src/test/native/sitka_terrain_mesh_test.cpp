#include "../../main/jni/native/sitka_terrain_mesh.h"
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
using namespace reverie::sitka;
static void u32(std::vector<uint8_t>& v,uint32_t n){for(int i=0;i<4;i++)v.push_back(uint8_t(n>>(8*i)));}
static void f32(std::vector<uint8_t>& v,float n){uint32_t b;std::memcpy(&b,&n,4);u32(v,b);}
static void f64(std::vector<uint8_t>& v,double n){uint64_t b;std::memcpy(&b,&n,8);for(int i=0;i<8;i++)v.push_back(uint8_t(b>>(8*i)));}
static bool rejects(const std::vector<uint8_t>& v){try{ParseTerrainMesh(v.data(),v.size());return false;}catch(const std::invalid_argument&){return true;}}
int main(){
    std::vector<uint8_t> bytes={'S','I','T','K','A','0','1',0};
    u32(bytes,4);u32(bytes,2);f64(bytes,480005.0);f64(bytes,6299995.0);
    for (auto p:std::array<std::array<float,3>,4>{{{{0,35,0}},{{10,35,0}},{{0,35,-10}},{{10,35,-10}}}}){
        for(float x:p)f32(bytes,x);
        bytes.insert(bytes.end(),{64,115,77});
    }
    for(uint32_t n:std::array<uint32_t,6>{{0,2,1,1,2,3}})u32(bytes,n);
    assert(bytes.size()==116u);
    const auto mesh=ParseTerrainMesh(bytes.data(),bytes.size());
    assert(mesh.origin_east==480005.0&&mesh.origin_north==6299995.0);
    assert(mesh.vertices.size()==4u&&mesh.indices.size()==6u);
    const auto packed=PackTerrainGles2(mesh);
    assert(packed.size()==1u&&packed[0].vertices.size()==4u&&packed[0].indices.size()==6u);
    assert(packed[0].indices[0]==0&&packed[0].indices[1]==1&&packed[0].indices[2]==2);
    auto bad=bytes;bad[0]='X';assert(rejects(bad));
    bad=bytes;bad.pop_back();assert(rejects(bad));
    bad=bytes;bad.push_back(0);assert(rejects(bad));
    bad=bytes;bad[8]=0;assert(rejects(bad));
    bad=bytes;bad[92]=0xff;bad[93]=0xff;bad[94]=0xff;bad[95]=0xff;assert(rejects(bad));
    bad=bytes;bad[96]=0;bad[97]=0;bad[98]=0;bad[99]=0;assert(rejects(bad));
    bad=bytes;bad[32]=0;bad[33]=0;bad[34]=0xc0;bad[35]=0x7f;assert(rejects(bad));
    bad=bytes;bad[16]=0xff;bad[17]=0xff;bad[18]=0xff;bad[19]=0xff;
    bad[20]=0xff;bad[21]=0xff;bad[22]=0xff;bad[23]=0x7f;assert(rejects(bad));
    // 70k vertices and 23k+ triangles force a real GLES2 16-bit split.
    TerrainMesh large;large.vertices.reserve(70002);
    for(int i=0;i<70002;i++)large.vertices.push_back({float(i%200),float(i/200),0,50,80,100});
    for(uint32_t i=0;i<70002;i+=3)large.indices.insert(large.indices.end(),{i,i+1,i+2});
    auto chunks=PackTerrainGles2(large);
    assert(chunks.size()==2u);
    size_t triangles=0;
    for(const auto& chunk:chunks){assert(chunk.vertices.size()<=65535u);assert(chunk.indices.size()%3u==0u);triangles+=chunk.indices.size()/3u;for(uint16_t index:chunk.indices)assert(index<chunk.vertices.size());}
    assert(triangles==23334u);
    std::cout<<"Sitka .sitkam v1 strict decode and GLES2 ushort batch split PASS\n";
}
