#ifndef REVERIE_OMEGA_FRONT_CC0_PALETTE_H
#define REVERIE_OMEGA_FRONT_CC0_PALETTE_H

#include "omega_front_tactics.h"
#include <cstdint>

namespace reverie { namespace omega_art {

// 64 hand-selected CC0 16x16 source tiles, preserved byte-for-byte.
// The battle grid remains the single authority on pathfinding, cover, water,
// line-of-sight and objectives. Art never mutates that simulation.
//  0..31: Kenney Tiny Battle (grass, coast, forts and wreck silhouettes)
// 32..47: Kenney RPG Urban (park/coast and paved city surfaces)
// 48..63: Kenney Roguelike City (street and ruin surface detail)
constexpr int kCc0VisualTiles=64;
inline int GroundVariant(omega::Ground ground,int operation,int x,int y) {
    if(x<0 || y<0 || x>=omega::kWidth || y>=omega::kWidth)
        return -1;
    const unsigned seed=static_cast<unsigned>(x+omega::kWidth*y+
                            (operation<0?0:operation)*131);
    const int theater=(operation<0?0:operation)%3;
    switch(ground) {
        case omega::Ground::Plain:
            // Green countryside, orchard waterfront, ruined urban quarter.
            return theater==0?static_cast<int>(seed%3u):
                   theater==1?32+5+static_cast<int>(seed%3u):
                              48+static_cast<int>(seed%8u);
        case omega::Ground::Road:
            // Country tracks retain existing Beckenburg road; city districts
            // use real RPG paving fragments, without granting fake cover.
            return theater==0?-1:
                   theater==1?32+8+static_cast<int>(seed%8u):
                              48+8+static_cast<int>(seed%8u);
        case omega::Ground::Rubble:
            // Physical battlefield rubble is already authoritative cover.
            // Tiny Battle wrecks/fortification details are overlays, not units.
            return theater==0?23+static_cast<int>(seed%9u):
                              48+static_cast<int>(seed%16u);
        case omega::Ground::Wall:
            // Static emplacements do not gain unit health or attack actions.
            return 5+static_cast<int>(seed%13u);
        case omega::Ground::DeepWater: {
            constexpr int shore[7]={3,4,18,19,20,21,22};
            return theater==1?32+static_cast<int>(seed%5u):
                              shore[seed%7u];
        }
    }
    return -1;
}
}} // namespace reverie::omega_art
#endif
