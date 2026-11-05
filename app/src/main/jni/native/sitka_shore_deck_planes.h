#ifndef REVERIE_SITKA_SHORE_DECK_PLANES_H
#define REVERIE_SITKA_SHORE_DECK_PLANES_H

// Source-footprint-aligned flat top texture for actual pier/pontoon POLYGONS.
// No fabricated float grid, supports, gangways, railings or buildings.
// Sits a few millimetres over the existing shaded P0 top; this is visual only.
// One static GPU mesh can hold every authored float; LOD chooses draw ranges.
#include "sitka_shore_geometry.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace reverie { namespace sitka {

struct ShoreDeckVertex {
    float x=0.f,y=0.f,z=0.f,u=0.f,v=0.f;
};
struct ShoreDeckRange {
    size_t first=0u,count=0u;
};
struct ShoreDeckMesh {
    std::vector<ShoreDeckVertex> vertices;
    std::vector<ShoreDeckRange> by_feature;
    size_t triangles()const{return vertices.size()/3u;}
};
constexpr float kShoreDeckTileM=2.4f;
constexpr float kShoreDeckLiftM=.010f;
constexpr size_t kMaxShoreDeckVertices=
    kMaxShoreFeatures*(kMaxShoreFootprintPoints-2u)*3u;

inline bool ShoreInsideDeck(ShorePoint a,ShorePoint b,ShorePoint c,
                            ShorePoint p,float orient){
    return orient*ShoreCross(a,b,p)>=-1.e-5f &&
           orient*ShoreCross(b,c,p)>=-1.e-5f &&
           orient*ShoreCross(c,a,p)>=-1.e-5f;
}

inline ShoreDeckMesh BuildShoreDeckPlanes(
    const std::vector<ShoreFeature>& features){
    if(features.size()>kMaxShoreFeatures)
        throw std::invalid_argument("Too many source harbor structures");
    ShoreDeckMesh mesh;
    mesh.by_feature.reserve(features.size());
    for(const auto& f:features){
        const size_t first=mesh.vertices.size();
        if(f.polygon && (f.material=="pier"||f.material=="pontoon")){
            ShoreCheckPolygon(f.footprint);
            const auto& p=f.footprint;
            const float top=f.base+f.height+kShoreDeckLiftM;
            if(!std::isfinite(top))
                throw std::invalid_argument("Nonfinite harbor deck elevation");
            const float orient=ShoreArea(p)>0.f?1.f:-1.f;
            const auto push=[&](size_t k){
                const auto& v=p[k];
                // UV local to each sourced polygon: no large-coordinate
                // precision loss, texture repeats rather than mesh splits.
                mesh.vertices.push_back({v.x,top,v.z,
                    (v.x-p[0].x)/kShoreDeckTileM,
                    (v.z-p[0].z)/kShoreDeckTileM});
            };
            std::vector<size_t> remaining;
            for(size_t i=0u;i<p.size();++i)remaining.push_back(i);
            while(remaining.size()>3u){
                bool found=false;
                for(size_t i=0;i<remaining.size();++i){
                    const size_t a=remaining[(i+remaining.size()-1u)%remaining.size()];
                    const size_t b=remaining[i];
                    const size_t c=remaining[(i+1u)%remaining.size()];
                    if(orient*ShoreCross(p[a],p[b],p[c])<1.e-5f)continue;
                    bool occupied=false;
                    for(const size_t k:remaining){
                        if(k!=a&&k!=b&&k!=c&&
                           ShoreInsideDeck(p[a],p[b],p[c],p[k],orient)){
                            occupied=true;break;
                        }
                    }
                    if(occupied)continue;
                    push(a);push(b);push(c);
                    remaining.erase(remaining.begin()+static_cast<std::ptrdiff_t>(i));
                    found=true;break;
                }
                if(!found)throw std::invalid_argument("Harbor deck triangulation failed");
            }
            for(const auto k:remaining)push(k);
        }
        mesh.by_feature.push_back({first,mesh.vertices.size()-first});
        if(mesh.vertices.size()>kMaxShoreDeckVertices)
            throw std::invalid_argument("Harbor deck GPU budget exceeded");
    }
    return mesh;
}

}} // namespace reverie::sitka
#endif
