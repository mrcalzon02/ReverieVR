#ifndef REVERIE_BREAKWATER_SILHOUETTES_H
#define REVERIE_BREAKWATER_SILHOUETTES_H
#include "breakwater_infantry_tactics.h"
#include "breakwater_invasion_units.h"
#include "breakwater_unit_geometry.h"
#include <algorithm>
#include <cmath>

namespace reverie { namespace breakwater {
// Game-private vertex-color recipes. Emitters take the existing Box and
// Triangle argument lists; no GL state, textures, RNG or heap allocation.
// Hulls and passengers are visual only: Campaign owns every combat hitbox.
// Pose is a pure function of the authoritative unit's travelled position.
// Rendering either stereo eye cannot advance or desynchronize animation.
inline float LandingRampProgress(float z) {
    if (!std::isfinite(z)) return 0.0f;
    return std::max(0.0f, std::min(1.0f, (z + 3.7f) / 1.5f));
}
inline float VesselWakePhase(float z) {
    if (!std::isfinite(z)) return 0.0f;
    float phase = std::fmod((z + 11.0f) * 2.0f, 1.0f);
    return phase < 0.0f ? phase + 1.0f : phase;
}
inline float InfantryMarchStride(const Infantry &soldier) {
    // Infantry freeze their stride upon reaching the real firing line.
    if (!soldier.active || !std::isfinite(soldier.x) ||
        !std::isfinite(soldier.z) || InfantryReady(soldier)) return 0.0f;
    float phase = std::fmod((soldier.z + 2.4f) * 4.0f +
                            (soldier.x + 4.0f) * .37f, 1.0f);
    if (phase < 0.0f) phase += 1.0f;
    // Cheap triangle wave: no trigonometry, clock, or persistent per-NPC state.
    return .075f * (1.0f - 4.0f * std::abs(phase - .5f));
}
template <typename BoxFn, typename TriFn>
void EmitVesselSilhouette(const Enemy &e, BoxFn box, TriFn triangle) {
    if (!e.active || !std::isfinite(e.x) || !std::isfinite(e.z) ||
        (e.type != UnitType::Rowboat &&
         e.type != UnitType::LandingCraft &&
         e.type != UnitType::Gunboat)) return;
    const float x = e.x, z = e.z;
    if(e.type==UnitType::Rowboat) {
        unit_geometry::ContouredHull(x,z,unit_geometry::HullKind::Rowboat,triangle);
        unit_geometry::RowboatFurniture(x,z,e.passengers,box,triangle);
    } else if(e.type==UnitType::LandingCraft) {
        unit_geometry::ContouredHull(x,z,unit_geometry::HullKind::LandingCraft,triangle);
        unit_geometry::LandingDetails(x,z,LandingRampProgress(z),
                                     e.passengers,e.carries_armor,box,triangle);
    } else {
        unit_geometry::ContouredHull(x,z,unit_geometry::HullKind::Gunboat,triangle);
        // Deck gun and elevated bridge retain the existing real geometry.
        box(x,-.70f,z-.08f,.90f,.07f,1.04f,.21f,.27f,.31f);
        box(x,-.49f,z-.25f,.54f,.37f,.41f,.45f,.49f,.48f);
        box(x,-.295f,z-.25f,.60f,.06f,.47f,.20f,.24f,.26f);
        box(x,-.48f,z+.28f,.35f,.17f,.34f,.32f,.36f,.33f);
        box(x,-.42f,z+.56f,.095f,.09f,.43f,.22f,.25f,.24f);
        // Observation windows, radome, exhaust and braced bow turret.
        box(x,-.47f,z-.044f,.35f,.12f,.018f,.12f,.18f,.19f);
        box(x+.21f,-.28f,z-.49f,.06f,.25f,.08f,.24f,.26f,.27f);
        for(int side:{-1,1}){
            const float sideX=static_cast<float>(side);
            triangle(x+sideX*.56f,-.75f,z+.43f,x,-.79f,z+.65f,
                     x+sideX*.43f,-.64f,z+.41f,.42f,.47f,.48f);
        }
    }
    // Boat-relative two-wedge foam pattern shifts with actual hull progress.
    // No independent clock, extra triangles or effect-particle allocations.
    const float wake = VesselWakePhase(z);
    for(int side=-1;side<=1;side+=2) {
        const float sx=static_cast<float>(side);
        triangle(x+sx*.24f,-1.095f,z-.42f,
                 x+sx*(.50f+.07f*wake),-1.095f,z-1.00f-.19f*wake,
                 x+sx*.40f,-1.095f,z-.92f-.10f*wake,.61f,.76f,.77f);
    }
}

// The genuine later-campaign heavy transport. This is a wide, armored
// assault ship with a reinforced ramp, raised gun turret and two covered
// vehicle cradles; simulation owns its larger hull hit volume and landing.
template <typename BoxFn, typename TriFn>
void EmitHeavyTransportSilhouette(const Enemy &ship,BoxFn box,TriFn tri) {
    if(!ship.active || ship.type!=UnitType::ArmoredTransport ||
       !std::isfinite(ship.x) || !std::isfinite(ship.z))return;
    const float x=ship.x,z=ship.z;
    // Lower sea hull, armored beam and reinforced belt.
    unit_geometry::ContouredHull(x,z,unit_geometry::HullKind::HeavyTransport,tri);
    box(x,-.63f,z-.04f,1.64f,.12f,1.94f,.34f,.40f,.34f);
    for(int side=-1;side<=1;side+=2) {
        const float sx=static_cast<float>(side);
        box(x+sx*.87f,-.49f,z-.11f,.16f,.40f,1.92f,
            .35f,.40f,.33f);
        box(x+sx*.95f,-.47f,z+.61f,.075f,.22f,.52f,
            .22f,.28f,.24f);
        // Faceted forward armor cheeks.
        tri(x+sx*.91f,-.60f,z+.79f,x+sx*.58f,-.44f,z+1.10f,
            x+sx*.94f,-.78f,z+1.10f,.45f,.48f,.38f);
    }
    // Elevated command shield, unmistakable relative to troop landing
    // craft. Turret barrel faces the player's battery along positive z.
    box(x,-.32f,z-.57f,.74f,.35f,.54f,.29f,.37f,.34f);
    box(x,-.14f,z-.59f,.60f,.07f,.42f,.18f,.22f,.21f);
    box(x,-.30f,z-.08f,.47f,.19f,.43f,.39f,.42f,.36f);
    box(x,-.28f,z+.32f,.13f,.11f,.73f,.22f,.26f,.22f);
    // Two independently simulated light cars remain cargo until landing.
    for(int side=-1;side<=1;side+=2) {
        const float sx=static_cast<float>(side);
        box(x+sx*.46f,-.37f,z+.34f,.60f,.20f,.58f,
            .28f,.38f,.27f);
        box(x+sx*.46f,-.22f,z+.32f,.25f,.12f,.31f,
            .20f,.28f,.22f);
        box(x+sx*.46f,-.22f,z+.56f,.07f,.065f,.31f,
            .19f,.22f,.18f);
    }
    // Only the final physical approach lowers the armored loading ramp.
    // Long hull stops with its center offshore; the bow meets the same
    // cargo-exit line as the real beach armor/infantry spawn coordinates.
    const float ramp=LandingRampProgress(invasion::HeavyRampReferenceZ(z));
    const float tipY=-.51f-.53f*ramp;
    const float tipZ=z+1.28f+.18f*ramp;
    tri(x-.73f,-.53f,z+.96f,x+.73f,-.53f,z+.96f,
        x+.70f,tipY,tipZ,.55f,.56f,.46f);
    tri(x-.73f,-.53f,z+.96f,x+.70f,tipY,tipZ,
        x-.70f,tipY,tipZ,.55f,.56f,.46f);
}
// A light armored shore car, not a sprite representing an invulnerable
// decoration. All geometry follows Campaign::armored_vehicles() positions;
// its 0.43-unit centered hit sphere encompasses hull/turret.
template <typename BoxFn, typename TriFn>
void EmitArmoredVehicleSilhouette(const ArmoredVehicle &vehicle,
                                 BoxFn box,TriFn triangle) {
    if(!vehicle.active || !std::isfinite(vehicle.x) ||
       !std::isfinite(vehicle.z))return;
    unit_geometry::ShoreArmor(vehicle.x,vehicle.z,
        vehicle.role==ArmorRole::Tank,box,triangle);
}
template <typename BoxFn, typename TriFn>
void EmitInfantrySilhouette(const Infantry &soldier, BoxFn box, TriFn triangle) {
    if (!soldier.active || !std::isfinite(soldier.x) ||
        !std::isfinite(soldier.z)) return;
    const float x=soldier.x,z=soldier.z;
    // Body/head preserve the campaign's 0.30-world-unit spherical aim volume.
    box(x,-.76f,z,.19f,.35f,.15f,.37f,.41f,.30f);
    box(x,-.47f,z,.14f,.15f,.14f,.60f,.52f,.36f);
    // Helmets remain in the aiming sphere; arms counter-swing with legs.
    triangle(x-.115f,-.41f,z+.075f,x+.115f,-.41f,z+.075f,
             x,-.370f,z+.075f,.27f,.33f,.27f);
    const float stride = InfantryMarchStride(soldier);
    for(int side=-1;side<=1;side+=2) {
        const float sx=static_cast<float>(side);
        triangle(x+sx*.085f,-.63f,z+.025f,
                 x+sx*.145f,-.80f,z+.045f-sx*stride*.35f,
                 x+sx*.09f,-.78f,z-.055f-sx*stride*.35f,
                 .35f,.39f,.29f);
        triangle(x+sx*.05f,-.87f,z,
                 x+sx*.095f,-.915f,z+sx*stride,
                 x+sx*.015f,-.915f,z-sx*stride*.45f,
                 .27f,.30f,.24f);
    }
    unit_geometry::SoldierEquipment(x,z,
        soldier.tactic==InfantryTactic::Support,box,triangle);
}
}} // namespace reverie::breakwater
#endif
