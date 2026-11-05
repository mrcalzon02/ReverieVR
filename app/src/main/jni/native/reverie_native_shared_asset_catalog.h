#ifndef REVERIE_NATIVE_SHARED_ASSET_CATALOG_H
#define REVERIE_NATIVE_SHARED_ASSET_CATALOG_H
// Exact, source-reviewed shared resources. This does not allow arbitrary
// file reads or mutable remote URLs; paths are relative to the verified
// game-pack root or the APK AssetManager. No new native ABI callback.
#include <cstdint>
#include <cstring>
namespace reverie { namespace native { namespace shared_assets {
constexpr uint8_t kBreakwater=1u,kOmega=2u,kRedLedger=4u,kWorkshop=8u,kSitka=16u;
struct Entry { const char *path; uint32_t max_bytes; uint8_t consumers; };
constexpr Entry kEntries[]={
    {"native/shared/v1/materials/coastal/shingle.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/materials/coastal/quay_concrete.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop|kSitka},
    {"native/shared/v1/materials/coastal/black_basalt.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/materials/coastal/oxidized_steel.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/materials/coastal/sandbag_burlap.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/materials/coastal/olive_enamel.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/materials/coastal/dock_planks.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/materials/coastal/mossy_seawall.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/materials/coastal/corrugated_roof.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/materials/coastal/camo_netting.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/materials/coastal/diamond_plate.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/materials/coastal/trench_soil.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/materials/coastal/reinforced_concrete.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop|kSitka},
    {"native/shared/v1/materials/coastal/crate_planks.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/materials/coastal/riveted_bulkhead.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/materials/coastal/pebble_ground.png",16384u,kBreakwater|kOmega|kRedLedger|kWorkshop},
    {"native/shared/v1/models/psx/concrete_block_1.bwm",55304u,kBreakwater|kRedLedger|kWorkshop},
    {"native/shared/v1/models/psx/wooden_crate_1.bwm",55304u,kBreakwater|kRedLedger|kWorkshop},
    {"native/shared/v1/models/psx/ammo_box_mp_1.bwm",55304u,kBreakwater|kRedLedger|kWorkshop}
};
inline uint8_t ConsumerMask(const char *id) {
    if(!id)return 0u;
    return std::strcmp(id,"breakwater-battery")==0?kBreakwater:
           std::strcmp(id,"omega-front")==0?kOmega:
           std::strcmp(id,"between-deliveries-red-ledger")==0?kRedLedger:
           std::strcmp(id,"procedural-test-chamber")==0?kWorkshop:
           std::strcmp(id,"sitka")==0?kSitka:0u;
}
inline uint32_t MaximumBytes(const char *id,const char *path) {
    if(!path)return 0u;
    const uint8_t mask=ConsumerMask(id);
    if(!mask)return 0u;
    for(const Entry &e:kEntries)
        if((e.consumers & mask)!=0u &&
           std::strcmp(e.path,path)==0)return e.max_bytes;
    return 0u;
}
}}} // namespace reverie::native::shared_assets
#endif
