#ifndef REVERIE_SITKA_SEAFLOOR_MESH_H
#define REVERIE_SITKA_SEAFLOOR_MESH_H
// Source-derived low-poly seafloor geometry. Never fabricate floors between
// missing soundings; no triangles can bridge missing samples during LOD.
#include "sitka_bathymetry.h"
#include "sitka_terrain_mesh.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace reverie { namespace sitka {
inline TerrainMesh BuildSeafloorTerrain(const BathyTile& bathy,
    double projected_origin_east,double projected_origin_north,
    unsigned decimation=0u) {
    if(bathy.columns<2u||bathy.rows<2u||
       bathy.elevations_m.size()!=size_t(bathy.columns)*bathy.rows||
       bathy.qualities.size()!=bathy.elevations_m.size()||
       !std::isfinite(projected_origin_east)||
       !std::isfinite(projected_origin_north))
        throw std::invalid_argument("invalid Sitka seafloor mesh input");
    if(decimation==0u)
        decimation=(bathy.columns>512u||bathy.rows>512u||
            size_t(bathy.columns-1u)*size_t(bathy.rows-1u)*2u>
                kMaxTerrainTriangles)?2u:1u;
    if(decimation>16u||(decimation&(decimation-1u))!=0u)
        throw std::invalid_argument("invalid bathymetry mesh LOD stride");
    const auto grid=[](unsigned count,unsigned stride){
        std::vector<unsigned> values;
        for(unsigned i=0;i<count-1u;i+=stride)values.push_back(i);
        if(values.empty()||values.back()!=count-1u)values.push_back(count-1u);
        return values;
    };
    const auto xs=grid(bathy.columns,decimation);
    const auto ys=grid(bathy.rows,decimation);
    if(xs.size()*ys.size()>kMaxTerrainVertices)
        throw std::invalid_argument("seafloor mesh exceeds indexed vertex budget");
    TerrainMesh mesh;
    mesh.origin_east=projected_origin_east;
    mesh.origin_north=projected_origin_north;
    mesh.vertices.reserve(xs.size()*ys.size());
    const auto id=[&](unsigned c,unsigned row){
        return size_t(row)*bathy.columns+c;
    };
    for(unsigned row:ys)for(unsigned col:xs){
        const size_t i=id(col,row);
        const double east=bathy.first_east+col*bathy.east_step-projected_origin_east;
        const double north=bathy.first_north-row*bathy.north_step-projected_origin_north;
        const float height=bathy.qualities[i]==BedQuality::Missing?
            0.f:bathy.elevations_m[i];
        if(!std::isfinite(east)||!std::isfinite(north)||
           std::abs(east)>100000.||std::abs(north)>100000.)
            throw std::invalid_argument("seafloor origin cannot be represented");
        // Distinctive but economical original PSX seabed shades, no textures.
        const bool shallow=height> -25.f;
        const bool composite=bathy.qualities[i]==BedQuality::Composite;
        const uint8_t red=composite?89u:shallow?92u:63u;
        const uint8_t green=composite?92u:shallow?123u:88u;
        const uint8_t blue=composite?94u:shallow?111u:106u;
        mesh.vertices.push_back({static_cast<float>(east),height,
            static_cast<float>(north),red,green,blue});
    }
    const size_t width=xs.size();
    for(size_t row=0;row+1u<ys.size();++row)
        for(size_t col=0;col+1u<xs.size();++col){
            // Conservative: a decimated cell is emitted only when *every*
            // underlying source pixel has coverage. This prevents triangles
            // bridging holes or unmeasured seabed at reduced LOD.
            bool covered=true;
            for(unsigned rr=ys[row];covered&&rr<=ys[row+1u];++rr)
                for(unsigned cc=xs[col];cc<=xs[col+1u];++cc)
                    if(bathy.qualities[id(cc,rr)]==BedQuality::Missing){
                        covered=false;break;
                    }
            if(!covered)continue;
            const uint32_t a=static_cast<uint32_t>(row*width+col);
            const uint32_t b=static_cast<uint32_t>(row*width+col+1u);
            const uint32_t c=static_cast<uint32_t>((row+1u)*width+col);
            const uint32_t d=static_cast<uint32_t>((row+1u)*width+col+1u);
            mesh.indices.insert(mesh.indices.end(),{a,c,b,b,c,d});
            if(mesh.indices.size()/3u>kMaxTerrainTriangles)
                throw std::invalid_argument("too many seabed triangles");
        }
    return mesh;
}
}}
#endif
