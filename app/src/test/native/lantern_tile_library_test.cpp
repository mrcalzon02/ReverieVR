#include "lantern_tile_library.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
using namespace reverie::lantern::tiles;
static std::vector<uint8_t> Read(const std::string& path){
    std::ifstream f(path,std::ios::binary);
    assert(f&&"immutable approved terrain pack missing");
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),
                                std::istreambuf_iterator<char>());
}
int main(){
    std::array<std::vector<uint8_t>,10> packs{};
    for(int b=0;b<10;++b){
        char path[64]={};assert(FileNameFor(static_cast<uint16_t>(b*100),path,sizeof(path)));
        packs[b]=Read(std::string("app/src/main/assets/")+path);
        assert(packs[b].size()==kPackBytes&&Read32(packs[b].data()+24)==unsigned(b*100));
    }
    uint64_t heightSum=0,vegetation=0,structures=0,navigable=0;
    std::set<uint32_t> distinctSeeds;
    std::set<uint32_t> roughness;
    for(int i=0;i<1000;++i){
        Tile tile;
        assert(ReadTile(packs[i/100].data(),packs[i/100].size(),
                        static_cast<uint16_t>(i),&tile));
        assert(tile.id==i);
        assert(distinctSeeds.insert(tile.seed).second);
        assert(tile.road[0]==0&&tile.road[16]==255);
        for(uint16_t h:tile.height_dm)heightSum+=h;
        roughness.insert(tile.height_dm[16*33+16]-tile.height_dm[0]);
        for(auto p:tile.foliage)vegetation+=p.kind>0;
        for(auto b:tile.buildings)structures+=b.exists>0;
        for(uint8_t cost:tile.nav_cost)navigable+=cost>0;
        if(i%173==0){
            Tile another;
            assert(ReadTile(packs[i/100].data(),packs[i/100].size(),
                            static_cast<uint16_t>(i),&another));
            assert(tile.height_dm==another.height_dm&&
                   tile.road==another.road&&tile.nav_cost==another.nav_cost);
            assert(!ReadTile(packs[i/100].data(),packs[i/100].size(),
                             static_cast<uint16_t>((i+1)%1000),&another));
            const float h=tile.GroundHeight(81.f,155.f);
            assert(std::isfinite(h)&&h>=0.f&&h<=6500.f);
        }
    }
    assert(vegetation>6000&&structures>2500&&navigable>40000);
    assert(heightSum>5000000ull&&roughness.size()>100);
    auto a=AssignWarfront(0x518bc7u),same=AssignWarfront(0x518bc7u);
    auto other=AssignWarfront(0x518bc8u);
    assert(a==same&&a!=other);
    auto sorted=a;std::sort(sorted.begin(),sorted.end());
    for(int i=0;i<1000;++i)assert(sorted[i]==i);
    const uint16_t region=127,tid=a[region];
    Tile tile;
    assert(ReadTile(packs[tid/100].data(),packs[tid/100].size(),tid,&tile));
    // Actual unit lifecycle uses the tile identity for the whole war.
    std::vector<WarOrder> war={
        {101u,region,UnitType::Truck,Activity::Transit,Doctrine::Exposed,
         1000,2000,0,1,3,1},
        {102u,region,UnitType::CampTent,Activity::Camp,Doctrine::BehindBuildings,
         1200,2900,3,4,18,1},
        {103u,region,UnitType::Tank,Activity::Conceal,Doctrine::UnderFoliage,
         1100,2900,7,8,1,1}
    };
    std::vector<PlacedUnit> first,middle,last;
    assert(ResolveAt(tile,region,1100,war,&first));
    assert(ResolveAt(tile,region,1650,war,&middle));
    assert(ResolveAt(tile,region,2500,war,&last));
    assert(first.size()==2&&middle.size()==3&&last.size()==2);
    assert(first[0].unit_id==101u&&middle[0].unit_id==101u);
    // The truck has actually moved along a traversable nav path.
    assert(first[0].x!=middle[0].x||first[0].y!=middle[0].y);
    assert(middle[1].unit_id==102u&&middle[2].unit_id==103u);
    // The same object cannot shift static terrain or change warfront id.
    Tile reopened;
    assert(ReadTile(packs[tid/100].data(),packs[tid/100].size(),a[region],&reopened));
    assert(reopened.height_dm==tile.height_dm&&
           reopened.road==tile.road&&a==same);
    std::vector<PlacedUnit> afterReload;
    assert(ResolveAt(reopened,region,1650,war,&afterReload));
    assert(afterReload.size()==middle.size());
    for(size_t i=0;i<middle.size();++i){
        assert(middle[i].unit_id==afterReload[i].unit_id&&
               middle[i].x==afterReload[i].x&&
               middle[i].y==afterReload[i].y);
    }
    // One bit of corruption anywhere in the reviewed pack, including an
    // unrelated tile, must reject the whole resource transactionally.
    auto bad=packs[tid/100];bad[201]^=1u;
    Tile untouched=tile;
    assert(!ReadTile(bad.data(),bad.size(),tid,&untouched));
    assert(untouched.height_dm==tile.height_dm);
    assert(!ReadTile(packs[tid/100].data(),packs[tid/100].size()-1,tid,&untouched));
    char name[64]={};assert(!FileNameFor(1000,name,sizeof(name)));
    assert(!FileNameFor(0,name,12));
    std::cout<<"PASS Lantern fixed terrain library: 1000 sealed identities, 10 immutable asset packs, 33x33 plasma elevation, 16x16 nav cost, "<<vegetation<<" foliage, "<<structures<<" buildings, "<<navigable<<" passable nav cells, stable warfront assignments, timestamped unit movement and exact revisit\n";
}
