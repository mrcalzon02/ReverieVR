#ifndef REVERIE_SITKA_WALK_SESSION_H
#define REVERIE_SITKA_WALK_SESSION_H

// Game-owned grounded walking transform for Sitka's sourced foot network.
// No synthetic path, noclip flight, camera bob, terrain-based bridge height,
// frame-rate-dependent acceleration, or host-owned global locomotion change.
// This source-stage adapter is for joystick travel; Daydream touchpad input
// mapping, public city assets and Playable Sitka registration remain separate.
#include "sitka_walk_graph.h"
#include "reverie_native_sdk.h"
#include "reverie_native_math.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace reverie { namespace sitka {

struct WalkPose {float x=0.f,ground_y=0.f,z=0.f;};
class WalkSession {
public:
    static constexpr float kSpeedMps=1.45f;
    static constexpr float kMaxFrameSeconds=.05f;
    static constexpr float kMaxStepHeight=.30f;

    explicit WalkSession(const WalkGraph& graph):graph_(&graph){
        if(graph.nodes.empty()||graph.spawn>=graph.nodes.size())
            throw std::invalid_argument("No authored pedestrian spawn node");
        const auto& n=graph.nodes[graph.spawn];
        pose_={n.x,n.y,n.z};
        const WalkSurface start=graph.Surface(n.x,n.z);
        if(!start.valid)
            throw std::invalid_argument("Pedestrian start is not on a public path");
        active_edge_=start.edge;
    }
    WalkPose pose()const{return pose_;}
    size_t accepted_steps()const{return accepted_;}
    size_t blocked_steps()const{return blocked_;}

    // The host supplies a normalized horizontal headset-right vector and
    // gamepad axes. Forward is right-rotated counterclockwise in the game's
    // right-handed X/Z coordinates. All walking is checked against the
    // same source-connected corridor mesh used for bank-fishing admission.
    bool Advance(float strafe,float forward,const float right[3],float dt){
        if(!right||!std::isfinite(strafe)||!std::isfinite(forward)||
           !std::isfinite(right[0])||!std::isfinite(right[2])||
           !std::isfinite(dt)||dt<=0.f)return false;
        const double rightlen=std::hypot(right[0],right[2]);
        if(rightlen<.5||rightlen>1.5)return false;
        strafe=std::max(-1.f,std::min(1.f,strafe));
        forward=std::max(-1.f,std::min(1.f,forward));
        const float axislen=std::hypot(strafe,forward);
        if(axislen<.12f)return false;
        const float scale=kSpeedMps*std::min(dt,kMaxFrameSeconds)/
                          std::max(1.f,axislen);
        const double rx=right[0]/rightlen,rz=right[2]/rightlen;
        const float dx=static_cast<float>(
            scale*(strafe*rx+forward*rz));
        const float dz=static_cast<float>(
            scale*(strafe*rz-forward*rx));
        if(TryMove(dx,dz))return true;
        // Slide along the authored corridor if the user angles into a
        // seawall or kerb. This cannot cross the edge or invent a junction.
        const auto original=ActiveSurface(pose_.x,pose_.z);
        if(original.valid){
            const auto& e=graph_->edges[original.edge];
            const auto& a=graph_->nodes[e.a],&b=graph_->nodes[e.b];
            const double vx=b.x-a.x,vz=b.z-a.z;
            const double len=std::hypot(vx,vz);
            if(len>.1){
                const double dot=(dx*vx+dz*vz)/len;
                if(TryMove(static_cast<float>(dot*vx/len),
                           static_cast<float>(dot*vz/len)))return true;
            }
        }
        ++blocked_;
        return false;
    }

    // A copy of the two-eye input: translate THE WORLD by the current
    // authored walkway pose before normal view projection. Do not modify
    // the shared headset pose, IPD, native shell locomotion, or input buffer.
    bool ShiftEye(const ReverieNativeEyeV1* eye,ReverieNativeEyeV1& out)const{
        if(!ReverieNativeEyeRenderableV1(eye))return false;
        out=*eye;
        const float transform[16]={
            1,0,0,0, 0,1,0,0, 0,0,1,0,
            -pose_.x,-pose_.ground_y,-pose_.z,1};
        ReverieNativeMat4Multiply(out.view,eye->view,transform);
        for(float v:out.view)if(!std::isfinite(v))return false;
        return true;
    }
private:
    // A sampled point on any nearby graph edge is NOT permission to switch
    // corridors. Keep the active sourced segment even at an X/Z crossing.
    WalkSurface ActiveSurface(double x,double z)const{
        if(active_edge_>=graph_->edges.size()||
           !std::isfinite(x)||!std::isfinite(z))return {};
        const auto& e=graph_->edges[active_edge_];
        const auto& a=graph_->nodes[e.a];
        const auto& b=graph_->nodes[e.b];
        const double vx=double(b.x)-a.x,vz=double(b.z)-a.z;
        const double len2=vx*vx+vz*vz;
        if(len2<.01)return {};
        const double t=std::clamp(((x-a.x)*vx+(z-a.z)*vz)/len2,0.,1.);
        const double dx=x-a.x-t*vx,dz=z-a.z-t*vz;
        const double distance=std::hypot(dx,dz);
        if(distance>double(e.width)*.5+.25)return {};
        return {true,static_cast<float>(a.y+t*(b.y-a.y)),
                active_edge_,static_cast<float>(distance)};
    }
    // A legal turn must pass through a shared, explicitly authored endpoint.
    // Planar crossings and near-misses are not graph junctions.
    bool CanEnter(size_t next,double x,double z)const{
        if(active_edge_>=graph_->edges.size()||
           next>=graph_->edges.size())return false;
        const auto& current=graph_->edges[active_edge_];
        const auto& target=graph_->edges[next];
        const size_t missing=graph_->nodes.size();
        size_t junction=missing;
        if(current.a==target.a||current.a==target.b)junction=current.a;
        else if(current.b==target.a||current.b==target.b)junction=current.b;
        if(junction==missing)return false;
        const auto& node=graph_->nodes[junction];
        const double radius=std::max(.5,
            .5*std::max(double(current.width),double(target.width))+.5);
        return std::hypot(double(pose_.x)-node.x,
                          double(pose_.z)-node.z)<=radius &&
               std::hypot(x-node.x,z-node.z)<=radius;
    }
    bool TryMove(float dx,float dz){
        if(dx==0.f&&dz==0.f)return false;
        const double x=double(pose_.x)+dx,z=double(pose_.z)+dz;
        WalkSurface hit=ActiveSurface(x,z);
        if(!hit.valid){
            hit=graph_->Surface(x,z);
            if(!hit.valid||!CanEnter(hit.edge,x,z))return false;
        }
        if(std::abs(hit.y-pose_.ground_y)>kMaxStepHeight)return false;
        pose_.x=static_cast<float>(x);
        pose_.ground_y=hit.y;
        pose_.z=static_cast<float>(z);
        active_edge_=hit.edge;
        ++accepted_;
        return true;
    }
    const WalkGraph* graph_=nullptr;
    WalkPose pose_{};
    size_t active_edge_=0u;
    size_t accepted_=0u,blocked_=0u;
};
}} // namespace reverie::sitka
#endif
