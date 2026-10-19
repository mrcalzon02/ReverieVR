#ifndef REVERIE_RED_LEDGER_OWNED_SMOKING_H
#define REVERIE_RED_LEDGER_OWNED_SMOKING_H
// Four small, owner-authored FBX 7.4 meshes, selected from private Models.zip.
// Uses REAL source polygons and UV indices, not procedural cuboid stand-ins.
// Only selected ~23-38 KiB originals ship as in-game model inputs.
#include "red_ledger_original_fbx.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace reverie { namespace redledger { namespace smoking {
constexpr size_t kCount=4u;
constexpr size_t kStride=10u; // XYZ, UV, RGB, atlas tile origin
enum ModelIndex : size_t {Ashtray=0u,Cigar=1u,BoxOpen=2u,BoxClosed=3u};
struct Source {
    const char *path;
    const char *geometry[2];
    float size[3]; // authored physical fit, meters; never use raw Blender units
    float rgb[3];
    int material; // 0 stone, 1 wood, 2 metal, 3 paper
};
constexpr Source kModels[kCount]={
    {"native/red-ledger/models/owned_ashtray.fbx",{"Cylinder",nullptr},
        {.145f,.028f,.145f},{.74f,.76f,.73f},2},
    {"native/red-ledger/models/owned_cigar.fbx",{"Cube",nullptr},
        {.165f,.013f,.013f},{.65f,.41f,.25f},1},
    {"native/red-ledger/models/owned_cigar_box_open.fbx",{"Cube.001",nullptr},
        {.22f,.050f,.15f},{.62f,.40f,.24f},1},
    {"native/red-ledger/models/owned_cigar_box_closed.fbx",
        {"Cube.001","Cube.003"},{.22f,.055f,.15f},{.55f,.34f,.21f},1}
};
struct Placement {
    ModelIndex model;
    float x,y,z;
    bool requires_patron,requires_empty;
};
// A clean bar still has an ashtray; a live patron exposes the smoking
// tableau, while its owner-authored cigar box closes when the bar is empty.
constexpr Placement kBarProps[]={
    {Ashtray,-2.21f,-.46f,-1.12f,false,false},
    {Cigar,-2.21f,-.433f,-1.12f,true,false},
    {BoxOpen,2.05f,-.50f,-1.45f,true,false},
    {BoxClosed,2.05f,-.50f,-1.45f,false,true}
};
inline bool Visible(const Placement &p,int room,bool patron) {
    return room==0 && (!p.requires_patron||patron) &&
           (!p.requires_empty||!patron);
}
struct Point {std::array<float,3> xyz; std::array<float,2> uv;};
using Triangle=std::array<Point,3>;
inline bool Build(const uint8_t *file,size_t length,ModelIndex model,
                  std::vector<float> *vertices) {
    if(!file||!vertices||model>BoxClosed || length<100u ||
       length>100000u ||std::memcmp(file,"Kaydara FBX Binary  ",20u)!=0 ||
       fbx::U32(file+23u)!=7400u)return false;
    const Source &source=kModels[model];
    std::vector<Triangle> triangles;
    float lo[3]={1e9f,1e9f,1e9f},hi[3]={-1e9f,-1e9f,-1e9f};
    for(const char *name:source.geometry) {
        if(!name)continue;
        fbx::Geometry geometry;
        if(!fbx::Extract(file,length,27u,length,0,false,false,name,&geometry) ||
           geometry.xyz.size()<9u ||geometry.xyz.size()%3u!=0u ||
           geometry.poly.empty() ||geometry.uv.empty() ||
           geometry.uv.size()%2u!=0u ||
           geometry.poly.size()!=geometry.uv_indices.size())return false;
        const size_t positions=geometry.xyz.size()/3u;
        const size_t uvs=geometry.uv.size()/2u;
        std::vector<Point> polygon;
        for(size_t index=0;index<geometry.poly.size();++index) {
            const int32_t signed_point=geometry.poly[index];
            const int64_t pos=signed_point<0?-int64_t(signed_point)-1:
                int64_t(signed_point);
            const int64_t uv=geometry.uv_indices[index];
            if(pos<0||size_t(pos)>=positions||uv<0||size_t(uv)>=uvs)
                return false;
            const double ax=geometry.xyz[size_t(pos)*3u];
            const double ay=geometry.xyz[size_t(pos)*3u+1u];
            const double az=geometry.xyz[size_t(pos)*3u+2u];
            const double tu=geometry.uv[size_t(uv)*2u];
            const double tv=geometry.uv[size_t(uv)*2u+1u];
            if(!std::isfinite(ax)||!std::isfinite(ay)||
               !std::isfinite(az)||!std::isfinite(tu)||
               !std::isfinite(tv))return false;
            Point next{};
            // Blender Z-up. Rotate the long cigar onto the counter surface.
            next.xyz=model==Cigar?
                std::array<float,3>{{float(az),float(ay),float(ax)}}:
                std::array<float,3>{{float(ax),float(az),-float(ay)}};
            next.uv={{float(tu),float(tv)}};
            polygon.push_back(next);
            for(int j=0;j<3;++j) {
                lo[j]=std::min(lo[j],next.xyz[j]);
                hi[j]=std::max(hi[j],next.xyz[j]);
            }
            if(signed_point<0) {
                if(polygon.size()<3u ||polygon.size()>64u)return false;
                for(size_t k=1;k+1<polygon.size();++k)
                    triangles.push_back({{polygon[0],polygon[k],polygon[k+1u]}});
                polygon.clear();
            }
            if(triangles.size()>2000u)return false;
        }
        if(!polygon.empty())return false;
    }
    if(triangles.empty())return false;
    float range[3],center[3];
    for(int j=0;j<3;++j) {
        range[j]=hi[j]-lo[j]; center[j]=(hi[j]+lo[j])*.5f;
        if(!std::isfinite(range[j])||range[j]<.000001f)return false;
    }
    std::vector<float> generated;
    generated.reserve(triangles.size()*3u*kStride);
    for(const Triangle &triangle:triangles) {
        const auto &a=triangle[0].xyz,&b=triangle[1].xyz,&c=triangle[2].xyz;
        const float ux=b[0]-a[0],uy=b[1]-a[1],uz=b[2]-a[2];
        const float vx=c[0]-a[0],vy=c[1]-a[1],vz=c[2]-a[2];
        const float ny=uz*vx-ux*vz;
        const float norm=std::sqrt((uy*vz-uz*vy)*(uy*vz-uz*vy)+ny*ny+
                                  (ux*vy-uy*vx)*(ux*vy-uy*vx));
        const float lit=.86f+.14f*(norm>.000001f?ny/norm:0.f);
        for(const Point &point:triangle) {
            for(int j=0;j<3;++j)
                generated.push_back((point.xyz[j]-center[j])/
                                    range[j]*source.size[j]);
            generated.push_back(std::max(0.f,std::min(1.f,point.uv[0])));
            generated.push_back(1.f-std::max(0.f,std::min(1.f,point.uv[1])));
            for(float color:source.rgb)generated.push_back(color*lit);
            generated.push_back(.00390625f+(source.material&1)*.5f);
            generated.push_back(.00390625f+(source.material>>1)*.5f);
        }
    }
    vertices->swap(generated);
    return true;
}
}}}
#endif
