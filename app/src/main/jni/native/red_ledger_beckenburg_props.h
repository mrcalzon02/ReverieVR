#ifndef REVERIE_RED_LEDGER_BECKENBURG_PROPS_H
#define REVERIE_RED_LEDGER_BECKENBURG_PROPS_H
// Curated owner-provided Beckenburg GLB derivatives. Only game-embedded
// binaries cleared for redistribution may be bundled in public APK builds.
// Game-private format (RLBP v1), not an unbounded runtime glTF parser.
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
namespace reverie { namespace redledger { namespace beckenburg {
constexpr int kCount=5;
constexpr uint32_t kAtlasSide=256u;
constexpr uint32_t kMaxVertices=50000u;
constexpr size_t kStride=10u; // XYZ, UV, RGB, unused tile UV
constexpr size_t kTextureBytes=kAtlasSide*kAtlasSide*2u; // RGB565
struct Placement {
    const char *path;
    int room; // 0 bar, 1 stockroom, 2 living
    float x,y,z,scale;
};
static constexpr Placement kProps[kCount]={
    {"native/red-ledger/beckenburg/switch.rlprop",2,-2.45f,-0.22f,-2.94f,0.20f},
    {"native/red-ledger/beckenburg/lamp_off.rlprop",0,0.0f,1.26f,-0.70f,0.65f},
    {"native/red-ledger/beckenburg/door.rlprop",0,-2.55f,-0.42f,-2.94f,2.05f},
    {"native/red-ledger/beckenburg/shelf.rlprop",1,-1.55f,-0.75f,-2.24f,1.45f},
    {"native/red-ledger/beckenburg/chair.rlprop",0,1.94f,-1.07f,-2.32f,0.75f}
};
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
