#ifndef REVERIE_NATIVE_HORIZON_GEOMETRY_H
#define REVERIE_NATIVE_HORIZON_GEOMETRY_H
#include <cmath>
#include <vector>
namespace reverie { namespace native { namespace horizon {
struct Vertex { float x,y,z,u,v; };
constexpr int kSegments=32;
constexpr int kBands=12;
constexpr float kPi=3.14159265358979323846f;
constexpr int kUpperVertices=kSegments*(kBands/2)*6;
constexpr int kFullVertices=kSegments*kBands*6;
inline void Sphere(std::vector<Vertex>& out) {
    out.clear();out.reserve(kFullVertices);
    // Bands first: the upper 1,152 vertices form a sky-only drawable
    // hemisphere. This lets survey-backed projects use clouds and daylight
    // without presenting AI-generated mountain silhouettes as real terrain.
    for(int row=0;row<kBands;++row){
        const float top=kPi*(.5f-float(row)/kBands);
        const float bottom=kPi*(.5f-float(row+1)/kBands);
        const float v0=float(row)/kBands,v1=float(row+1)/kBands;
        for(int col=0;col<kSegments;++col){
            const float a=2*kPi*float(col)/kSegments;
            const float b=2*kPi*float(col+1)/kSegments;
            const float u0=float(col)/kSegments,u1=float(col+1)/kSegments;
            const Vertex p0={std::sin(a)*std::cos(top),std::sin(top),-std::cos(a)*std::cos(top),u0,v0};
            const Vertex p1={std::sin(b)*std::cos(top),std::sin(top),-std::cos(b)*std::cos(top),u1,v0};
            const Vertex p2={std::sin(b)*std::cos(bottom),std::sin(bottom),-std::cos(b)*std::cos(bottom),u1,v1};
            const Vertex p3={std::sin(a)*std::cos(bottom),std::sin(bottom),-std::cos(a)*std::cos(bottom),u0,v1};
            out.insert(out.end(),{p0,p1,p2,p0,p2,p3});
        }
    }
}
}}}
#endif
