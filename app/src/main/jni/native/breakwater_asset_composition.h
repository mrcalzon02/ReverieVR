#ifndef REVERIE_BREAKWATER_ASSET_COMPOSITION_H
#define REVERIE_BREAKWATER_ASSET_COMPOSITION_H

#include "breakwater_beckenburg_mesh.h"

#include <array>
#include <cstddef>
#include <cmath>

namespace reverie { namespace breakwater { namespace dressing {

// Composite real converted PSX models instead of drawing placeholder blocks.
// All placements deliberately stay outside the central seated gun sightline.
// Static dressing has no combat target ID or collider and cannot take damage;
// authored terrain collision remains authoritative.
struct Placement {
    imported::ModelKind mesh;
    float x,y,z,scale,yaw;
};
constexpr std::array<Placement,11> kEmplacements={{
    {imported::Concrete,   -3.21f,-1.12f,-1.75f,2.15f,.12f},
    {imported::Concrete,    3.29f,-1.12f,-1.77f,2.15f,-.10f},
    {imported::WoodenCrate,-2.88f,-1.04f,-.66f,1.35f,.24f},
    {imported::WoodenCrate, 3.08f,-1.04f,-.60f,1.22f,-.27f},
    {imported::AmmoBox,    -2.93f,-.52f,-.61f,2.45f,.21f},
    // New compound revetments: a staggered rear row and upper course.
    // One block still contains all its exact original sampled-color faces.
    {imported::Concrete,   -3.31f,-1.12f,-2.18f,2.15f,-.09f},
    {imported::Concrete,    3.31f,-1.12f,-2.18f,2.15f,.09f},
    {imported::Concrete,   -3.24f,-.65f,-1.96f,1.85f,.04f},
    {imported::Concrete,    3.24f,-.65f,-1.96f,1.85f,-.04f},
    // A rear crate and second ammo tin make both flanks a real supply area.
    {imported::WoodenCrate,-3.05f,-1.04f,.13f,1.12f,-.14f},
    {imported::AmmoBox,     3.03f,-.52f,-.58f,2.30f,-.18f}
}};
static_assert(kEmplacements.size()==11u,"Missing composed beach dressing");

template<class EmitTriangle>
size_t Emit(const std::array<imported::Model,imported::kAssetPaths.size()> &models,
            bool purchasedTurret,float turretX,float turretZ,EmitTriangle emit) {
    size_t triangles=0u;
    for(const Placement &part:kEmplacements)
        triangles+=models[part.mesh].Emit(
            part.x,part.y,part.z,part.scale,part.yaw,emit);
    if(purchasedTurret)
        triangles+=models[imported::AmmoBox].Emit(
            turretX+.52f,-.86f,turretZ+.38f,2.05f,.55f,emit);
    return triangles;
}

}}} // namespace reverie::breakwater::dressing
#endif
