#include "../../main/jni/native/sitka_seafloor_mesh.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace reverie::sitka;
static BathyTile Fixture(unsigned width=3u,unsigned height=3u) {
    BathyTile b;
    b.columns=width;b.rows=height;
    b.first_east=1000.;b.first_north=3000.;
    b.east_step=10.;b.north_step=10.;
    b.datum_id=BathyDatumId("NAVD88");
    b.elevations_m.assign(size_t(width)*height,-8.f);
    b.qualities.assign(b.elevations_m.size(),BedQuality::Survey);
    return b;
}
static bool Throws(const BathyTile& b,unsigned stride=0u){
    try{(void)BuildSeafloorTerrain(b,1000.,3000.,stride);return false;}
    catch(const std::invalid_argument&){return true;}
}
int main(){
    auto bed=Fixture();
    const auto mesh=BuildSeafloorTerrain(bed,1000.,3000.);
    assert(mesh.vertices.size()==9u);
    assert(mesh.indices.size()==24u);
    assert(mesh.vertices[0].x==0.f&&mesh.vertices[0].z==0.f);
    assert(mesh.vertices[0].y==-8.f);
    assert(mesh.vertices[1].x==10.f&&mesh.vertices[1].z==0.f);
    assert(mesh.vertices[3].z==-10.f);
    const auto parts=PackTerrainGles2(mesh);
    assert(parts.size()==1u&&parts[0].vertices.size()==9u);
    assert(parts[0].indices.size()==24u);
    bed.qualities[8]=BedQuality::Missing;
    auto clipped=BuildSeafloorTerrain(bed,1000.,3000.);
    assert(clipped.indices.size()==18u);
    assert(Throws(bed,3u)); // only powers of two
    bed.qualities[4]=BedQuality::Missing;
    clipped=BuildSeafloorTerrain(bed,1000.,3000.,2u);
    assert(clipped.indices.empty()); // cannot span a missing interior cell
    bed.qualities[4]=BedQuality::Survey;
    bed.qualities[8]=BedQuality::Composite;
    clipped=BuildSeafloorTerrain(bed,1000.,3000.,2u);
    assert(clipped.vertices.size()==4u&&clipped.indices.size()==6u);
    assert(clipped.vertices[3].r!=clipped.vertices[0].r);
    assert(Throws(bed,32u));
    assert(Throws(bed,0xFFFFFFFFu));
    auto huge=Fixture(513u,513u);
    const auto lod=BuildSeafloorTerrain(huge,1000.,3000.);
    assert(lod.vertices.size()==257u*257u);
    assert(lod.indices.size()==256u*256u*6u);
    auto gpu=PackTerrainGles2(lod);
    assert(gpu.size()>1u); // strict 16-bit per batch
    for(const auto& part:gpu)
        assert(part.vertices.size()<=kMaxGles2BatchVertices);
    huge.qualities[256u*huge.columns+256u]=BedQuality::Missing;
    const auto gap=BuildSeafloorTerrain(huge,1000.,3000.);
    assert(gap.indices.size()<lod.indices.size());
    // Identical approved bathymetry yields byte-for-byte stable PSX geometry.
    auto again=BuildSeafloorTerrain(Fixture(),1000.,3000.);
    assert(again.indices==mesh.indices&&again.vertices[0].r==mesh.vertices[0].r);
    std::cout<<"PASS real-data low-poly seabed mesh, hole-safe LOD, source origin and GLES2 chunking\n";
}
