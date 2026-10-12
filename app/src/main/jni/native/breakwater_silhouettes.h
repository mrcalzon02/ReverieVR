#ifndef REVERIE_BREAKWATER_SILHOUETTES_H
#define REVERIE_BREAKWATER_SILHOUETTES_H
#include "breakwater_campaign.h"
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
        !std::isfinite(soldier.z) || soldier.z >= -1.34f) return 0.0f;
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
    if (e.type == UnitType::Rowboat) {
        // Slender pointed clinker-style hull, open dark well and two thwarts.
        box(x,-.83f,z,.68f,.20f,1.02f,.46f,.34f,.21f);
        box(x,-.715f,z,.49f,.045f,.76f,.16f,.18f,.15f);
        for(int side=-1;side<=1;side+=2) {
            const float sx=static_cast<float>(side);
            triangle(x+sx*.34f,-.78f,z+.38f,
                     x,-.85f,z+.68f,
                     x+sx*.20f,-.68f,z+.37f,.58f,.44f,.28f);
        }
        box(x,-.68f,z-.19f,.50f,.055f,.10f,.59f,.46f,.29f);
        // The one embarked passenger is not an extra target or infantry slot.
        if(e.passengers>0) {
            box(x,-.57f,z+.06f,.17f,.24f,.15f,.41f,.43f,.32f);
            box(x,-.38f,z+.06f,.14f,.14f,.14f,.66f,.54f,.37f);
        }
    } else if (e.type == UnitType::LandingCraft) {
        // Broad troop well; bow ramp hinges down only during final approach.
        box(x,-.86f,z,1.10f,.20f,1.10f,.39f,.44f,.40f);
        box(x-.50f,-.68f,z-.04f,.10f,.24f,.99f,.49f,.52f,.46f);
        box(x+.50f,-.68f,z-.04f,.10f,.24f,.99f,.49f,.52f,.46f);
        box(x,-.755f,z-.44f,.91f,.17f,.13f,.31f,.33f,.31f);
        const float ramp = LandingRampProgress(z);
        const float rampTipY = -.46f - .57f * ramp;
        const float rampTipZ = z + .56f + .16f * ramp;
        triangle(x-.48f,-.76f,z+.48f,x+.48f,-.76f,z+.48f,
                 x+.40f,rampTipY,rampTipZ,.60f,.62f,.53f);
        triangle(x-.48f,-.76f,z+.48f,x+.40f,rampTipY,rampTipZ,
                 x-.40f,rampTipY,rampTipZ,.60f,.62f,.53f);
        const int passengers=std::min(6,static_cast<int>(e.passengers));
        for(int n=0;n<passengers;n++) {
            const float px=x+(static_cast<float>(n%3)-1.0f)*.27f;
            const float pz=z+(n<3?-.17f:.19f);
            box(px,-.64f,pz,.14f,.19f,.13f,.29f,.36f,.28f);
            box(px,-.47f,pz,.12f,.12f,.12f,.57f,.53f,.38f);
        }
    } else if (e.type == UnitType::Gunboat) {
        // Long angular patrol hull, raised wheelhouse and a real forward gun.
        box(x,-.85f,z,1.18f,.24f,1.18f,.30f,.36f,.41f);
        box(x,-.71f,z-.08f,.96f,.07f,1.06f,.21f,.27f,.31f);
        box(x,-.49f,z-.25f,.54f,.37f,.41f,.45f,.49f,.48f);
        box(x,-.295f,z-.25f,.60f,.06f,.47f,.20f,.24f,.26f);
        box(x,-.48f,z+.28f,.35f,.17f,.34f,.32f,.36f,.33f);
        box(x,-.42f,z+.56f,.095f,.09f,.43f,.22f,.25f,.24f);
        // Sloped bow and wheelhouse windows survive low-res silhouettes.
        triangle(x-.58f,-.81f,z+.45f,x,-.82f,z+.72f,
                 x-.46f,-.70f,z+.43f,.43f,.48f,.49f);
        triangle(x+.58f,-.81f,z+.45f,x+.46f,-.70f,z+.43f,
                 x,-.82f,z+.72f,.43f,.48f,.49f);
        box(x,-.47f,z-.044f,.35f,.12f,.018f,.12f,.18f,.19f);
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
}
}} // namespace reverie::breakwater
#endif
