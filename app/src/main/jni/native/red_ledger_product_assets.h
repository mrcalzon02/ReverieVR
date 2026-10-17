#ifndef REVERIE_RED_LEDGER_PRODUCT_ASSETS_H
#define REVERIE_RED_LEDGER_PRODUCT_ASSETS_H

// Original UE 5.7 FBX/PNG exports. Every path must resolve to an actual APK
// asset; original catalog entries remain separate from procedural furniture.
// Offer IDs match the saved Trade catalog; no new save format is required.
#include "red_ledger_trade.h"
namespace reverie { namespace redledger { namespace products {
struct Entry {
    int offer;
    const char *model_name;
    const char *mesh_path;
    const char *texture_path;
};
constexpr Entry kCatalog[]={
    {1,"101draft","native/red-ledger/models/101draft.fbx",
        "native/red-ledger/textures/101draft_texture_0.png"},
    {2,"101beer","native/red-ledger/models/101beer.fbx",
        "native/red-ledger/textures/101beer_texture_0.png"},
    {3,"101hard","native/red-ledger/models/101hard.fbx",
        "native/red-ledger/textures/101hard_texture_0.png"},
    {trade::kDrinks,"101snacks","native/red-ledger/models/101snacks.fbx",
        "native/red-ledger/textures/101snacks_texture_0.png"}
};
constexpr int kCount=sizeof(kCatalog)/sizeof(kCatalog[0]);
constexpr int IndexForOffer(int offer) {
    for(int i=0;i<kCount;++i)
        if(kCatalog[i].offer==offer)return i;
    return -1;
}
static_assert(kCount==4,"Four genuine stock examples");
static_assert(IndexForOffer(0)==-1,"House lager is not a branded product");
static_assert(IndexForOffer(19)==3,"First snack shares the real shop SKU");
static_assert(IndexForOffer(4)==-1,"Do not show another supplier's goods");
}}}
#endif
