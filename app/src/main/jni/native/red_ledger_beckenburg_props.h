#ifndef REVERIE_RED_LEDGER_BECKENBURG_PROPS_H
#define REVERIE_RED_LEDGER_BECKENBURG_PROPS_H
// Curated models from the owner-approved Beckenburg library. Purchased mesh
// files and packaged RLBP derivatives are intentionally NOT in public GitHub.
// This manifest contains instance transforms, not model data.
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
namespace reverie { namespace redledger { namespace beckenburg {
constexpr int kCount=14;
constexpr uint32_t kAtlasSide=256u;
constexpr uint32_t kMaxVertices=50000u;
constexpr size_t kStride=10u; // XYZ UV RGB + unused tile UV
constexpr size_t kTextureBytes=kAtlasSide*kAtlasSide*2u;
struct Asset {const char *path;};
static constexpr Asset kAssets[kCount]={
    {"native/red-ledger/beckenburg/switch.rlprop"},
    {"native/red-ledger/beckenburg/lamp_off.rlprop"},
    {"native/red-ledger/beckenburg/door.rlprop"},
    {"native/red-ledger/beckenburg/shelf.rlprop"},
    {"native/red-ledger/beckenburg/chair.rlprop"},
    {"native/red-ledger/beckenburg/wall.rlprop"},
    {"native/red-ledger/beckenburg/floor.rlprop"},
    {"native/red-ledger/beckenburg/window.rlprop"},
    {"native/red-ledger/beckenburg/crate.rlprop"},
    {"native/red-ledger/beckenburg/barrel.rlprop"},
    {"native/red-ledger/beckenburg/table.rlprop"},
    {"native/red-ledger/beckenburg/mattress.rlprop"},
    {"native/red-ledger/beckenburg/lamp_on.rlprop"},
    {"native/red-ledger/beckenburg/coffee_table.rlprop"}
};
enum Gate : int {Always=-1, Walls=0, Floor=1, Ceiling=2, Seating=3};
struct Placement {
    int asset,room; // 0 bar, 1 stockroom, 2 living
    float x,y,z,sx,sy,sz;
    int quarter_turns; // 90-degree world-Y rotations, no trig per draw
    Gate gate;
    int min_tier,max_tier;
};
static constexpr Placement kPlacements[]={
    {5,0,-2.94f,0.0f,-0.6f,4.95f,4.2f,3.2f,1,Always,0,5},
    {5,1,-2.94f,0.0f,-0.6f,4.95f,4.2f,3.2f,1,Always,0,5},
    {5,2,-2.94f,0.0f,-0.6f,4.95f,4.2f,3.2f,1,Always,0,5},
    {7,0,2.94f,0.0f,-0.6f,4.95f,4.2f,3.2f,1,Always,0,5},
    {7,2,2.94f,0.0f,-0.6f,4.95f,4.2f,3.2f,1,Always,0,5},
    {6,0,0.0f,-1.485f,-0.6f,6.0f,1.0f,5.0f,0,Floor,1,5},
    {6,1,0.0f,-1.485f,-0.6f,6.0f,1.0f,5.0f,0,Always,0,5},
    {6,2,0.0f,-1.485f,-0.6f,6.0f,1.0f,5.0f,0,Always,0,5},
    {2,0,-2.55f,-0.23f,-2.94f,2.05f,2.05f,2.05f,0,Always,0,5},
    {1,0,0.0f,1.26f,-0.7f,0.68f,0.68f,0.68f,0,Ceiling,0,0},
    {12,0,0.0f,1.26f,-0.7f,0.68f,0.68f,0.68f,0,Ceiling,1,5},
    {4,0,1.94f,-1.07f,-2.32f,0.95f,0.95f,0.95f,0,Always,0,5},
    {10,0,-2.02f,-1.07f,0.18f,1.7f,1.7f,1.7f,0,Always,0,5},
    {4,0,-2.15f,-1.04f,-1.11f,0.94f,0.94f,0.94f,0,Seating,1,5},
    {4,0,-2.15f,-1.04f,-0.24f,0.94f,0.94f,0.94f,0,Seating,2,5},
    {4,0,2.18f,-1.04f,-0.04f,0.94f,0.94f,0.94f,0,Seating,3,5},
    {4,0,-1.28f,-1.04f,0.82f,0.94f,0.94f,0.94f,0,Seating,4,5},
    {4,0,1.55f,-1.04f,0.88f,0.94f,0.94f,0.94f,0,Seating,5,5},
    {3,1,-1.55f,-0.75f,-2.24f,1.45f,1.45f,1.45f,0,Always,0,5},
    {8,1,1.8f,-1.13f,-2.2f,0.75f,0.75f,0.75f,0,Always,0,5},
    {8,1,2.35f,-1.13f,-1.48f,0.75f,0.75f,0.75f,0,Always,0,5},
    {9,1,2.42f,-0.99f,-2.5f,1.08f,1.08f,1.08f,0,Always,0,5},
    {0,2,-2.45f,-0.22f,-2.94f,0.2f,0.2f,0.2f,0,Always,0,5},
    {11,2,1.35f,-0.9f,-1.74f,1.65f,1.35f,2.75f,0,Always,0,5},
    {13,2,-1.65f,-1.11f,-2.3f,1.15f,1.15f,1.15f,0,Always,0,5}
};
constexpr int kPlacementCount=sizeof(kPlacements)/sizeof(kPlacements[0]);
inline bool Visible(const Placement &p,int room,int walls,int floor,
                    int ceiling,int seating) {
    if(p.room!=room)return false;
    const int tier=p.gate==Walls?walls:p.gate==Floor?floor:
                   p.gate==Ceiling?ceiling:p.gate==Seating?seating:0;
    return tier>=p.min_tier&&tier<=p.max_tier;
}
inline void MakePlacementTransform(const Placement &p,float *out) {
    const int q=p.quarter_turns;
    const float c=q==0?1.f:q==2?-1.f:0.f;
    const float s=q==1?1.f:q==3?-1.f:0.f;
    const float m[16]={
        c*p.sx,0.f,-s*p.sx,0.f,
        0.f,p.sy,0.f,0.f,
        s*p.sz,0.f,c*p.sz,0.f,
        p.x,p.y,p.z,1.f
    };
    std::memcpy(out,m,sizeof(m));
}
inline uint32_t ReadLe32(const uint8_t *p) {
    return uint32_t(p[0])|(uint32_t(p[1])<<8u)|
           (uint32_t(p[2])<<16u)|(uint32_t(p[3])<<24u);
}
// Does not claim a model imported when the data is invalid. Geometry is
// copied to an aligned float vector before GLES2 upload.
inline bool Decode(const uint8_t *data,size_t length,
                   std::vector<float> *vertices,const uint8_t **rgb565) {
    if(!data||!vertices||!rgb565||length<16u+kTextureBytes||
       std::memcmp(data,"RLBP",4u)!=0||ReadLe32(data+4u)!=1u||
       ReadLe32(data+12u)!=kAtlasSide)return false;
    const uint32_t count=ReadLe32(data+8u);
    if(count==0u||count>kMaxVertices||count%3u!=0u)return false;
    const size_t count_floats=size_t(count)*kStride;
    const size_t geometry_bytes=count_floats*sizeof(float);
    if(length!=16u+geometry_bytes+kTextureBytes)return false;
    std::vector<float> unpacked(count_floats);
    std::memcpy(unpacked.data(),data+16u,geometry_bytes);
    for(size_t i=0;i<count_floats;i+=kStride){
        for(size_t c=0;c<8u;++c)
            if(!std::isfinite(unpacked[i+c]))return false;
        if(unpacked[i+3u]<0.f||unpacked[i+3u]>1.f||
           unpacked[i+4u]<0.f||unpacked[i+4u]>1.f)return false;
        for(size_t c=5u;c<8u;++c)
            if(unpacked[i+c]<0.f||unpacked[i+c]>1.f)return false;
    }
    *rgb565=data+16u+geometry_bytes;
    vertices->swap(unpacked);
    return true;
}
}}}
#endif
