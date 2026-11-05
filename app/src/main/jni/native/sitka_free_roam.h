#ifndef REVERIE_SITKA_FREE_ROAM_H
#define REVERIE_SITKA_FREE_ROAM_H

// Sitka-only grounded open-world travel over actual authored terrain triangles.
// Paths are navigational references, NOT movement permissions. Terrain and
// coastline are source geometry; no synthetic streets or invisible corridors.
// This class is CPU-only and advances exactly once in simulation update.
#include "sitka_terrain_mesh.h"
#include "sitka_environment.h"
#include "sitka_shore_geometry.h"
#include "reverie_native_sdk.h"
#include "reverie_native_math.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace reverie { namespace sitka {

struct RoamPose {float x=0.f,ground_y=0.f,z=0.f;};
struct GroundHit {bool valid=false;float y=0.f;};

class RoamTerrainIndex {
public:
    static constexpr double kCellM=32.;
    static constexpr size_t kMaxCellRefs=2000000u;
    explicit RoamTerrainIndex(const TerrainMesh& source):mesh_(&source){
        if(source.indices.empty()||source.indices.size()%3u!=0u)
            throw std::invalid_argument("Sitka roam needs authored terrain");
        for(size_t i=0;i<source.indices.size();i+=3u){
            const auto& a=source.vertices.at(source.indices[i]);
            const auto& b=source.vertices.at(source.indices[i+1u]);
            const auto& c=source.vertices.at(source.indices[i+2u]);
            const double minx=std::min({a.x,b.x,c.x});
            const double maxx=std::max({a.x,b.x,c.x});
            const double minz=std::min({a.z,b.z,c.z});
            const double maxz=std::max({a.z,b.z,c.z});
            const int lo_x=Cell(minx),hi_x=Cell(maxx);
            const int lo_z=Cell(minz),hi_z=Cell(maxz);
            const uint64_t cols=uint64_t(hi_x-lo_x)+1u;
            const uint64_t rows=uint64_t(hi_z-lo_z)+1u;
            const uint32_t tri=static_cast<uint32_t>(i/3u);
            if(cols*rows>64u){
                // Very long authentic triangles are checked separately;
                // don't explode a single polygon into thousands of bins.
                if(wide_.size()>=1024u)
                    throw std::invalid_argument("Sitka roam oversized terrain spans");
                wide_.push_back(tri);
                continue;
            }
            if(refs_.size()+size_t(cols*rows)>kMaxCellRefs)
                throw std::invalid_argument("Sitka roam terrain index budget");
            for(int x=lo_x;x<=hi_x;++x)
                for(int z=lo_z;z<=hi_z;++z)
                    refs_.push_back({x,z,tri});
        }
        std::sort(refs_.begin(),refs_.end());
    }
    GroundHit Sample(double x,double z,double previous_y=0.)const{
        if(!std::isfinite(x)||!std::isfinite(z)||
           std::abs(x)>100000.||std::abs(z)>100000.)return {};
        const Ref key{Cell(x),Cell(z),0u};
        const auto lo=std::lower_bound(refs_.begin(),refs_.end(),key,
            [](const Ref& a,const Ref& b){return a.x<b.x||(a.x==b.x&&a.z<b.z);});
        GroundHit best;
        double best_distance=std::numeric_limits<double>::infinity();
        const auto consider=[&](uint32_t tri){
            const size_t i=size_t(tri)*3u;
            const auto& a=mesh_->vertices[mesh_->indices[i]];
            const auto& b=mesh_->vertices[mesh_->indices[i+1u]];
            const auto& c=mesh_->vertices[mesh_->indices[i+2u]];
            const double abx=b.x-a.x,abz=b.z-a.z;
            const double acx=c.x-a.x,acz=c.z-a.z;
            const double det=abx*acz-acx*abz;
            if(std::abs(det)<1.e-8)return;
            const double dx=x-a.x,dz=z-a.z;
            const double u=(dx*acz-acx*dz)/det;
            const double v=(abx*dz-dx*abz)/det;
            if(u< -1.e-5||v< -1.e-5||u+v>1.+1.e-5)return;
            const double dyb=b.y-a.y,dyc=c.y-a.y;
            const double slope=std::hypot(
                (dyb*acz-dyc*abz)/det,
                (abx*dyc-acx*dyb)/det);
            if(!std::isfinite(slope)||slope>1.0)return;
            const double y=a.y+u*dyb+v*dyc;
            const double difference=std::abs(y-previous_y);
            if(!best.valid||difference<best_distance){
                best={true,static_cast<float>(y)};
                best_distance=difference;
            }
        };
        for(auto p=lo;p!=refs_.end()&&p->x==key.x&&p->z==key.z;++p)
            consider(p->tri);
        for(uint32_t tri:wide_)consider(tri);
        return best;
    }
    // Pick real dry surface near local origin if no optional sourced pedestrian
    // starting point is installed. This never generates a ground triangle.
    template<class Valid>
    GroundHit ChooseStart(Valid&& valid,double& x,double& z)const{
        GroundHit chosen;
        double best=std::numeric_limits<double>::infinity();
        for(size_t i=0;i<mesh_->indices.size();i+=3u){
            const auto& a=mesh_->vertices[mesh_->indices[i]];
            const auto& b=mesh_->vertices[mesh_->indices[i+1u]];
            const auto& c=mesh_->vertices[mesh_->indices[i+2u]];
            const double cx=(double(a.x)+b.x+c.x)/3.;
            const double cz=(double(a.z)+b.z+c.z)/3.;
            const double y=(double(a.y)+b.y+c.y)/3.;
            const double distance=cx*cx+cz*cz;
            if(distance>=best||!valid(cx,cz,y))continue;
            const auto hit=Sample(cx,cz,y);
            if(!hit.valid||std::abs(hit.y-y)>.05)continue;
            best=distance;chosen=hit;x=cx;z=cz;
        }
        return chosen;
    }
private:
    struct Ref{
        int x,z;
        uint32_t tri;
        bool operator<(const Ref& other)const{
            return x<other.x||(x==other.x&&
                 (z<other.z||(z==other.z&&tri<other.tri)));
        }
    };
    static int Cell(double x){return static_cast<int>(std::floor(x/kCellM));}
    const TerrainMesh* mesh_;
    std::vector<Ref> refs_;
    std::vector<uint32_t> wide_;
};

class FreeRoamSession {
public:
    // Home's exact analog response: 28% dead zone, normalized direction,
    // linear post-deadzone strength, 0.05-second frame cap, 2.8 m/s maximum.
    static constexpr float kDeadzone=.28f;
    static constexpr float kSpeedMps=2.8f;
    static constexpr float kMaxFrameSeconds=.05f;
    static constexpr float kMaxStepHeight=.30f;
    static constexpr double kFootRadius=.24;
    FreeRoamSession(const TerrainMesh& terrain,
                    const std::vector<Water>& waters,
                    const std::vector<ShoreFeature>& structures,
                    bool sourced_start=false,double start_x=0.,double start_z=0.)
        :ground_(terrain),waters_(&waters),structures_(&structures){
        GroundHit start;
        if(sourced_start){
            start=ground_.Sample(start_x,start_z);
            if(!start.valid||!CanStand(start_x,start_z,start.y))
                throw std::invalid_argument("Sitka authored spawn not on dry walkable terrain");
        }else{
            start=ground_.ChooseStart([&](double x,double z,double y){
                return CanStand(x,z,y);
            },start_x,start_z);
            if(!start.valid)
                throw std::invalid_argument("Sitka terrain has no walkable dry spawn");
        }
        pose_={static_cast<float>(start_x),start.y,static_cast<float>(start_z)};
    }
    RoamPose pose()const{return pose_;}
    // Exposed to the author-sourced beach discovery admission path.
    // Ground samples are georeferenced terrain, not invented collectible land.
    GroundHit GroundAt(double x,double z)const{
        const auto sample=ground_.Sample(x,z,pose_.ground_y);
        if(!sample.valid||!CanStand(x,z,sample.y))return {};
        return sample;
    }
    size_t accepted_steps()const{return accepted_;}
    size_t blocked_steps()const{return blocked_;}
    bool Advance(float strafe,float forward,const float right[3],float seconds){
        if(!right||!std::isfinite(strafe)||!std::isfinite(forward)||
           !std::isfinite(right[0])||!std::isfinite(right[2])||
           !std::isfinite(seconds)||seconds<=0.)return false;
        const double right_length=std::hypot(right[0],right[2]);
        if(right_length<.5||right_length>1.5)return false;
        const float x=std::clamp(strafe,-1.f,1.f);
        const float y=std::clamp(forward,-1.f,1.f);
        const float magnitude=std::hypot(x,y);
        if(magnitude<=kDeadzone)return false;
        const double strength=std::clamp(
            (double(magnitude)-kDeadzone)/(1.-kDeadzone),0.,1.);
        const double d=kSpeedMps*std::min(seconds,kMaxFrameSeconds)*strength;
        const double strafe_unit=x/magnitude,forward_unit=y/magnitude;
        const double rx=right[0]/right_length,rz=right[2]/right_length;
        const double dx=d*(strafe_unit*rx+forward_unit*rz);
        const double dz=d*(strafe_unit*rz-forward_unit*rx);
        const unsigned n=std::max(1u,static_cast<unsigned>(
            std::ceil(std::hypot(dx,dz)/.12)));
        bool moved=false;
        for(unsigned i=0;i<n;++i){
            const double stepx=dx/n,stepz=dz/n;
            if(TryStep(stepx,stepz)||TryStep(stepx,0.)||TryStep(0.,stepz))
                moved=true;
            else {++blocked_;break;}
        }
        return moved;
    }
    bool ShiftEye(const ReverieNativeEyeV1* eye,ReverieNativeEyeV1& out)const{
        if(!ReverieNativeEyeRenderableV1(eye))return false;
        out=*eye;
        const float transform[16]={
            1,0,0,0, 0,1,0,0, 0,0,1,0,
            -pose_.x,-pose_.ground_y,-pose_.z,1};
        ReverieNativeMat4Multiply(out.view,eye->view,transform);
        for(float value:out.view)if(!std::isfinite(value))return false;
        return true;
    }
private:
    static bool WithinPolygon(const std::vector<ShorePoint>& polygon,
                              double x,double z){
        bool inside=false;
        for(size_t i=0,j=polygon.size()-1u;i<polygon.size();j=i++){
            const auto& a=polygon[i];
            const auto& b=polygon[j];
            const double dx=b.x-a.x,dz=b.z-a.z;
            const double length2=dx*dx+dz*dz;
            if(length2>1.e-10){
                const double t=std::clamp(
                    ((x-a.x)*dx+(z-a.z)*dz)/length2,0.,1.);
                if(std::hypot(x-(a.x+t*dx),z-(a.z+t*dz))<=kFootRadius)
                    return true;
            }
            if((a.z>z)!=(b.z>z) &&
               x<(b.x-a.x)*(z-a.z)/(b.z-a.z)+a.x)inside=!inside;
        }
        return inside;
    }
    bool CanStand(double x,double z,double y)const{
        // A beach is walkable if its real land triangles are above simulated
        // tide; submerged seabed remains water, not free-roam terrain.
        for(const auto& water:*waters_)
            if(FootprintContains(water,static_cast<float>(x),
                                static_cast<float>(z)) &&
               y<water.y+.05)return false;
        for(const auto& f:*structures_){
            const bool solid=f.material=="storefront"||
                f.material=="cannery"||f.material=="heritage"||
                f.material=="warehouse"||f.material=="harbor_shack"||
                f.material=="pile";
            if(!solid||y<f.base-1.8||y>f.base+f.height+.25)continue;
            if(f.polygon){
                if(WithinPolygon(f.footprint,x,z))return false;
            }else{
                const double dx=x-f.x,dz=z-f.z;
                const double c=std::cos(f.yaw),s=std::sin(f.yaw);
                const double a=dx*c+dz*s,b=-dx*s+dz*c;
                if(std::abs(a)<=f.width*.5+kFootRadius&&
                   std::abs(b)<=f.depth*.5+kFootRadius)return false;
            }
        }
        return true;
    }
    bool TryStep(double dx,double dz){
        if(dx==0.&&dz==0.)return false;
        const double x=double(pose_.x)+dx,z=double(pose_.z)+dz;
        const GroundHit hit=ground_.Sample(x,z,pose_.ground_y);
        if(!hit.valid||std::abs(hit.y-pose_.ground_y)>kMaxStepHeight||
           !CanStand(x,z,hit.y))return false;
        pose_={static_cast<float>(x),hit.y,static_cast<float>(z)};
        ++accepted_;
        return true;
    }
    RoamTerrainIndex ground_;
    const std::vector<Water>* waters_=nullptr;
    const std::vector<ShoreFeature>* structures_=nullptr;
    RoamPose pose_{};
    size_t accepted_=0u,blocked_=0u;
};
}} // namespace reverie::sitka
#endif
