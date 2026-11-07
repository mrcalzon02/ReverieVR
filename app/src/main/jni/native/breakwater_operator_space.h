#ifndef REVERIE_BREAKWATER_OPERATOR_SPACE_H
#define REVERIE_BREAKWATER_OPERATOR_SPACE_H
#include <cmath>
namespace reverie { namespace breakwater { namespace console {
// Wall-mounted instruments on the operator's right, facing the seated
// station. These transforms never depend on headset orientation.
constexpr float kScale=.30f,kX=1.45f,kY=-.23f,kZ=.15f,kPivotZ=-2.08f;
inline void Basis(float &nx,float &nz) {
    const float length=std::hypot(kX,kZ);nx=-kX/length;nz=-kZ/length;
}
inline void Point(float &x,float &y,float &z) {
    float nx,nz;Basis(nx,nz);
    const float u=x*kScale,v=(z-kPivotZ)*kScale;
    x=kX+nz*u+nx*v;y=kY+y*kScale;z=kZ-nx*u+nz*v;
}
inline void InverseRay(const float *origin,const float *direction,float *o,float *d) {
    float nx,nz;Basis(nx,nz);
    const float x=origin[0]-kX,z=origin[2]-kZ;
    o[0]=(nz*x-nx*z)/kScale;o[1]=(origin[1]-kY)/kScale;
    o[2]=(nx*x+nz*z)/kScale+kPivotZ;
    d[0]=(nz*direction[0]-nx*direction[2])/kScale;
    d[1]=direction[1]/kScale;d[2]=(nx*direction[0]+nz*direction[2])/kScale;
}
}}}
#endif
