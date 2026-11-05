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
        if(!graph.Surface(n.x,n.z).valid)
            throw std::invalid_argument("Pedestrian start is not on a public path");
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
        const auto original=graph_->Surface(pose_.x,pose_.z);
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
    bool TryMove(float dx,float dz){
        if(dx==0.f&&dz==0.f)return false;
        const double x=double(pose_.x)+dx,z=double(pose_.z)+dz;
        const auto hit=graph_->Surface(x,z);
        if(!hit.valid ||
           std::abs(hit.y-pose_.ground_y)>kMaxStepHeight)return false;
        pose_.x=static_cast<float>(x);
        pose_.ground_y=hit.y;
        pose_.z=static_cast<float>(z);
        ++accepted_;
        return true;
    }
    const WalkGraph* graph_=nullptr;
    WalkPose pose_{};
    size_t accepted_=0u,blocked_=0u;
};
}} // namespace reverie::sitka
#endif
