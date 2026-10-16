#ifndef REVERIE_RED_LEDGER_MUG_MESH_H
#define REVERIE_RED_LEDGER_MUG_MESH_H
#include <cstddef>
// A shared 12-sided hollow vessel with a narrow rolled lip and closed base.
// Deterministic, context-time mesh generation; no GLES or runtime trigonometry.
namespace reverie { namespace redledger { namespace mug {
constexpr size_t kSides=12u;
constexpr size_t kStride=5u; // xyz, u, v
constexpr size_t kVerticesPerSide=15u; // six wall, six rim, three base
constexpr size_t kVertexCount=kSides*kVerticesPerSide;
constexpr size_t kFloatCount=kVertexCount*kStride;
struct RingPoint { float x,z; };
static const RingPoint kRing[kSides] = {
 {0.5f,0.0f},{0.433013f,0.25f},{0.25f,0.433013f},{0.0f,0.5f},
 {-0.25f,0.433013f},{-0.433013f,0.25f},{-0.5f,0.0f},
 {-0.433013f,-0.25f},{-0.25f,-0.433013f},{0.0f,-0.5f},
 {0.25f,-0.433013f},{0.433013f,-0.25f}
};
inline void Put(float *out,size_t &at,float x,float y,float z,float u,float v) {
    float *p=&out[at*kStride];p[0]=x;p[1]=y;p[2]=z;p[3]=u;p[4]=v;
    ++at;
}
inline bool Build(float *out,size_t floats) {
    if(out==nullptr || floats<kFloatCount) return false;
    size_t at=0;
    for(size_t side=0;side<kSides;++side) {
        const RingPoint &a=kRing[side],&b=kRing[(side+1u)%kSides];
        const float u0=static_cast<float>(side)/static_cast<float>(kSides);
        const float u1=static_cast<float>(side+1u)/static_cast<float>(kSides);
        // Side wall, double triangle winding is immaterial (culling off).
        Put(out,at,a.x,-0.5f,a.z,u0,0.0f);
        Put(out,at,b.x,-0.5f,b.z,u1,0.0f);
        Put(out,at,b.x,0.5f,b.z,u1,1.0f);
        Put(out,at,a.x,-0.5f,a.z,u0,0.0f);
        Put(out,at,b.x,0.5f,b.z,u1,1.0f);
        Put(out,at,a.x,0.5f,a.z,u0,1.0f);
        // Top rim is annular; leave the opening genuinely hollow.
        Put(out,at,a.x,0.5f,a.z,0.5f+a.x,0.5f+a.z);
        Put(out,at,b.x,0.5f,b.z,0.5f+b.x,0.5f+b.z);
        Put(out,at,b.x*0.82f,0.5f,b.z*0.82f,
            0.5f+b.x*0.82f,0.5f+b.z*0.82f);
        Put(out,at,a.x,0.5f,a.z,0.5f+a.x,0.5f+a.z);
        Put(out,at,b.x*0.82f,0.5f,b.z*0.82f,
            0.5f+b.x*0.82f,0.5f+b.z*0.82f);
        Put(out,at,a.x*0.82f,0.5f,a.z*0.82f,
            0.5f+a.x*0.82f,0.5f+a.z*0.82f);
        // Solid base at bottom. Inward-facing cavity is kept simple.
        Put(out,at,0.0f,-0.5f,0.0f,0.5f,0.5f);
        Put(out,at,b.x,-0.5f,b.z,0.5f+b.x,0.5f+b.z);
        Put(out,at,a.x,-0.5f,a.z,0.5f+a.x,0.5f+a.z);
    }
    return at==kVertexCount;
}
}}}
#endif
