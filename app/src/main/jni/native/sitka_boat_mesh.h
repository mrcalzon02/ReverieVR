#ifndef REVERIE_SITKA_BOAT_MESH_H
#define REVERIE_SITKA_BOAT_MESH_H
// Game-private, asset-free low-poly boat geometry. CPU builds only on hull/
// equipment changes, never during eye drawing. Coordinates are column-local,
// with the wheel always at (0,0,0), and +Z toward the bow.
#include "sitka_helm.h"
#include "sitka_terrain_mesh.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace reverie { namespace sitka {
struct BoatTint { uint8_t r,g,b; };
inline constexpr BoatTint kHullColor={155,164,157};
inline constexpr BoatTint kDeckColor={165,139,103};
inline constexpr BoatTint kRailColor={212,207,180};
inline constexpr BoatTint kSteelColor={95,109,119};
inline constexpr BoatTint kBrassColor={205,170,87};
inline constexpr BoatTint kDarkScreen={19,43,49};

class BoatMeshBuilder {
public:
    TerrainMesh mesh;
    void Tri(const std::array<float,3>& a,const std::array<float,3>& b,
             const std::array<float,3>& c,BoatTint tint){
        const uint32_t n=static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back({a[0],a[1],a[2],tint.r,tint.g,tint.b});
        mesh.vertices.push_back({b[0],b[1],b[2],tint.r,tint.g,tint.b});
        mesh.vertices.push_back({c[0],c[1],c[2],tint.r,tint.g,tint.b});
        mesh.indices.insert(mesh.indices.end(),{n,n+1u,n+2u});
    }
    void Quad(const std::array<float,3>& a,const std::array<float,3>& b,
              const std::array<float,3>& c,const std::array<float,3>& d,
              BoatTint tint){
        Tri(a,b,c,tint);Tri(a,c,d,tint);
    }
    void Box(float x0,float y0,float z0,float x1,float y1,float z1,BoatTint tint){
        if(!(x0<x1&&y0<y1&&z0<z1))
            throw std::invalid_argument("Sitka boat box has degenerate volume");
        const std::array<float,3>
            a={x0,y0,z0},b={x1,y0,z0},c={x1,y0,z1},d={x0,y0,z1},
            e={x0,y1,z0},f={x1,y1,z0},g={x1,y1,z1},h={x0,y1,z1};
        Quad(a,b,c,d,tint);Quad(e,h,g,f,tint);
        Quad(a,e,f,b,tint);Quad(d,c,g,h,tint);
        Quad(a,d,h,e,tint);Quad(b,f,g,c,tint);
    }
    void RingXY(float center_x,float center_y,float z,float radius,
                float width,unsigned segments,BoatTint tint){
        if(segments<8u||segments>32u||radius<=width||width<=0.f)
            throw std::invalid_argument("Sitka boat ring dimensions");
        constexpr float tau=6.2831853071795864769f;
        for(unsigned i=0;i<segments;++i){
            const float a=tau*float(i)/segments,b=tau*float(i+1u)/segments;
            const float ca=std::cos(a),sa=std::sin(a);
            const float cb=std::cos(b),sb=std::sin(b);
            const float inner=radius-width;
            Quad({center_x+radius*ca,center_y+radius*sa,z},
                 {center_x+radius*cb,center_y+radius*sb,z},
                 {center_x+inner*cb,center_y+inner*sb,z},
                 {center_x+inner*ca,center_y+inner*sa,z},tint);
        }
    }
    void Hull(const DeckBounds& bounds,unsigned tier){
        // Open deck and tapered multisection hull: no giant solid cuboid or
        // forward windshield obstructing the seated player's sightline.
        const float bow=bounds.bow,stern=bounds.stern;
        const std::array<float,5> z={{stern,stern*.55f,-.15f,bow*.58f,bow}};
        const std::array<float,5> factor={{.40f,.88f,1.f,.81f,.34f}};
        const float w=bounds.half_width;
        const BoatTint plating=tier==3u?BoatTint{89,106,117}:
                               tier>=1u?BoatTint{186,201,204}:kHullColor;
        for(unsigned i=0;i<4u;++i){
            const float p=w*factor[i],q=w*factor[i+1u];
            const float a=z[i],b=z[i+1u];
            // A shaped, walkable deck connects to the safety rail, leaving
            // permanent boat-local walking coordinates across hull tiers.
            Quad({-p,-.59f,a},{p,-.59f,a},
                 {q,-.59f,b},{-q,-.59f,b},kDeckColor);
            for(int side:{-1,1}){
                const float x0=float(side)*p,x1=float(side)*q;
                Quad({x0,-.56f,a},{x1,-.56f,b},
                     {x1,-.12f,b},{x0,-.12f,a},plating);
                Quad({x0,-.56f,a},{0.f,-1.17f,a},
                     {0.f,-1.17f,b},{x1,-.56f,b},plating);
                // Two visible, continuous gunwale bands, rather than
                // a hard invisible collision rectangle alone.
                Quad({x0,-.12f,a},{x1,-.12f,b},
                     {x1,-.05f,b},{x0,-.05f,a},kRailColor);
            }
        }
        // Short stern and bow caps protect a walking passenger.
        Box(-w*.36f,-.58f,stern-.035f,w*.36f,-.13f,stern+.035f,plating);
        Box(-w*.30f,-.58f,bow-.035f,w*.30f,-.15f,bow+.035f,plating);
    }
    void Helm(){
        Box(-.045f,-.51f,-.05f,.045f,.04f,.05f,kSteelColor);
        Box(-.095f,-.03f,.07f,.095f,.03f,.18f,kBrassColor);
        RingXY(0.f,0.f,.175f,.235f,.028f,16u,kSteelColor);
        Box(-.19f,-.018f,.166f,.19f,.018f,.185f,kSteelColor);
        Box(-.018f,-.19f,.166f,.018f,.19f,.185f,kSteelColor);
        // Compass housing, deliberately without a misleading fixed
        // north-pointing needle; a heading-correct card needs a separate
        // pivot/rotation before being accepted as navigational feedback.
        Box(-.115f,.28f,-.02f,.115f,.335f,.18f,kBrassColor);
        Box(-.087f,.335f,.012f,.087f,.34f,.15f,kDarkScreen);
    }
    void Installed(HelmItem item,const HelmPart& part,const DeckBounds& deck){
        const float x=part.x,y=part.y,z=part.z;
        switch(item){
        case HelmItem::Battery:
            Box(-.72f,-.34f,.21f,.72f,-.21f,.47f,kSteelColor);
            break;
        case HelmItem::Sonar: case HelmItem::Gps: case HelmItem::Radar:
            Box(x-.19f,y-.13f,z-.035f,x+.19f,y+.14f,z+.05f,kSteelColor);
            Box(x-.155f,y-.10f,z-.04f,x+.155f,y+.10f,z-.036f,
                kDarkScreen); // unlit until actual display text exists
            break;
        case HelmItem::Lights: case HelmItem::Foghorn: case HelmItem::Wipers:
            Box(x-.08f,y-.05f,z-.04f,x+.08f,y+.05f,z+.04f,kBrassColor);
            break;
        case HelmItem::Roof: {
            const float side=deck.half_width*.67f;
            for(int dir:{-1,1})
                for(float zz:{-.8f,deck.bow*.65f})
                    Box(float(dir)*side-.025f,-.48f,zz-.025f,
                        float(dir)*side+.025f,1.60f,zz+.025f,kSteelColor);
            Box(-side-.13f,1.60f,-.94f,side+.13f,1.67f,
                deck.bow*.65f+.11f,kRailColor);
            break;
        }
        case HelmItem::Overhead:
            Box(-.40f,1.43f,.50f,.40f,1.52f,.59f,kSteelColor);break;
        case HelmItem::MastCamera:
            Box(-.025f,1.60f,-.31f,.025f,2.27f,-.26f,kSteelColor);
            Box(-.12f,2.22f,-.42f,.12f,2.38f,-.21f,kDarkScreen);break;
        case HelmItem::RodRacks:
            for(int i=0;i<4;++i){
                const float zz=-.75f-.19f*i;
                Box(deck.half_width*.74f,-.43f,zz-.025f,
                     deck.half_width*.74f+.065f,.08f,zz+.025f,kSteelColor);
            }
            break;
        case HelmItem::Livewell:
            Box(-.34f,-.56f,-1.37f,.34f,-.26f,-1.03f,kSteelColor);break;
        case HelmItem::Outriggers:
            Box(-deck.half_width-.42f,.05f,-1.75f,
                 deck.half_width+.42f,.095f,-1.68f,kSteelColor);break;
        case HelmItem::Winch:
            Box(-.52f,-.45f,-3.62f,.52f,.22f,-2.95f,kSteelColor);break;
        case HelmItem::Outboard:
            Box(-.20f,-.80f,deck.stern-.39f,.20f,-.02f,
                deck.stern-.09f,kSteelColor);break;
        case HelmItem::Diesels:
            Box(-.65f,-.55f,-2.95f,.65f,-.27f,-2.12f,kSteelColor);break;
        }
    }
};
inline TerrainMesh BuildBoatMesh(const HelmProgress& progress){
    if(progress.hull>3u||progress.owned>65535u)
        throw std::invalid_argument("invalid Sitka purchased boat fittings");
    BoatMeshBuilder builder;
    const DeckBounds deck=kDecks[progress.hull];
    builder.Hull(deck,progress.hull);
    builder.Helm();
    for(unsigned i=0;i<kParts.size();++i)
        if(progress.Has(static_cast<HelmItem>(i)))
            builder.Installed(static_cast<HelmItem>(i),kParts[i],deck);
    if(builder.mesh.vertices.size()>4096u||
       builder.mesh.indices.size()>4096u)
        throw std::invalid_argument("Sitka boat mesh exceeds mobile budget");
    return std::move(builder.mesh);
}
// Construct from current world-space pose on the update/render boundary, NOT
// headset pose. This keeps geometry aligned with bathymetry local origin.
// Multiplication is column-major, compatible with ReverieNativeMat4Multiply.
inline std::array<float,16> BoatModelMatrix(const BoatPose& pose,
       double origin_east,double origin_north,double water_surface_y){
    if(!pose.launched||!std::isfinite(pose.east)||!std::isfinite(pose.north)||
       !std::isfinite(pose.heading)||!std::isfinite(origin_east)||
       !std::isfinite(origin_north)||!std::isfinite(water_surface_y)||
       std::abs(pose.east-origin_east)>20000.||
       std::abs(pose.north-origin_north)>20000.||
       std::abs(water_surface_y)>100.)
        throw std::invalid_argument("invalid Sitka boat world pose");
    const float c=static_cast<float>(std::cos(pose.heading));
    const float s=static_cast<float>(std::sin(pose.heading));
    return {{c,0.f,-s,0.f,
             0.f,1.f,0.f,0.f,
             s,0.f,c,0.f,
             static_cast<float>(pose.east-origin_east),
             static_cast<float>(water_surface_y+.40),
             static_cast<float>(pose.north-origin_north),1.f}};
}
}} // namespace reverie::sitka
#endif
