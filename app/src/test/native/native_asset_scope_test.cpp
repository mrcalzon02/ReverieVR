#include "../../main/jni/native/reverie_native_asset_scope.h"
#include <cassert>
using reverie::native::assets::MaximumBytes;
int main(){
    const char *red="between-deliveries-red-ledger";
    const char *omega="omega-front";
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
