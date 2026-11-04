#include "../../main/jni/native/sitka_seafloor_sparse_mesh.h"
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
using namespace reverie::sitka;
static BathyTile Source(){
    BathyTile tile;
    tile.columns=513u;tile.rows=513u;
    tile.first_east=0.;tile.first_north=512.;
    tile.east_step=1.;tile.north_step=1.;
    tile.datum_id=BathyDatumId("NAVD88");
    const size_t total=size_t(tile.columns)*tile.rows;
    tile.elevations_m.assign(total,-36.f);
    tile.qualities.assign(total,BedQuality::Survey);
    // Exactly one sampled LOD vertex is an unmeasured survey hole.
    const size_t hole=128u*tile.columns+128u;
    tile.elevations_m[hole]=0.f;
    tile.qualities[hole]=BedQuality::Missing;
    // One additional unsurveyed cell should NOT be procedurally evaluated
    // because it is not an LOD4 vertex. Coarse quad remains rendered.
    const size_t unneeded=130u*tile.columns+130u;
    tile.elevations_m[unneeded]=0.f;
    tile.qualities[unneeded]=BedQuality::Missing;
    return tile;
}
static bool IsInvalid(const BathyTile& tile,const ProceduralSeabed& g,
                      unsigned stride){
    try{(void)BuildHybridSeafloorLod(tile,g,0.,0.,stride,
                                     [](double,double){return true;});
        return false;
    }catch(const std::invalid_argument&){return true;}
}
int main(){
    SeabedGeneration config;
    config.datum_id=BathyDatumId("NAVD88");
    config.seed=123456789u;
    config.water_reference_m=0.;
    config.influence_radius_m=500.;
    config.guides={{0.,0.,-25.},{512.,512.,-60.}};
    ProceduralSeabed generator(config);
    const BathyTile tile=Source();
    const auto water=[](double,double){return true;};
    const auto start=std::chrono::steady_clock::now();
    SeafloorSparseMetrics metrics{};
    const auto mesh=BuildHybridSeafloorLod(tile,generator,0.,0.,4u,
                                           water,&metrics);
    const auto micros=std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now()-start).count();
    assert(mesh.vertices.size()==129u*129u);
    assert(mesh.indices.size()==128u*128u*6u);
    assert(metrics.sampled_vertices==129u*129u);
    assert(metrics.surveyed_vertices==129u*129u-1u);
    assert(metrics.generated_vertices==1u);
    assert(metrics.discarded_vertices==0u);
    assert(metrics.emitted_triangles==128u*128u*2u);
    assert(metrics.blocked_quads==0u);
    const size_t hole_vertex=32u*129u+32u;
    const auto expected=generator.Sample(128.,384.);
    assert(std::abs(mesh.vertices[hole_vertex].y-expected.elevation_m)<1.e-5);
    assert(mesh.vertices[hole_vertex].r==69u); // PROCEDURAL
    assert(mesh.vertices[0].y==-36.f);
    assert(mesh.vertices[0].r!=69u); // original surveyed cell intact
    auto gpu=PackTerrainGles2(mesh);
    assert(gpu.size()==1u&&gpu[0].vertices.size()==129u*129u);
    const size_t payload=mesh.vertices.size()*
            sizeof(TerrainBatch::Vertex)+mesh.indices.size()*sizeof(uint16_t);
    assert(payload<512u*1024u); // source tile LOD4 ~463 KiB GPU payload
    // Deep inland pixel inside one 4x4 source cell blocks its whole quad,
    // even though all four visible LOD grid corners are offshore.
    const auto island=[](double east,double north){
        return !(east==2.&&north==510.);
    };
    SeafloorSparseMetrics clipped{};
    const auto coast=BuildHybridSeafloorLod(tile,generator,0.,0.,4u,
                                            island,&clipped);
    assert(coast.indices.size()==mesh.indices.size()-6u);
    assert(clipped.blocked_quads==1u);
    assert(clipped.checked_water_cells>=mesh.vertices.size());
    // No source data is required for valid GAME geometry below known water.
    auto none=tile;
    none.qualities.assign(none.qualities.size(),BedQuality::Missing);
    none.elevations_m.assign(none.elevations_m.size(),0.f);
    SeafloorSparseMetrics generated{};
    const auto fictional=BuildHybridSeafloorLod(
        none,generator,0.,0.,4u,water,&generated);
    assert(fictional.indices.size()==mesh.indices.size());
    assert(generated.generated_vertices==129u*129u);
    assert(generated.surveyed_vertices==0u);
    const auto rerun=BuildHybridSeafloorLod(tile,generator,0.,0.,4u,water);
    assert(rerun.indices==mesh.indices);
    assert(rerun.vertices[hole_vertex].y==mesh.vertices[hole_vertex].y);
    assert(IsInvalid(tile,generator,3u));
    assert(IsInvalid(tile,generator,1u)); // 513² > bounded input mesh
    auto wrong=tile;wrong.datum_id=BathyDatumId("MLLW");
    assert(IsInvalid(wrong,generator,4u));
    std::cout<<"SPARSE_MESH_METRICS source_pixels="<<size_t(tile.rows)*tile.columns
             <<" sampled_vertices="<<metrics.sampled_vertices
             <<" survey_vertices="<<metrics.surveyed_vertices
             <<" generated_vertices="<<metrics.generated_vertices
             <<" tris="<<metrics.emitted_triangles
             <<" gpu_payload_bytes="<<payload
             <<" cpu_build_microseconds_ci="<<micros<<"\n";
    std::cout<<"PASS Sitka sampled-only LOD, survey priority, generated ocean, small-island masking and bounded GPU payload\n";
}
