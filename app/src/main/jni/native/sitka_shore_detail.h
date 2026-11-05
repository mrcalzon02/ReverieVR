#ifndef REVERIE_SITKA_SHORE_DETAIL_H
#define REVERIE_SITKA_SHORE_DETAIL_H

// Game-private Sitka P1 *stylized* massing detail, not surveyed facade truth.
// The source feature's polygon and stable ID remain authoritative. Geometry is
// generated once at module creation and shares static terrain VBOs across eyes.
#include "sitka_shore_geometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace reverie { namespace sitka {
struct ShoreDetailCount {
    size_t windows=0u, canopies=0u, plank_seams=0u, guardrails=0u, shack_windows=0u;
};
constexpr size_t kMaxShoreDetailQuads=1800u;

inline void ShoreDetailQuad(TerrainMesh& mesh,size_t& budget,
    const std::array<std::array<float,3>,4>& points,
    const std::array<uint8_t,3>& color){
    if(budget>=kMaxShoreDetailQuads)
        throw std::invalid_argument("Sitka shore P1 detail geometry budget exceeded");
    ShoreQuad(mesh,points,color);
    ++budget;
}
inline std::array<float,3> ShoreDetailPoint(const ShoreFeature& f,
                                             float lx,float y,float lz){
    const float c=std::cos(f.yaw),s=std::sin(f.yaw);
    return {f.x+lx*c-lz*s,y,f.z+lx*s+lz*c};
}

inline void ShorePolygonDetail(TerrainMesh& mesh,const ShoreFeature& f,
                                size_t& budget,ShoreDetailCount& counts){
    // Unverified stylized facade detail: never infer a door, accessibility,
    // real window count, public entrance, or building interior from this.
    // A real harbor float is not a shop: never invent glass windows,
    // storefront awnings, or an entrance on its water-facing edges.
    if(f.footprint.size()<3u ||
       (f.material!="storefront" && f.material!="cannery" &&
        f.material!="warehouse" && f.material!="heritage"))return;
    const float orient=ShoreArea(f.footprint)>0.f?1.f:-1.f;
    const size_t floors=std::min<size_t>(3u,std::max<size_t>(1u,
        static_cast<size_t>(f.height/3.2f)));
    const auto glass=std::array<uint8_t,3>{31,53,62};
    size_t windows=0u;
    size_t longest_edge=0u;
    float longest=0.f;
    for(size_t i=0;i<f.footprint.size();++i){
        const auto a=f.footprint[i],b=f.footprint[(i+1u)%f.footprint.size()];
        const float dx=b.x-a.x,dz=b.z-a.z;
        const float len=std::hypot(dx,dz);
        if(len>longest){longest=len;longest_edge=i;}
        if(len<3.f)continue;
        const float ex=dx/len,ez=dz/len;
        const float nx=orient*ez,nz=-orient*ex;
        const size_t slots=std::min<size_t>(6u,
            static_cast<size_t>((len-1.2f)/2.6f));
        for(size_t level=0;level<floors&&windows<16u;++level){
            const float y=f.base+f.height*(.28f+(.48f*float(level)+.02f)/float(floors));
            const float top=std::min(f.base+f.height-.24f,y+.76f);
            if(top-y<.35f)continue;
            for(size_t j=0;j<slots&&windows<16u;++j){
                const float t=(float(j)+.5f)*len/float(slots);
                const float cx=a.x+ex*t+nx*.027f;
                const float cz=a.z+ez*t+nz*.027f;
                const float half=std::min(.43f,.30f*len/float(slots));
                const auto p=[&](float d,float h){
                    return std::array<float,3>{cx+ex*d,h,cz+ez*d};
                };
                ShoreDetailQuad(mesh,budget,{p(-half,y),p(half,y),
                                            p(half,top),p(-half,top)},glass);
                ++windows;
                ++counts.windows;
            }
        }
    }
    // In rainy Southeast Alaska, a thin canopy gives storefronts a distinct
    // skyline without assuming the actual door location. This follows the
    // longest real facade segment rather than a fabricated city street grid.
    if(f.material=="storefront"&&longest>2.6f){
        const auto a=f.footprint[longest_edge];
        const auto b=f.footprint[(longest_edge+1u)%f.footprint.size()];
        const float dx=(b.x-a.x)/longest,dz=(b.z-a.z)/longest;
        const float nx=orient*dz,nz=-orient*dx;
        const float inset=std::min(1.0f,longest*.18f);
        const float y=f.base+std::min(f.height-.3f,2.8f);
        const auto p=[&](float t,float out,float h){
            return std::array<float,3>{a.x+dx*t+nx*out,h,a.z+dz*t+nz*out};
        };
        ShoreDetailQuad(mesh,budget,{
            p(inset,.015f,y),p(longest-inset,.015f,y),
            p(longest-inset,.86f,y-.14f),p(inset,.86f,y-.14f)},
            {97,115,108});
        ++counts.canopies;
    }
}

inline void ShoreBlockDetail(TerrainMesh& mesh,const ShoreFeature& f,
                              size_t& budget,ShoreDetailCount& counts){
    const float deck=f.base+f.height;
    if((f.material=="pier"||f.material=="pontoon")&&f.depth>=2.4f){
        const size_t seams=std::min<size_t>(24u,
            static_cast<size_t>(f.depth/2.0f));
        for(size_t i=1;i<seams;++i){
            const float z=-f.depth*.5f+float(i)*f.depth/float(seams);
            const float w=f.width*.49f;
            const float dz=std::min(.045f,f.depth/float(seams)*.06f);
            ShoreDetailQuad(mesh,budget,{
                ShoreDetailPoint(f,-w,deck+.009f,z-dz),
                ShoreDetailPoint(f,w,deck+.009f,z-dz),
                ShoreDetailPoint(f,w,deck+.009f,z+dz),
                ShoreDetailPoint(f,-w,deck+.009f,z+dz)},
                f.material=="pier"?std::array<uint8_t,3>{73,61,49}:
                                   std::array<uint8_t,3>{102,109,102});
            ++counts.plank_seams;
        }
    }else if(f.material=="bridge_deck"||f.material=="bridge_ramp"){
        for(const float side:{-1.f,1.f}){
            const float x=side*f.width*.49f;
            const float near=f.base+f.height;
            const float far=near+f.rise;
            ShoreDetailQuad(mesh,budget,{
                ShoreDetailPoint(f,x,near+.20f,-f.depth*.5f),
                ShoreDetailPoint(f,x,far+.20f,f.depth*.5f),
                ShoreDetailPoint(f,x,far+.72f,f.depth*.5f),
                ShoreDetailPoint(f,x,near+.72f,-f.depth*.5f)},
                {84,93,96});
            ++counts.guardrails;
        }
    }else if(f.material=="harbor_shack"){
        // One generic facing marker, not a claimed surveyed shack window.
        const float y=f.base+f.height*.50f;
        const float z=-f.depth*.5f-.022f;
        const float hw=std::min(.52f,f.width*.32f);
        ShoreDetailQuad(mesh,budget,{
            ShoreDetailPoint(f,-hw,y-.24f,z),
            ShoreDetailPoint(f,hw,y-.24f,z),
            ShoreDetailPoint(f,hw,y+.24f,z),
            ShoreDetailPoint(f,-hw,y+.24f,z)},
            {29,41,47});
        ++counts.shack_windows;
    }
}

// Called only at initial scene assembly. It is transactional: any geometry
// overflow or malformed authoring leaves the original terrain untouched.
inline ShoreDetailCount AppendShoreDetails(TerrainMesh& mesh,
                                const std::vector<ShoreFeature>& features){
    TerrainMesh candidate=mesh;
    ShoreDetailCount counts{};
    size_t budget=0u;
    for(const auto& feature:features){
        if(feature.polygon)ShorePolygonDetail(candidate,feature,budget,counts);
        else ShoreBlockDetail(candidate,feature,budget,counts);
    }
    mesh=std::move(candidate);
    return counts;
}
}} // namespace reverie::sitka
#endif
