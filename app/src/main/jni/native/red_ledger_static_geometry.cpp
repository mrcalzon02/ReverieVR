#include "red_ledger_static_geometry.h"
#include "procedural_material_atlas.h"
#include <cstddef>
namespace reverie { namespace redledger { namespace geometry {
const float kUnitCubeVertices[kCubeVertexCount * 3u] = {
    -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
     0.5f,-0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
     0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f,
     0.5f,-0.5f, 0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
     0.5f,-0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,
    -0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f
};
namespace {
using reverie::procedural::Material;
struct StaticCube {
    float x,y,z,sx,sy,sz,r,g,b;
    Material material;
};
// Only immutable room pieces: dynamic hover, cups, patrons and event props
// remain in the ordinary DrawCube path.
constexpr StaticCube kRoom[] = {
    { 0.0f,-1.55f,-0.60f, 6.0f,0.10f,5.0f, 0.20f,0.20f,0.18f, Material::Stone },
    { 0.0f, 0.00f,-3.05f, 6.0f,3.10f,0.10f, 0.29f,0.28f,0.24f, Material::Stone },
    {-3.05f,0.00f,-0.60f, 0.10f,3.10f,5.0f, 0.27f,0.27f,0.24f, Material::Stone },
    { 3.05f,0.00f,-0.60f, 0.10f,3.10f,5.0f, 0.27f,0.27f,0.24f, Material::Stone },
    { 0.0f, 1.55f,-0.60f, 6.0f,0.10f,5.0f, 0.18f,0.18f,0.17f, Material::Stone },
    { 0.0f,-0.83f,-0.95f, 3.7f,0.85f,0.65f, 0.30f,0.20f,0.12f, Material::Wood },
    { 0.0f,-0.36f,-0.95f, 3.9f,0.12f,0.75f, 0.39f,0.27f,0.15f, Material::Wood },
    { 1.35f,-0.95f,-1.80f, 0.72f,0.16f,0.72f, 0.22f,0.16f,0.12f, Material::Wood },
    { 1.35f,-1.30f,-1.80f, 0.12f,0.70f,0.12f, 0.17f,0.13f,0.10f, Material::Wood },
    { 1.90f,-1.38f,-2.38f, 1.35f,0.18f,0.70f, 0.31f,0.28f,0.23f, Material::Wood },
    { 0.0f, 1.28f,-0.70f, 0.55f,0.10f,0.34f, 0.54f,0.45f,0.28f, Material::Metal }
};
static_assert(sizeof(kRoom)/sizeof(kRoom[0])==kStaticCubeCount,"cube count");
}
bool BuildStaticRoomVertices(float *output,size_t capacity_floats) {
    if (output==nullptr || capacity_floats<kStaticVertexFloats) return false;
    for (size_t cube=0;cube<kStaticCubeCount;++cube) {
        const StaticCube &item=kRoom[cube];
        const int material=static_cast<int>(item.material);
        const float tile_u=0.00390625f+(material&1)*0.5f;
        const float tile_v=0.00390625f+(material>>1)*0.5f;
        for (size_t vertex=0;vertex<kCubeVertexCount;++vertex) {
            const size_t face=vertex/6u;
            const float *unit=&kUnitCubeVertices[vertex*3u];
            const float u=(face<2u||face>=4u)?unit[0]+0.5f:unit[2]+0.5f;
            const float v=face>=4u?unit[2]+0.5f:unit[1]+0.5f;
            float *out=&output[(cube*kCubeVertexCount+vertex)*kStaticVertexStride];
            out[0]=item.x+unit[0]*item.sx;
            out[1]=item.y+unit[1]*item.sy;
            out[2]=item.z+unit[2]*item.sz;
            out[3]=u;out[4]=v;
            out[5]=item.r;out[6]=item.g;out[7]=item.b;
            out[8]=tile_u;out[9]=tile_v;
        }
    }
    return true;
}
}}}
