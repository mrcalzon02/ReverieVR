#ifndef REVERIE_RED_LEDGER_STOCK_LAYOUT_H
#define REVERIE_RED_LEDGER_STOCK_LAYOUT_H

// Six supplier columns x four original product categories. Each physical
// stockroom card points at an actual purchasable Trade offer index. Keep this
// layout independent of GL and the game simulation for deterministic testing.
namespace reverie { namespace redledger { namespace stock {
constexpr int kSuppliers=6;
constexpr int kGroups=4;
constexpr int kCells=kSuppliers*kGroups;
constexpr float kLeft=-2.05f;
constexpr float kTop=0.19f;
constexpr float kStepX=0.82f;
constexpr float kStepY=0.35f;
constexpr float kDepth=-1.98f;
constexpr float kHalfWidth=0.36f;
constexpr float kHalfHeight=0.13f;
struct Cell {
    float x,y,z;
    int offer,faction,group;
};
constexpr Cell At(int slot) {
    const int faction=slot%kSuppliers;
    const int group=slot/kSuppliers;
    const int offer=group<3 ? 1+faction*3+group : 19+faction;
    return {
        kLeft+static_cast<float>(faction)*kStepX,
        kTop-static_cast<float>(group)*kStepY,
        kDepth, offer, faction, group
    };
}
constexpr bool IsValid(int slot) {return slot>=0 && slot<kCells;}
constexpr int SupplierCode(int faction) {return (faction+1)*101;}
inline const char *GroupLabel(int group) {
    static const char *const kNames[kGroups]={"DRAFT","BOTTLE","SPIRIT","SNACK"};
    return (group>=0&&group<kGroups)?kNames[group]:"INVALID";
}
static_assert(kCells==24,"Original Unreal shop catalog has 24 SKUs");
static_assert(At(0).offer==1 && At(5).offer==16,"Six draft offers");
static_assert(At(6).offer==2 && At(12).offer==3,"Bottled and spirit rows");
static_assert(At(18).offer==19 && At(23).offer==24,"Six snack offers");
}}}
#endif
