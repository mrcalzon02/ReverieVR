#ifndef REVERIE_NATIVE_FLAT_MESH_H
#define REVERIE_NATIVE_FLAT_MESH_H
// CPU-only colored geometry recipes. Each vertex is XYZ RGB (six floats).
// Caller controls regeneration, GPU upload, context loss and stereo rendering.
#include <cmath>
#include <vector>
namespace reverie { namespace flatmesh {
inline void Vertex(std::vector<float>& v,float x,float y,float z,float r,float g,float b){
    v.insert(v.end(),{x,y,z,r,g,b});
}
inline void Triangle(std::vector<float>& v,float ax,float ay,float bx,float by,
                     float cx,float cy,float z,float r,float g,float b){
    Vertex(v,ax,ay,z,r,g,b);Vertex(v,bx,by,z,r,g,b);Vertex(v,cx,cy,z,r,g,b);
}
inline void Rect(std::vector<float>& v,float x,float y,float w,float h,
                 float z,float r,float g,float b){
    const float l=x-w*.5f,q=x+w*.5f,d=y-h*.5f,t=y+h*.5f;
    Triangle(v,l,d,q,d,q,t,z,r,g,b);Triangle(v,l,d,q,t,l,t,z,r,g,b);
}
inline void Segment(std::vector<float>& v,float x,float y,float xx,float yy,
                    float width,float z,float r,float g,float b){
    const float dx=xx-x,dy=yy-y,length=std::sqrt(dx*dx+dy*dy);
    if(length<.00001f)return;
    const float px=-dy/length*width*.5f,py=dx/length*width*.5f;
    Triangle(v,x+px,y+py,x-px,y-py,xx+px,yy+py,z,r,g,b);
    Triangle(v,xx+px,yy+py,x-px,y-py,xx-px,yy-py,z,r,g,b);
}
inline void Disc(std::vector<float>& v,float x,float y,float rad,
                 float z,float r,float g,float b){
    constexpr float pi=3.14159265358979323846f;
    for(int i=0;i<12;i++){
        const float a=2.f*pi*i/12.f,aa=2.f*pi*(i+1)/12.f;
        Triangle(v,x,y,x+rad*std::cos(a),y+rad*std::sin(a),
                 x+rad*std::cos(aa),y+rad*std::sin(aa),z,r,g,b);
    }
}
}} // namespace reverie::flatmesh
#endif
