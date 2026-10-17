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

// The native FBX decoder centers assets at this original podium anchor.
// Positions are kept in one place so render placement and tests agree.
struct Placement { float x,y,z,scale; };
constexpr float kModelAnchorX=.60f;
constexpr float kModelAnchorY=-.57f;
constexpr float kModelAnchorZ=-1.54f;
constexpr Placement StockShelf(int index) {
    return {-1.55f+static_cast<float>(index)*.52f,-.42f,-1.55f,.78f};
}
constexpr Placement StockPodium() { return {.60f,-.57f,-1.54f,1.f}; }
constexpr Placement BarCounter() { return {.65f,-.30f,-1.25f,.70f}; }
inline int UnitsOnHand(const trade::Trade &market,int index) {
    if(index<0 || index>=kCount)return 0;
    const int offer=kCatalog[index].offer;
    return offer<trade::kDrinks?
        market.stock_drink(offer):market.stock_snack(offer-trade::kDrinks);
}
static_assert(StockShelf(0).x<StockShelf(3).x,
              "Original products must occupy distinct shelf spaces");
static_assert(StockPodium().scale==1.f,
              "Inspect the full original model on the purchase plinth");
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
