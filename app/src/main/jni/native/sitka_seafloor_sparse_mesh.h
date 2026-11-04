#ifndef REVERIE_SITKA_SEAFLOOR_SPARSE_MESH_H
#define REVERIE_SITKA_SEAFLOOR_SPARSE_MESH_H
// LOD-first seabed mesh: sample only needed GPU vertices, not every source
// pixel, while checking ALL covered original water-mask cells per triangle.
// Real hydrography wins, missing samples use deterministic procedural ocean.
// Never called from a stereo eye render; CPU-only input to PrepareFrame.
#include "sitka_procedural_seabed.h"
#include "sitka_seafloor_mesh.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace reverie { namespace sitka {
struct SeafloorSparseMetrics {
    size_t sampled_vertices=0u;
    size_t surveyed_vertices=0u;
    size_t generated_vertices=0u;
    size_t discarded_vertices=0u;
    size_t checked_water_cells=0u;
    size_t emitted_triangles=0u;
    size_t blocked_quads=0u;
};
template<class InWater>
inline TerrainMesh BuildHybridSeafloorLod(
    const BathyTile& source,const ProceduralSeabed& generator,
    double projected_origin_east,double projected_origin_north,
    unsigned lod_stride,InWater&& in_water,
    SeafloorSparseMetrics* metrics=nullptr){
    if(source.columns<2u||source.columns>kBathyMaxAxis||
       source.rows<2u||source.rows>kBathyMaxAxis||
       source.qualities.size()!=size_t(source.columns)*source.rows||
       source.elevations_m.size()!=source.qualities.size()||
       source.datum_id!=generator.settings().datum_id||
       !std::isfinite(source.first_east)||!std::isfinite(source.first_north)||
       !std::isfinite(source.east_step)||!std::isfinite(source.north_step)||
       source.east_step<=0.||source.north_step<=0.||
       !std::isfinite(projected_origin_east)||
       !std::isfinite(projected_origin_north)||
       lod_stride<1u||lod_stride>16u||
       (lod_stride&(lod_stride-1u))!=0u)
        throw std::invalid_argument("invalid Sitka sparse seabed mesh configuration");
    const auto grid=[&](unsigned count){
        std::vector<unsigned> axis;
        for(unsigned i=0u;i<count-1u;i+=lod_stride)axis.push_back(i);
        if(axis.empty()||axis.back()!=count-1u)axis.push_back(count-1u);
        return axis;
    };
    const auto xs=grid(source.columns),ys=grid(source.rows);
    if(xs.size()*ys.size()>kMaxTerrainVertices)
        throw std::invalid_argument("Sitka sparse seabed vertex limit");
    SeafloorSparseMetrics local;
    TerrainMesh mesh;
    mesh.origin_east=projected_origin_east;
    mesh.origin_north=projected_origin_north;
    mesh.vertices.reserve(xs.size()*ys.size());
    const auto east_at=[&](unsigned col){
        return source.first_east+source.east_step*col;
    };
    const auto north_at=[&](unsigned row){
        return source.first_north-source.north_step*row;
    };
    const auto source_id=[&](unsigned col,unsigned row){
        return size_t(row)*source.columns+col;
    };
    for(unsigned row:ys)for(unsigned col:xs){
        const double x=east_at(col),z=north_at(row);
        const bool wet=in_water(x,z);
        const size_t i=source_id(col,row);
        BedQuality quality=wet?source.qualities[i]:BedQuality::Missing;
        float elevation=0.f;
        if(wet){
            if(quality==BedQuality::Missing){
                const BedSample sample=generator.Sample(x,z);
                if(sample.known){
                    quality=BedQuality::Procedural;
                    elevation=static_cast<float>(sample.elevation_m);
                    ++local.generated_vertices;
                }
            }else{
                elevation=source.elevations_m[i];
                ++local.surveyed_vertices;
            }
        }
        if(quality==BedQuality::Missing)++local.discarded_vertices;
        const double east=x-projected_origin_east;
        const double north=z-projected_origin_north;
        if(!std::isfinite(east)||!std::isfinite(north)||
           std::abs(east)>1.e5||std::abs(north)>1.e5||
           !std::isfinite(elevation))
            throw std::invalid_argument("bad Sitka sparse mesh world position");
        const bool shallow=elevation> -25.f;
        const bool composite=quality==BedQuality::Composite;
        const bool procedural=quality==BedQuality::Procedural;
        mesh.vertices.push_back({static_cast<float>(east),elevation,
            static_cast<float>(north),
            static_cast<uint8_t>(procedural?69u:composite?89u:shallow?92u:63u),
            static_cast<uint8_t>(procedural?101u:composite?92u:shallow?123u:88u),
            static_cast<uint8_t>(procedural?117u:composite?94u:shallow?111u:106u)});
        ++local.sampled_vertices;
    }
    const size_t width=xs.size();
    for(size_t row=0;row+1u<ys.size();++row)
        for(size_t col=0;col+1u<xs.size();++col){
            // A tiny authored island, shoal above water or a water-mask
            // hole MUST block the whole decimated quad. Testing only the
            // four visible corners can draw underwater land incorrectly.
            bool wet=true;
            for(unsigned rr=ys[row];wet&&rr<=ys[row+1];++rr)
                for(unsigned cc=xs[col];cc<=xs[col+1];++cc){
                    ++local.checked_water_cells;
                    if(!in_water(east_at(cc),north_at(rr))){
                        wet=false;break;
                    }
                }
            if(!wet){++local.blocked_quads;continue;}
            const uint32_t a=static_cast<uint32_t>(row*width+col);
            const uint32_t b=static_cast<uint32_t>(row*width+col+1u);
            const uint32_t c=static_cast<uint32_t>((row+1u)*width+col);
            const uint32_t d=static_cast<uint32_t>((row+1u)*width+col+1u);
            mesh.indices.insert(mesh.indices.end(),{a,c,b,b,c,d});
            local.emitted_triangles+=2u;
            if(local.emitted_triangles>kMaxTerrainTriangles)
                throw std::invalid_argument("Sitka sparse mesh triangle limit");
        }
    if(metrics)*metrics=local;
    return mesh;
}
}} // namespace
#endif
