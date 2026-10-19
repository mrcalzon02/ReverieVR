#ifndef REVERIE_BREAKWATER_BECKENBURG_MESH_H
#define REVERIE_BREAKWATER_BECKENBURG_MESH_H

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace reverie { namespace breakwater { namespace imported {

// Real PSX Mega Pack GLB source triangles, converted to BWM1 using their
// original indices, vertex positions and sampled embedded albedo textures.
// Not procedural primitives, sprite cards, or a substitute mesh library.
// Each triangle stores three source vertices as signed 16-bit XYZ * 8192
// plus its three corresponding sampled RGB texels. There are no pointers,
// GPU resources, animations or endianness-sensitive structure casts.
constexpr std::array<const char*,5> kAssetPaths={{
    "native/breakwater/beckenburg/concrete_block_1.bwm",
    "native/breakwater/beckenburg/wooden_crate_1.bwm",
    "native/breakwater/beckenburg/ammo_box_mp_1.bwm",
    "native/breakwater/beckenburg/owner_boat_rowboat.bwm",
    "native/breakwater/beckenburg/owner_apc_armor.bwm"
}};
constexpr size_t kTriangleBytes=27u;
constexpr uint32_t kMaxTriangles=2048u;
constexpr uint32_t kMaxAssetBytes=8u+kMaxTriangles*kTriangleBytes;
enum ModelKind : size_t { Concrete=0u, WoodenCrate=1u, AmmoBox=2u, OwnerRowboat=3u, OwnerAPC=4u };
constexpr uint32_t ReadU32(const uint8_t *data) {
    return static_cast<uint32_t>(data[0])|
           (static_cast<uint32_t>(data[1])<<8u)|
           (static_cast<uint32_t>(data[2])<<16u)|
           (static_cast<uint32_t>(data[3])<<24u);
}
constexpr int16_t ReadI16(const uint8_t *data) {
    const uint16_t value=static_cast<uint16_t>(
        static_cast<unsigned>(data[0])|(static_cast<unsigned>(data[1])<<8u));
    return static_cast<int16_t>(value<32768u?
        static_cast<int>(value):static_cast<int>(value)-65536);
}
struct Model {
    std::vector<uint8_t> bytes;
    uint32_t triangles=0u;
    bool Parse(const uint8_t *data,size_t length) {
        bytes.clear();triangles=0u;
        if(!data||length<8u||length>kMaxAssetBytes||
           data[0]!='B'||data[1]!='W'||data[2]!='M'||data[3]!='1')
            return false;
        const uint32_t count=ReadU32(data+4u);
        if(count==0u||count>kMaxTriangles||
           length!=8u+static_cast<size_t>(count)*kTriangleBytes)
            return false;
        // Reject malformed/truncated source points; empty or implausible
        // submeshes must not silently turn into old placeholder boxes.
        for(size_t i=8u;i<length;i+=9u) {
            const int16_t x=ReadI16(data+i),y=ReadI16(data+i+2u),
                z=ReadI16(data+i+4u);
            if(x<=-32000||x>=32000||y<=-32000||y>=32000||
               z<=-32000||z>=32000)return false;
        }
        bytes.assign(data,data+length);
        triangles=count;
        return true;
    }
    bool valid() const {return triangles>0u&&!bytes.empty();}
    template<class EmitTriangle>
    size_t Emit(float x,float floorY,float z,float scale,float yaw,
                EmitTriangle emit) const {
        if(!valid()||!std::isfinite(x)||!std::isfinite(floorY)||
           !std::isfinite(z)||!std::isfinite(scale)||scale<=0.f||
           !std::isfinite(yaw))return 0u;
        const float c=std::cos(yaw),s=std::sin(yaw);
        for(size_t n=0;n<triangles;++n) {
            float p[3][3]{},color[3][3]{};
            for(size_t v=0;v<3u;++v) {
                const size_t start=8u+n*kTriangleBytes+v*9u;
                const float lx=ReadI16(&bytes[start])/8192.f*scale;
                const float ly=ReadI16(&bytes[start+2u])/8192.f*scale;
                const float lz=ReadI16(&bytes[start+4u])/8192.f*scale;
                p[v][0]=x+lx*c-lz*s;
                p[v][1]=floorY+ly;
                p[v][2]=z+lx*s+lz*c;
                for(int channel=0;channel<3;++channel)
                    color[v][channel]=bytes[start+6u+channel]/255.f;
            }
            // Current GLES2 XYZRGB pipeline takes one color per triangle;
            // averaged vertex colors preserve a truthful sampled original
            // texture tone while retaining ALL authentic source faces.
            emit(p[0][0],p[0][1],p[0][2],
                 p[1][0],p[1][1],p[1][2],
                 p[2][0],p[2][1],p[2][2],
                 (color[0][0]+color[1][0]+color[2][0])/3.f,
                 (color[0][1]+color[1][1]+color[2][1])/3.f,
                 (color[0][2]+color[1][2]+color[2][2])/3.f);
        }
        return triangles;
    }
};

template<class ReadAsset>
bool Load(Model *model,const char *path,ReadAsset reader) {
    if(!model||!path)return false;
    uint32_t length=0u;
    if(!reader(path,nullptr,0u,&length)||length<8u||
       length>kMaxAssetBytes)return false;
    std::vector<uint8_t> file(length);
    uint32_t received=0u;
    if(!reader(path,file.data(),length,&received)||received!=length)
        return false;
    return model->Parse(file.data(),received);
}

}}} // namespace reverie::breakwater::imported
#endif
