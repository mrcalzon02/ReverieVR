#include "sitka_shore_detail.h"
#include <cassert>
#include <stdexcept>
#include <string>
using namespace reverie::sitka;
int main(){
    const auto features=ParseShoreLayout(
        "SITKA-SHORE-1\n"
        "P|a_store|storefront|1|7|0,0;16,0;16,4;11,4;11,9;0,9\n"
        "P|b_cannery|cannery|0.5|9|18,0;37,0;37,10;18,10\n"
        "B|c_pier|pier|3|20|0.8|3|18|0.5|0\n"
        "R|d_bridge|bridge_ramp|40|0|2|9|24|0.8|0|4\n"
        "B|e_shack|harbor_shack|10|12|0|2|2|3|0\n");
    TerrainMesh base;
    AppendShoreGeometry(base,features);
    const auto before=base.vertices.size();
    const auto stat=AppendShoreDetails(base,features);
    assert(stat.windows>0 && stat.windows<=32u);
    assert(stat.canopies==1u && stat.plank_seams>0u);
    assert(stat.guardrails==2u && stat.shack_windows==1u);
    assert(base.vertices.size()>before);
    assert(base.indices.size()%3u==0u);
    // Build must be deterministic across context recreation and runs.
    TerrainMesh again;
    AppendShoreGeometry(again,features);
    const auto repeat=AppendShoreDetails(again,features);
    assert(repeat.windows==stat.windows);
    assert(base.indices==again.indices);
    assert(base.vertices.size()==again.vertices.size());
    for(size_t i=0;i<base.vertices.size();++i){
        const auto& a=base.vertices[i],&b=again.vertices[i];
        assert(a.x==b.x&&a.y==b.y&&a.z==b.z);
        assert(a.r==b.r&&a.g==b.g&&a.b==b.b);
    }
    const auto packed=PackTerrainGles2(base);
    assert(!packed.empty());
    // A real source outline remains unchanged: detail only appends vertices.
    assert(features[0].footprint[3].x==11.f);
    assert(features[0].footprint[3].z==4.f);
    // No fabricated facade details on natural rock.
    auto rocks=ParseShoreLayout("SITKA-SHORE-1\nB|rock0|rock|0|0|0|4|4|1|0\n");
    TerrainMesh raw;
    auto none=AppendShoreDetails(raw,rocks);
    assert(raw.vertices.empty()&&none.windows==0u);
    return 0;
}
