#ifndef REVERIE_SITKA_TERRAIN_MESH_H
#define REVERIE_SITKA_TERRAIN_MESH_H

// Game-private, CPU-only .sitkam v1 reader and GLES2 index-width adapter.
// The exporter writes little-endian float32 XYZ, RGB8 and uint32 triangles.
// GLES2 core only guarantees uint16 indices; do not require OES_element_index_uint.
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace reverie { namespace sitka {
constexpr uint32_t kMaxTerrainVertices = 512u * 512u;
constexpr uint32_t kMaxTerrainTriangles = 500000u;
constexpr uint32_t kMaxGles2BatchVertices = 65535u;
constexpr size_t kTerrainHeaderBytes = 32u;
constexpr size_t kTerrainVertexBytes = 15u;

struct TerrainVertex {
    float x, y, z;
    uint8_t r, g, b;
};
struct TerrainMesh {
    double origin_east = 0.0, origin_north = 0.0;
    std::vector<TerrainVertex> vertices;
    std::vector<uint32_t> indices;
};
struct TerrainBatch {
    // Interleaved XYZ(float32) RGB(uint8) padding: one 16-byte GPU vertex.
    struct Vertex {
        float x, y, z;
        uint8_t r, g, b, pad;
    };
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
};
static_assert(sizeof(TerrainBatch::Vertex) == 16u, "terrain GPU vertex stride");
static_assert(sizeof(float) == 4u && sizeof(double) == 8u, "IEEE754 sizes required");

inline uint32_t TerrainU32(const uint8_t *p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8u) |
           (uint32_t(p[2]) << 16u) | (uint32_t(p[3]) << 24u);
}
inline float TerrainF32(const uint8_t *p) {
    uint32_t bits = TerrainU32(p);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
inline double TerrainF64(const uint8_t *p) {
    uint64_t bits = 0;
    for (unsigned i = 0; i < 8; ++i) bits |= uint64_t(p[i]) << (8u * i);
    double value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
inline TerrainMesh ParseTerrainMesh(const uint8_t *data, size_t size) {
    static const uint8_t magic[8] = {'S','I','T','K','A','0','1',0};
    if (data == nullptr || size < kTerrainHeaderBytes ||
        std::memcmp(data, magic, sizeof(magic)) != 0)
        throw std::invalid_argument("invalid Sitka mesh magic/header");
    const uint32_t nv = TerrainU32(data + 8u);
    const uint32_t nt = TerrainU32(data + 12u);
    if (nv < 3u || nv > kMaxTerrainVertices || nt == 0u || nt > kMaxTerrainTriangles)
        throw std::invalid_argument("Sitka mesh count exceeds limits");
    const size_t expected = kTerrainHeaderBytes + size_t(nv) * kTerrainVertexBytes
                          + size_t(nt) * 3u * sizeof(uint32_t);
    if (size != expected) throw std::invalid_argument("Sitka mesh truncated/trailing bytes");
    TerrainMesh mesh;
    mesh.origin_east = TerrainF64(data + 16u);
    mesh.origin_north = TerrainF64(data + 24u);
    if (!std::isfinite(mesh.origin_east) || !std::isfinite(mesh.origin_north) ||
        std::abs(mesh.origin_east) > 1.0e8 || std::abs(mesh.origin_north) > 1.0e8)
        throw std::invalid_argument("Sitka mesh origin invalid");
    mesh.vertices.reserve(nv);
    for (uint32_t i = 0; i < nv; ++i) {
        const uint8_t *p = data + kTerrainHeaderBytes + size_t(i) * kTerrainVertexBytes;
        TerrainVertex v{TerrainF32(p), TerrainF32(p + 4u), TerrainF32(p + 8u),
                        p[12], p[13], p[14]};
        if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z) ||
            std::abs(v.x) > 100000.0f || std::abs(v.y) > 20000.0f ||
            std::abs(v.z) > 100000.0f)
            throw std::invalid_argument("Sitka mesh vertex nonfinite/out of bounds");
        mesh.vertices.push_back(v);
    }
    const uint8_t *raw = data + kTerrainHeaderBytes + size_t(nv) * kTerrainVertexBytes;
    mesh.indices.reserve(size_t(nt) * 3u);
    for (uint32_t i = 0; i < nt; ++i) {
        const uint32_t a = TerrainU32(raw + size_t(i) * 12u);
        const uint32_t b = TerrainU32(raw + size_t(i) * 12u + 4u);
        const uint32_t c = TerrainU32(raw + size_t(i) * 12u + 8u);
        if (a >= nv || b >= nv || c >= nv || a == b || a == c || b == c)
            throw std::invalid_argument("Sitka mesh triangle index invalid");
        const auto &v0 = mesh.vertices[a], &v1 = mesh.vertices[b], &v2 = mesh.vertices[c];
        const double ax = double(v1.x) - v0.x, ay = double(v1.y) - v0.y,
                     az = double(v1.z) - v0.z;
        const double bx = double(v2.x) - v0.x, by = double(v2.y) - v0.y,
                     bz = double(v2.z) - v0.z;
        const double cx = ay*bz - az*by, cy = az*bx - ax*bz, cz = ax*by - ay*bx;
        if (cx*cx + cy*cy + cz*cz <= 1.0e-16)
            throw std::invalid_argument("Sitka mesh degenerate triangle");
        mesh.indices.insert(mesh.indices.end(), {a,b,c});
    }
    return mesh;
}
inline std::vector<TerrainBatch> PackTerrainGles2(const TerrainMesh &mesh) {
    if (mesh.indices.empty() || mesh.indices.size() % 3u != 0u ||
        mesh.vertices.size() > kMaxTerrainVertices ||
        mesh.indices.size() / 3u > kMaxTerrainTriangles)
        throw std::invalid_argument("invalid terrain batch input");
    std::vector<TerrainBatch> batches;
    TerrainBatch batch;
    std::unordered_map<uint32_t,uint16_t> remap;
    for (size_t i = 0; i < mesh.indices.size(); i += 3u) {
        unsigned needed = 0;
        for (unsigned j = 0; j < 3; ++j) {
            const uint32_t index = mesh.indices[i+j];
            if (index >= mesh.vertices.size()) throw std::invalid_argument("batch index out of range");
            if (remap.count(index) == 0u) ++needed;
        }
        if (!batch.indices.empty() && batch.vertices.size() + needed > kMaxGles2BatchVertices) {
            batches.push_back(std::move(batch));
            batch = TerrainBatch{};
            remap.clear();
        }
        for (unsigned j = 0; j < 3; ++j) {
            const uint32_t index = mesh.indices[i+j];
            auto found = remap.find(index);
            if (found == remap.end()) {
                const uint16_t local = static_cast<uint16_t>(batch.vertices.size());
                const auto &v = mesh.vertices[index];
                batch.vertices.push_back({v.x,v.y,v.z,v.r,v.g,v.b,0});
                found = remap.emplace(index,local).first;
            }
            batch.indices.push_back(found->second);
        }
    }
    if (!batch.indices.empty()) batches.push_back(std::move(batch));
    return batches;
}
}} // namespace reverie::sitka
#endif
