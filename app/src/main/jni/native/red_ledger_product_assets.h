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
        "native/red-ledger/textures/101snacks_texture_0.png"},
    {4,"202draft","native/red-ledger/models/202draft.fbx",
        "native/red-ledger/textures/202draft_texture_0.png"},
    {5,"202Beer","native/red-ledger/models/202Beer.fbx",
        "native/red-ledger/textures/202Beer_texture_0.png"},
    {6,"202HARD","native/red-ledger/models/202HARD.fbx",
        "native/red-ledger/textures/202HARD_texture_0.png"},
    {trade::kDrinks+1,"202snacks","native/red-ledger/models/202snacks.fbx",
        "native/red-ledger/textures/202snacks_texture_0.png"}
};
constexpr int kCount=sizeof(kCatalog)/sizeof(kCatalog[0]);

// The native FBX decoder centers assets at this original podium anchor.
// Positions are kept in one place so render placement and tests agree.
struct Placement { float x,y,z,scale; };
constexpr float kModelAnchorX=.60f;
constexpr float kModelAnchorY=-.57f;
constexpr float kModelAnchorZ=-1.54f;
constexpr Placement StockShelf(int index) {
    // One supplier per column, four product classes per vertical rack.
    // This scales to all 6x4 original SKUs without models leaving the room.
    return {-1.55f+static_cast<float>(index/4)*.55f,
            -.52f+static_cast<float>(index%4)*.30f,-1.55f,.68f};
}
constexpr Placement StockPodium() { return {.60f,-.57f,-1.54f,1.f}; }
constexpr Placement BarCounter() { return {.65f,-.30f,-1.25f,.70f}; }
inline int UnitsOnHand(const trade::Trade &market,int index) {
    if(index<0 || index>=kCount)return 0;
    const int offer=kCatalog[index].offer;
    return offer<trade::kDrinks?
        market.stock_drink(offer):market.stock_snack(offer-trade::kDrinks);
}
static_assert(StockShelf(0).x==StockShelf(3).x &&
              StockShelf(0).y<StockShelf(3).y,
              "Keep the four product classes vertically separated");
static_assert(StockShelf(4).x>StockShelf(0).x,
              "Adjacent suppliers get their own display columns");
static_assert(StockPodium().scale==1.f,
              "Inspect the full original model on the purchase plinth");
constexpr int IndexForOffer(int offer) {
    for(int i=0;i<kCount;++i)
        if(kCatalog[i].offer==offer)return i;
    return -1;
}
static_assert(kCount==8,"Two complete original supplier product sets");
static_assert(IndexForOffer(0)==-1,"House lager is not a branded product");
static_assert(IndexForOffer(19)==3,"First snack shares the real shop SKU");
static_assert(IndexForOffer(4)==4 && IndexForOffer(5)==5 &&
              IndexForOffer(6)==6 && IndexForOffer(20)==7,
              "Map UED products to genuine catalog indices");
static_assert(IndexForOffer(7)==-1,
              "Unimported supplier offers must not display substitute art");
}}}
#endif
