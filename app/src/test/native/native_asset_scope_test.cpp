#include "../../main/jni/native/reverie_native_asset_scope.h"
#include <cassert>
#include <string>
using reverie::native::assets::MaximumBytes;
int main(){
    const char *red="between-deliveries-red-ledger";
    const char *omega="omega-front";
    const char *gumball="gumball-armada";
    const char *candyRoot="native/gumball/nodes/";
    const char* const candyTiles[]={
        "gummy_world.rgb565","chocolate_world.rgb565",
        "ice_candy_world.rgb565","citrus_world.rgb565",
        "gas_giant.rgb565","ring_world.rgb565",
        "outpost_station.rgb565","crystal_field.rgb565"};
    for(const char* tile:candyTiles){
        const std::string asset=std::string(candyRoot)+tile;
        assert(MaximumBytes(gumball,asset.c_str())==16520u);
        assert(MaximumBytes(red,asset.c_str())==0u);
        assert(MaximumBytes(omega,asset.c_str())==0u);
    }
    assert(MaximumBytes(gumball,"native/gumball/nodes/../gummy_world.rgb565")==0u);
    assert(MaximumBytes(gumball,"native/gumball/nodes/unknown.rgb565")==0u);
    assert(MaximumBytes(gumball,"native/gumball/nodes/gummy_world.png")==0u);
    assert(MaximumBytes(gumball,"native/gumball/nodes/sub/gummy_world.rgb565")==0u);
    assert(MaximumBytes(gumball,"native/gumball/nodes/gummy_world.rgb565.bak")==0u);
    assert(MaximumBytes(gumball,"native/omega-front/terrain/ground.png")==0u);
    assert(MaximumBytes(red,"native/red-ledger/beckenburg/wall.rlprop")==3145728u);
    assert(MaximumBytes(red,"native/red-ledger/beckenburg/light_on.rlprop")>0u);
    assert(MaximumBytes(red,"native/red-ledger/icons/source.png")==1048576u);
    assert(MaximumBytes(red,"native/red-ledger/models/source.fbx")>0u);
    assert(MaximumBytes(red,"native/red-ledger/textures/source.png")>0u);
    assert(MaximumBytes(omega,"native/omega-front/terrain/ground.png")>0u);
    assert(MaximumBytes(omega,"native/omega-front/units/kenney/tank.png")>0u);
    assert(MaximumBytes(red,"native/omega-front/terrain/ground.png")==0u);
    assert(MaximumBytes(omega,"native/red-ledger/beckenburg/wall.rlprop")==0u);
    assert(MaximumBytes(red,"native/red-ledger/beckenburg/wall.so")==0u);
    assert(MaximumBytes(red,"native/red-ledger/beckenburg/wall.rlprop.txt")==0u);
    assert(MaximumBytes(red,"native/red-ledger/beckenburg/../wall.rlprop")==0u);
    assert(MaximumBytes(red,"native/red-ledger/beckenburg/subdir/wall.rlprop")==0u);
    assert(MaximumBytes(red,"native/red-ledger/beckenburg/")==0u);
    assert(MaximumBytes("not-a-module","native/red-ledger/models/a.fbx")==0u);
    assert(MaximumBytes(nullptr,"native/red-ledger/models/a.fbx")==0u);
    assert(MaximumBytes(red,nullptr)==0u);
}
