#ifndef REVERIE_SITKA_BRIDGE_VISIBILITY_H
#define REVERIE_SITKA_BRIDGE_VISIBILITY_H

// CPU-only per-frame cull gate for one sourced landmark, never per eye.
// Bounds derive from actual authored bridge quads in terrain-local metres.
// No guessed world location, no geometry/texture rebuild on visibility flips.
#include "sitka_bridge_planes.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace reverie { namespace sitka {

class BridgeVisibility {
public:
    static constexpr double kEnterM=1250.0;
    static constexpr double kExitM=1450.0;

    explicit BridgeVisibility(const BridgePlaneMesh& mesh){
        if(mesh.planes.empty() ||
           mesh.planes.size()>2u+2u*kMaxBridgeGradeSegments)
            throw std::invalid_argument("No bounded authored Sitka bridge");
        bool first=true;
        for(const auto& plane:mesh.planes){
            for(const auto& p:plane.vertex){
                if(!std::isfinite(p.x)||!std::isfinite(p.y)||
                   !std::isfinite(p.z))
                    throw std::invalid_argument("Nonfinite bridge view bound");
                if(first){
                    low_e_=high_e_=p.x;low_n_=high_n_=p.z;first=false;
                }else{
                    low_e_=std::min(low_e_,double(p.x));
                    high_e_=std::max(high_e_,double(p.x));
                    low_n_=std::min(low_n_,double(p.z));
                    high_n_=std::max(high_n_,double(p.z));
                }
            }
        }
    }

    // The caller uses the same terrain origin as the bridge authoring file.
    // Returns true while the same existing GL resources should be drawn.
    bool Update(double east,double north,double origin_e,double origin_n){
        if(!std::isfinite(east)||!std::isfinite(north)||
           !std::isfinite(origin_e)||!std::isfinite(origin_n))return visible_;
        const double x=east-origin_e,z=north-origin_n;
        if(!std::isfinite(x)||!std::isfinite(z)||
           std::abs(x)>1.e6||std::abs(z)>1.e6)return visible_;
        const double dx=x<low_e_?low_e_-x:x>high_e_?x-high_e_:0.;
        const double dz=z<low_n_?low_n_-z:z>high_n_?z-high_n_:0.;
        const double range=visible_?kExitM:kEnterM;
        const bool desired=dx*dx+dz*dz<=range*range;
        if(desired!=visible_){
            visible_=desired;
            ++transitions_;
        }
        return visible_;
    }
    void Reset(){visible_=false;transitions_=0u;}
    bool visible()const{return visible_;}
    size_t transitions()const{return transitions_;}
private:
    double low_e_=0,high_e_=0,low_n_=0,high_n_=0;
    bool visible_=false;
    size_t transitions_=0u;
};

}} // namespace reverie::sitka
#endif
