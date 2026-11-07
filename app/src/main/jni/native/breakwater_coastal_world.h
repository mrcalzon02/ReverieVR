#ifndef REVERIE_BREAKWATER_COASTAL_WORLD_H
#define REVERIE_BREAKWATER_COASTAL_WORLD_H
#include "breakwater_battlefield.h"
#include "breakwater_environment.h"
#include <vector>

namespace reverie { namespace breakwater { namespace battlefield {
// Shared rows give a crack-free grid with small near cells and sparse far
// cells. Only visible upper land surfaces exist: no buried sides/bottoms.
struct WorldBudget { size_t textured=0,colored=0; };
template<class Triangle>
WorldBudget Coast(uint8_t location,bool night,std::vector<float> &textured,
                  Triangle triangle) {
    WorldBudget budget{};
    if(location>=Campaign::kLocations)return budget;
    const auto palette=EnvironmentFor(location,night);
    for(size_t iz=1;iz<std::size(kGroundZ);++iz)
        for(size_t ix=1;ix<std::size(kGroundX);++ix) {
            const float x0=kGroundX[ix-1],x1=kGroundX[ix];
            const float z0=kGroundZ[iz-1],z1=kGroundZ[iz];
            // Cells wholly below the sea stay unbuilt; the last submerged
            // row is retained to prevent shoreline holes as waves recede.
            if(z1<std::min(ShoreZ(location,x0),ShoreZ(location,x1))-16.f)continue;
            const materials::Point a{x0,Ground(location,x0,z0),z0};
            const materials::Point b{x1,Ground(location,x1,z0),z0};
            const materials::Point c{x1,Ground(location,x1,z1),z1};
            const materials::Point d{x0,Ground(location,x0,z1),z1};
            const float distance=std::max(std::abs((x0+x1)*.5f),std::abs((z0+z1)*.5f));
            if(distance<136.f) {
                const auto surface=z0>8.f?materials::Surface::TrenchSoil:
                    z0>-12.f?materials::Surface::PebbleGround:
                              materials::CoastSurface(location);
                // One 64px tile per 2-4m near cell; 8-32m distant cells use
                // intentionally lower density, not hundreds of tiny repeats.
                materials::Face(textured,surface,a,b,c,d);
                budget.textured+=2;
            } else {
                const auto col=z0<0?palette.hill:palette.earth;
                triangle(a.x,a.y,a.z,b.x,b.y,b.z,c.x,c.y,c.z,col.r,col.g,col.b);
                triangle(a.x,a.y,a.z,c.x,c.y,c.z,d.x,d.y,d.z,col.r*.96f,col.g*.96f,col.b*.96f);
                budget.colored+=2;
            }
        }
    // Low polygon physical hills remain behind/alongside the coast; no
    // close landscape cylinder surrounds the gun. Far sky is separate.
    for(int ringIndex=0;ringIndex<2;++ringIndex)EmitLandscapeRing(location,night,ringIndex!=0,
        [&](float x,float y,float z,float a,float b,float c,
            float d,float e,float f,float r,float g,float blue) {
            triangle(x*22.f,y*22.f,z*22.f,a*22.f,b*22.f,c*22.f,
                     d*22.f,e*22.f,f*22.f,r,g,blue);budget.colored++;
        });
    return budget;
}
constexpr int kSeaAcross=24,kSeaRows=28;
constexpr float kSeaFar=-3400.f;
inline float SeaHeight(float x,float z,float elapsed) {
    const float offshore=std::clamp((kShoreZ-z)/150.f,0.f,1.f);
    const float amplitude=.08f+.28f*offshore;
    return kSeaY+amplitude*(.65f*std::sin(x*.055f+z*.038f+elapsed*.8f)+
                           .35f*std::sin(x*.09f-z*.024f+elapsed*.52f));
}
template<class Triangle>
void Sea(uint8_t location,bool night,float elapsed,Triangle triangle) {
    if(location>=Campaign::kLocations || !std::isfinite(elapsed))return;
    const auto color=EnvironmentFor(location,night).sea;
    for(int row=0;row<kSeaRows;++row)for(int col=0;col<kSeaAcross;++col) {
        const auto point=[&](int i,int j) {
            const float t=float(j)/kSeaRows,u=2.f*i/kSeaAcross-1.f;
            const float x=u*(512.f+2800.f*t*t);
            const float shore=ShoreZ(location,x);
            const float z=shore+(kSeaFar-shore)*t*t;
            return materials::Point{x,SeaHeight(x,z,elapsed),z};
        };
        const auto a=point(col,row),b=point(col+1,row);
        const auto c=point(col+1,row+1),d=point(col,row+1);
        const float facet=(row+col*3)%5==0?.93f:1.f;
        triangle(a.x,a.y,a.z,b.x,b.y,b.z,c.x,c.y,c.z,
                 color.r*facet,color.g*facet,color.b*facet);
        triangle(a.x,a.y,a.z,c.x,c.y,c.z,d.x,d.y,d.z,
                 color.r*facet*.96f,color.g*facet*.98f,color.b*facet);
    }
}
// Deterministic geometry LOD from the selected station, never per-eye.
enum class Detail { Full, Reduced, Distant };
inline Detail DetailAt(float x,float z,defense::Position station) {
    const float d=std::hypot(x-station.x,z-station.z);
    return d<180.f?Detail::Full:d<600.f?Detail::Reduced:Detail::Distant;
}
// Lowest tier stays a real 3D hull/airframe, with no image billboard swap.
template<class Triangle>
void DistantVessel(const Enemy &e,Triangle triangle) {
    const float x=e.x,z=e.z;
    const float y=e.type==UnitType::Aircraft?1.6f:-.63f;
    const float width=e.type==UnitType::Aircraft?1.25f:.65f;
    const float length=e.type==UnitType::ArmoredTransport?1.8f:.95f;
    const float red=e.type==UnitType::Aircraft?.40f:.28f;
    const float green=e.type==UnitType::Aircraft?.45f:.35f;
    const float blue=e.type==UnitType::Aircraft?.43f:.33f;
    const float points[6][3]={{x-width,y,z},{x+width,y,z},
        {x,y+.30f,z},{x,y-.30f,z},{x,y,z-length},{x,y,z+length}};
    constexpr int faces[8][3]={{0,2,4},{2,1,4},{1,3,4},{3,0,4},
                               {2,0,5},{1,2,5},{3,1,5},{0,3,5}};
    for(const auto &face:faces) {
        const auto &a=points[face[0]],&b=points[face[1]],&c=points[face[2]];
        triangle(a[0],a[1],a[2],b[0],b[1],b[2],c[0],c[1],c[2],red,green,blue);
    }
}
}}}
#endif
