#include "../../main/jni/native/reverie_native_asset_scope.h"
#include <cassert>
#include <cstdio>
#include <string>
using reverie::native::assets::MaximumBytes;
int main(){
    const char *red="between-deliveries-red-ledger";
    const char *omega="omega-front";
    const char *gumball="gumball-armada";
    const char *sitka="sitka";
    constexpr uint32_t bathy_cap=56u+513u*513u*5u;
    const char* exact_sitka[]={
        "sitka/coast/terrain.sitkam",
        "sitka/lighting/sitka.properties",
        "sitka/weather/sitka.properties",
        "sitka/water/sitka.water",
        "sitka/fishing/sites.txt",
        "sitka/boat/launch.txt",
        "sitka/shore/waterfront.sitkash",
        "sitka/bathymetry/index.txt",
        "sitka/bathymetry/procedural-guides.txt"
    };
    for(const char* path:exact_sitka){
        assert(MaximumBytes(sitka,path)>0u);
        assert(MaximumBytes("omega-front",path)==0u);
        assert(MaximumBytes("breakwater-battery",path)==0u);
        assert(MaximumBytes("classic-chess",path)==0u);
    }
    assert(MaximumBytes(sitka,"sitka/bathymetry/bathymetry-r00-c00.sitkab")==bathy_cap);
    assert(MaximumBytes(sitka,"sitka/bathymetry/bathymetry-r99-c99.sitkab")==bathy_cap);
    const char* invalid_sitka[]={
        "sitka/bathymetry/bathymetry-r0-c00.sitkab",
        "sitka/bathymetry/bathymetry-rXX-c00.sitkab",
        "sitka/bathymetry/bathymetry-r00-c0X.sitkab",
        "sitka/bathymetry/bathymetry-r00-c00.sitkab.bak",
        "sitka/bathymetry/../boat/launch.txt",
        "sitka/bathymetry/bathymetry-r00-c00.sitkab/../other",
        "sitka/bathymetry/sub/bathymetry-r00-c00.sitkab",
        "sitka/boat/../../native/omega-front/units/tank.png",
        "sitka/boat/boat.sav",
        "sitka/shore/waterfront.sitkash.bak",
        "sitka/shore/../boat/launch.txt",
        "sitka/shore/unknown.sitkash",
        "sitka/coast/terrain.sitkam.bak",
        "sitka/water/other.water",
        "sitka/boat/launch.txt/",
        "native/omega-front/terrain/ground.png",
        "file:///etc/passwd",
        "../sitka/bathymetry/index.txt"
    };
    for(const char* path:invalid_sitka)
        assert(MaximumBytes(sitka,path)==0u);
    assert(MaximumBytes(sitka,nullptr)==0u);

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
    assert(MaximumBytes(gumball,"native/gumball/allies/pip_parfait.rgb565")==16520u);
    assert(MaximumBytes(gumball,"native/gumball/allies/lolly_wren.rgb565")==16520u);
    assert(MaximumBytes(red,"native/gumball/allies/pip_parfait.rgb565")==0u);
    assert(MaximumBytes(omega,"native/gumball/allies/lolly_wren.rgb565")==0u);
    assert(MaximumBytes(gumball,"native/gumball/allies/not_canonical.rgb565")==0u);
    assert(MaximumBytes(gumball,"native/gumball/allies/../pip_parfait.rgb565")==0u);
    assert(MaximumBytes(gumball,"native/gumball/nodes/gummy_world.png")==0u);
    assert(MaximumBytes(gumball,"native/gumball/nodes/sub/gummy_world.rgb565")==0u);
    assert(MaximumBytes(gumball,"native/gumball/nodes/gummy_world.rgb565.bak")==0u);
    assert(MaximumBytes(gumball,"native/omega-front/terrain/ground.png")==0u);
    // Shared assets are immutable exact paths with explicit opt-in modules.
    // They cannot authorize arbitrary foreign game-folder reads.
    const char* const accepted[]={"breakwater-battery","omega-front",
        "between-deliveries-red-ledger","procedural-test-chamber"};
    for(const auto& item:reverie::native::shared_assets::kEntries){
        for(const char* consumer:accepted) {
            const auto mask=reverie::native::shared_assets::ConsumerMask(consumer);
            const uint32_t expected=(item.consumers & mask)!=0u?
                item.max_bytes:0u;
            assert(MaximumBytes(consumer,item.path)==expected);
        }
        assert(MaximumBytes("gumball-armada",item.path)==0u);
        assert(MaximumBytes("classic-chess",item.path)==0u);
        const std::string traversal=std::string(item.path)+"/../evil.png";
        assert(MaximumBytes("breakwater-battery",traversal.c_str())==0u);
    }
    assert(MaximumBytes("omega-front",
        "native/shared/v1/materials/coastal/unknown.png")==0u);
    assert(MaximumBytes("breakwater-battery",
        "native/shared/v1/models/psx/owner_apc_armor.bwm")==0u);
    // Legacy read permissions are retained solely for old Breakwater packs.
    // Game-private authored material paths must not expand other module
    // capabilities or allow traversal/alternate extensions.
    const char* const breakwaterMaterials[]={
        "shingle.png","quay_concrete.png","black_basalt.png",
        "oxidized_steel.png","sandbag_burlap.png","olive_enamel.png"
    };
    for(const char* tile:breakwaterMaterials) {
        const std::string path=std::string("native/breakwater/materials/")+tile;
        assert(MaximumBytes("breakwater-battery",path.c_str())==16384u);
        assert(MaximumBytes(red,path.c_str())==0u);
        assert(MaximumBytes(omega,path.c_str())==0u);
    }
    assert(MaximumBytes("breakwater-battery",
                        "native/breakwater/materials/../shingle.png")==0u);
    assert(MaximumBytes("breakwater-battery",
                        "native/breakwater/materials/shingle.png.bak")==0u);
    assert(MaximumBytes("breakwater-battery",
                        "native/breakwater/materials/not_approved.png")==0u);
    assert(MaximumBytes("breakwater-battery",
                        "native/breakwater/materials/subdir/shingle.png")==0u);
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
    // Exactly the 64 licensed Omega Front RPG tiles, no other paths.
    for (unsigned i=0;i<64u;++i) {
        const char* family=i<32u?"tiny_battle_":
            i<48u?"rpg_urban_":"roguelike_city_";
        const unsigned slot=i<32u?i:i<48u?i-32u:i-48u;
        char path[128];
        std::snprintf(path,sizeof(path),
            "native/omega-front/rpg_cc0/%s%04u.png",family,slot);
        assert(MaximumBytes(omega,path)==256u*1024u);
        assert(MaximumBytes(red,path)==0u);
        assert(MaximumBytes(gumball,path)==0u);
    }
    assert(MaximumBytes(omega,"native/omega-front/rpg_cc0/tiny_battle_0032.png")==0u);
    assert(MaximumBytes(omega,"native/omega-front/rpg_cc0/rpg_urban_0016.png")==0u);
    assert(MaximumBytes(omega,"native/omega-front/rpg_cc0/roguelike_city_0016.png")==0u);
    assert(MaximumBytes(omega,"native/omega-front/rpg_cc0/tiny_battle_000A.png")==0u);
    assert(MaximumBytes(omega,"native/omega-front/rpg_cc0/tiny_battle_1.png")==0u);
    assert(MaximumBytes(omega,"native/omega-front/rpg_cc0/tiny_battle_0000.png.bak")==0u);
    assert(MaximumBytes(omega,"native/omega-front/rpg_cc0/other_0000.png")==0u);
    assert(MaximumBytes(omega,"native/omega-front/rpg_cc0/sub/tiny_battle_0000.png")==0u);
    assert(MaximumBytes(omega,"native/omega-front/rpg_cc0/../tiny_battle_0000.png")==0u);
    assert(MaximumBytes(omega,"native/red-ledger/rpg_cc0/tiny_battle_0000.png")==0u);
    assert(MaximumBytes(red,nullptr)==0u);
}
