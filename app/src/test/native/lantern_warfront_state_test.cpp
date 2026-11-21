#include "lantern_warfront_state.h"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
using namespace reverie::lantern;
static tiles::Tile Load(uint16_t id){
    char name[64];assert(tiles::FileNameFor(id,name,sizeof(name)));
    std::ifstream f(std::string("app/src/main/assets/")+name,std::ios::binary);
    assert(f);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),
                                std::istreambuf_iterator<char>());
    tiles::Tile tile;assert(tiles::ReadTile(bytes.data(),bytes.size(),id,&tile));
    return tile;
}
int main(){
    war::Campaign first;
    assert(first.Start(0x63119ad7u,1680000));
    assert(!first.Start(45u,1680100));
    assert(!first.Advance(1679999));
    assert(first.Advance(1690000));
    const uint16_t location=32u;
    uint16_t permanentTile=tiles::kUnassigned;
    assert(first.AssignedTile(location,&permanentTile));
    assert(permanentTile<1000);
    auto ground=Load(permanentTile);
    tiles::WarOrder convoy{1007u,location,tiles::UnitType::Truck,
        tiles::Activity::Transit,tiles::Doctrine::Exposed,
        1680100,1681800,0,1,4,1};
    tiles::WarOrder camp{1011u,location,tiles::UnitType::CampTent,
        tiles::Activity::Camp,tiles::Doctrine::BehindBuildings,
        1681200,1689400,3,4,20,1};
    assert(first.AddOrder(convoy)&&first.AddOrder(camp));
    assert(!first.AddOrder(convoy));
    std::vector<tiles::PlacedUnit> before,now,after;
    assert(first.Snapshot(location,1680700,ground,&before));
    assert(first.Snapshot(location,1681600,ground,&now));
    assert(first.Snapshot(location,1689100,ground,&after));
    assert(before.size()==1&&now.size()==2&&after.size()==1);
    assert(!first.Snapshot(location,1690001,ground,&after));
    assert(first.tile_at_location[location]==permanentTile);
    std::vector<uint8_t> file;
    assert(war::Encode(first,&file)&&file.size()==40u+2000u+64u+4u);
    war::Campaign loaded;
    assert(war::Decode(file.data(),file.size(),&loaded));
    assert(loaded.seed==first.seed&&loaded.current_at==first.current_at&&
           loaded.orders.size()==2&&loaded.tile_at_location==first.tile_at_location);
    auto again=Load(loaded.tile_at_location[location]);
    std::vector<tiles::PlacedUnit> repeated;
    assert(loaded.Snapshot(location,1681600,again,&repeated));
    assert(repeated.size()==now.size());
    for(size_t i=0;i<now.size();++i){
        assert(now[i].unit_id==repeated[i].unit_id);
        assert(now[i].x==repeated[i].x&&now[i].y==repeated[i].y);
    }
    assert(again.height_dm==ground.height_dm&&again.road==ground.road);
    const auto stable=file;
    auto invalid=file;invalid[40+location*2]^=1u;
    assert(!war::Decode(invalid.data(),invalid.size(),&loaded));
    assert(loaded.tile_at_location==first.tile_at_location);
    invalid=file;invalid[40+location*2]^=1u;
    war::Put32(invalid,invalid.size()-4,
               tiles::Fnv(invalid.data(),invalid.size()-4));
    assert(!war::Decode(invalid.data(),invalid.size(),&loaded));
    invalid=file;invalid[invalid.size()-1]^=0xffu;
    assert(!war::Decode(invalid.data(),invalid.size(),&loaded));
    assert(loaded.tile_at_location==first.tile_at_location);
    assert(!war::Decode(stable.data(),stable.size()-1,&loaded));
    assert(loaded.tile_at_location==first.tile_at_location);
    war::Campaign tampered=first;
    std::swap(tampered.tile_at_location[0],tampered.tile_at_location[1]);
    std::vector<uint8_t> unchanged={23u,7u};
    assert(!war::Encode(tampered,&unchanged));
    assert(unchanged==std::vector<uint8_t>({23u,7u}));
    // One unit can change location in a later non-overlapping movement
    // phase, but its already photographed route is never retroactively
    // replaced with the new assignment.
    tiles::WarOrder moved=convoy;
    moved.front_location=location+1;
    moved.enter_at=1683000;moved.depart_at=1686000;
    assert(first.AddOrder(moved));
    assert(first.orders.size()==3u);
    std::vector<tiles::PlacedUnit> historic;
    assert(first.Snapshot(location,1680700,ground,&historic));
    assert(historic.size()==1&&historic[0].unit_id==1007u);
    assert(first.tile_at_location[location]==permanentTile);
    std::cout<<"PASS Lantern warfront history: permanent 1000-place allocation, saved seed+mapping+orders, checked exact replay, time-ordered unit movement, corrupt state rejection and unchanged terrain\n";
}
