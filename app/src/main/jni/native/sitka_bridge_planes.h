#ifndef REVERIE_SITKA_BRIDGE_PLANES_H
#define REVERIE_SITKA_BRIDGE_PLANES_H

// PSX Sitka bridge: SIMPLE textured planes, NOT a tower/girder/cable mesh.
// The bridge's real-world alignment/elevation must be supplied by reviewed GIS.
// No built-in O'Connell origin, invented endpoints, procedural bridge roads
// or synthetic landmark placement is permitted by this file.
//
// Source-authored side silhouette = ONE alpha-textured quad per side;
// concrete road = one top quad per grade interval;
// underside = one separate quad per grade interval.
// This is CPU-only geometry. The textured GL adapter and verified geographic
// placement are separate gates; material paths describe approved candidates.
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace reverie { namespace sitka {
enum class BridgePlaneMaterial : uint8_t {
    SideSilhouette = 0, RoadConcrete = 1, UnderdeckConcrete = 2
};

struct BridgeAxisPoint {
    float x=0.f, y=0.f, z=0.f; // terrain-origin-local metres, Y is road TOP
};
struct BridgePlaneVertex {
    float x=0.f, y=0.f, z=0.f, u=0.f, v=0.f;
};
struct BridgePlane {
    BridgePlaneMaterial material;
    std::array<BridgePlaneVertex,4> vertex;
};
struct BridgePlaneMesh {
    std::vector<BridgePlane> planes;
    size_t triangle_count() const { return planes.size()*2u; }
    size_t vertex_count() const { return planes.size()*4u; }
    size_t Count(BridgePlaneMaterial m) const {
        size_t n=0u;
        for(const auto& plane:planes)if(plane.material==m)++n;
        return n;
    }
};

constexpr size_t kMaxBridgeGradeSegments=64u;
constexpr float kBridgeMaxAxisDeviationM=.5f;
constexpr float kBridgeRepeatMetres=8.f;
// These textures are shared candidates, not duplicated Sitka assets.
// The side sprite is intentionally unbundled until landmark art is checked.
inline const char* BridgePlaneTexturePath(BridgePlaneMaterial material){
    switch(material){
        case BridgePlaneMaterial::SideSilhouette:
            return "sitka/structures/oconnell_side.png";
        case BridgePlaneMaterial::RoadConcrete:
            return "native/shared/v1/materials/coastal/quay_concrete.png";
        case BridgePlaneMaterial::UnderdeckConcrete:
            return "native/shared/v1/materials/coastal/reinforced_concrete.png";
    }
    throw std::invalid_argument("Unknown Sitka bridge material slot");
}

inline BridgePlaneVertex BridgeVertex(
    float x,float y,float z,float u,float v){
    return {x,y,z,u,v};
}
inline void BridgeAddQuad(BridgePlaneMesh& mesh,BridgePlaneMaterial material,
                          BridgePlaneVertex a,BridgePlaneVertex b,
                          BridgePlaneVertex c,BridgePlaneVertex d){
    mesh.planes.push_back({material,{{a,b,c,d}}});
}

// A bridge corridor must be a *single sourced straight span*. Curved or
// skewed approaches are represented as separately verified bridge sections
// rather than hiding a fabricated continuous billboard across a bend.
inline BridgePlaneMesh BuildBridgePlanes(
    const std::vector<BridgeAxisPoint>& axis, float width_m,
    float deck_thickness_m, float side_art_above_deck_m){
    if(axis.size()<2u || axis.size()>kMaxBridgeGradeSegments+1u ||
       !std::isfinite(width_m) || width_m<2.f || width_m>50.f ||
       !std::isfinite(deck_thickness_m) || deck_thickness_m<.05f ||
       deck_thickness_m>5.f ||
       !std::isfinite(side_art_above_deck_m) ||
       side_art_above_deck_m<.25f || side_art_above_deck_m>60.f)
        throw std::invalid_argument("Invalid sourced Sitka bridge plane profile");
    for(const auto& p:axis)
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||
           !std::isfinite(p.z)||std::abs(p.x)>100000.f||
           std::abs(p.z)>100000.f||std::abs(p.y)>2000.f)
            throw std::invalid_argument("Invalid bridge axis coordinate");
    const float ax=axis.back().x-axis.front().x;
    const float az=axis.back().z-axis.front().z;
    const float span=std::hypot(ax,az);
    if(span<1.f || span>5000.f)
        throw std::invalid_argument("Invalid bridge span length");
    const float dx=ax/span,dz=az/span;
    const float nx=-dz,nz=dx;
    float previous=0.f;
    for(size_t i=0;i<axis.size();++i){
        const float relx=axis[i].x-axis.front().x;
        const float relz=axis[i].z-axis.front().z;
        const float along=relx*dx+relz*dz;
        const float cross=relx*nx+relz*nz;
        if(std::abs(cross)>kBridgeMaxAxisDeviationM ||
           (i>0u && along-previous<.25f) ||
           along>span+.05f)
            throw std::invalid_argument("Bridge section must follow a sourced straight axis");
        previous=along;
        if(i>0u){
            const float dy=std::abs(axis[i].y-axis[i-1u].y);
            const float ds=std::hypot(axis[i].x-axis[i-1u].x,
                                      axis[i].z-axis[i-1u].z);
            if(dy>ds*.3f)
                throw std::invalid_argument("Bridge deck grade exceeds source profile limit");
        }
    }
    const float half=width_m*.5f;
    BridgePlaneMesh mesh;
    mesh.planes.reserve(2u*axis.size());
    const auto at=[&](const BridgeAxisPoint& p,float lateral,float y,
                      float u,float v){
        return BridgeVertex(p.x+nx*lateral,y,p.z+nz*lateral,u,v);
    };
    // Each deck interval is a single quad. Repeat the 64x64 concrete source
    // in UVs rather than subdividing the plane into geometry for tile detail.
    float travelled=0.f;
    for(size_t i=1u;i<axis.size();++i){
        const auto& p=axis[i-1u];
        const auto& q=axis[i];
        const float step=std::hypot(q.x-p.x,q.z-p.z);
        const float v0=travelled/kBridgeRepeatMetres;
        const float v1=(travelled+step)/kBridgeRepeatMetres;
        BridgeAddQuad(mesh,BridgePlaneMaterial::RoadConcrete,
            at(p,-half,p.y,0.f,v0),
            at(p, half,p.y,1.f,v0),
            at(q, half,q.y,1.f,v1),
            at(q,-half,q.y,0.f,v1));
        BridgeAddQuad(mesh,BridgePlaneMaterial::UnderdeckConcrete,
            at(q,-half,q.y-deck_thickness_m,0.f,v1),
            at(q, half,q.y-deck_thickness_m,1.f,v1),
            at(p, half,p.y-deck_thickness_m,1.f,v0),
            at(p,-half,p.y-deck_thickness_m,0.f,v0));
        travelled+=step;
    }

    // The entire side silhouette (towers, stays, girders as approved artwork)
    // is one alpha-textured surface per side, UV [0,1] in both axes.
    // No duplicate textured geometry and no dozens of decorative trusses.
    const auto& p=axis.front();
    const auto& q=axis.back();
    constexpr float pad=.04f; // prevent z-fighting with deck edges
    for(const float side:{-1.f,1.f}){
        const float lateral=side*(half+pad);
        BridgeAddQuad(mesh,BridgePlaneMaterial::SideSilhouette,
            at(p,lateral,p.y-deck_thickness_m,0.f,1.f),
            at(q,lateral,q.y-deck_thickness_m,1.f,1.f),
            at(q,lateral,q.y+side_art_above_deck_m,1.f,0.f),
            at(p,lateral,p.y+side_art_above_deck_m,0.f,0.f));
    }
    if(mesh.planes.size()!=2u*axis.size())
        throw std::logic_error("Sitka bridge plane budget drift");
    return mesh;
}
}} // namespace reverie::sitka
#endif
