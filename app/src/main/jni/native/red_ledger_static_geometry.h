#ifndef REVERIE_RED_LEDGER_STATIC_GEOMETRY_H
#define REVERIE_RED_LEDGER_STATIC_GEOMETRY_H
#include <cstddef>
namespace reverie { namespace redledger { namespace geometry {
constexpr size_t kCubeVertexCount = 36u;
constexpr size_t kStaticCubeCount = 11u;
constexpr size_t kStaticVertexStride = 10u; // xyz, uv, rgb, tile origin
constexpr size_t kStaticVertexCount = kStaticCubeCount * kCubeVertexCount;
constexpr size_t kStaticVertexFloats = kStaticVertexCount * kStaticVertexStride;
constexpr size_t kStaticVertexBytes = kStaticVertexFloats * sizeof(float);
extern const float kUnitCubeVertices[kCubeVertexCount * 3u];
// Immutable world-space geometry, generated once per GL context.
// Rejects invalid/undersized output before any write.
bool BuildStaticRoomVertices(float *output, size_t capacity_floats);
}}}
#endif
