#ifndef REVERIE_SITKA_SHORE_STREAM_H
#define REVERIE_SITKA_SHORE_STREAM_H

// Game-private shore visibility/LOD selection. Feature metadata is resident;
// geometry is generated only when a meaningful distance tier changes. All
// source X/Z are local to terrain.sitkam's double-precision projected origin.
// Source footprint topology/IDs are never changed, and no fictional roads,
// entry points, buildings, or charted water are synthesized here.
#include "sitka_shore_detail.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace reverie { namespace sitka {

struct ShoreBounds {double low_x=0,low_z=0,high_x=0,high_z=0;};

inline ShoreBounds ShoreFeatureBounds(const ShoreFeature& f) {
    ShoreBounds b;
    if(f.polygon){
        if(f.footprint.empty())throw std::invalid_argument("Empty shore footprint");
        b={f.footprint[0].x,f.footprint[0].z,
           f.footprint[0].x,f.footprint[0].z};
        for(const auto& p:f.footprint){
            b.low_x=std::min(b.low_x,double(p.x));
            b.high_x=std::max(b.high_x,double(p.x));
            b.low_z=std::min(b.low_z,double(p.z));
            b.high_z=std::max(b.high_z,double(p.z));
        }
    }else{
        if(!std::isfinite(f.x)||!std::isfinite(f.z)||
           !std::isfinite(f.yaw)||!std::isfinite(f.width)||
           !std::isfinite(f.depth)||f.width<=0.f||f.depth<=0.f)
            throw std::invalid_argument("Invalid shore block bounds");
        const double c=std::cos(f.yaw),s=std::sin(f.yaw);
        const double ex=0.5*(std::abs(c)*f.width+std::abs(s)*f.depth);
        const double ez=0.5*(std::abs(s)*f.width+std::abs(c)*f.depth);
        b={double(f.x)-ex,double(f.z)-ez,
           double(f.x)+ex,double(f.z)+ez};
    }
    for(double v:{b.low_x,b.low_z,b.high_x,b.high_z})
        if(!std::isfinite(v))throw std::invalid_argument("Invalid shore bounds");
    return b;
}

inline double ShoreBoundDistanceSquared(const ShoreBounds& b,double x,double z){
    const double dx=x<b.low_x?b.low_x-x:x>b.high_x?x-b.high_x:0.;
    const double dz=z<b.low_z?b.low_z-z:z>b.high_z?z-b.high_z:0.;
    return dx*dx+dz*dz;
}

class ShoreStreamPlanner {
public:
    enum : uint8_t { Hidden=0, Massing=1, Detailed=2 };
    static constexpr double kDetailEnterM=80.,kDetailExitM=112.;
    static constexpr double kVisibleEnterM=440.,kVisibleExitM=490.;
    static constexpr unsigned kRetryFrames=30u;
    static constexpr size_t kMaxVisibleVertices=60000u;
    static constexpr size_t kMaxVisibleTriangles=90000u;

    // P1 procedural windows are *illustrative*, not independently proven.
    // Production starts with NO detail approvals. An authored and audited
    // facade reference must explicitly opt a stable site ID into P1 geometry.
    explicit ShoreStreamPlanner(std::vector<ShoreFeature> features,
              std::set<std::string> approved_details = {})
        :features_(std::move(features)),
         approved_details_(std::move(approved_details)){
        if(features_.size()>kMaxShoreFeatures)
            throw std::invalid_argument("Shore feature count exceeds file contract");
        bounds_.reserve(features_.size());
        for(const auto& f:features_){
            bounds_.push_back(ShoreFeatureBounds(f));
        }
        for(const auto& detail_id:approved_details_){
            const auto match=std::find_if(features_.begin(),features_.end(),
                [&](const ShoreFeature& f){return f.id==detail_id;});
            if(match==features_.end())
                throw std::invalid_argument("Unknown Sitka approved detail ID");
        }
        committed_.assign(features_.size(),Hidden);
        pending_=committed_;
    }

    // Runs once per simulation frame. It never rebuilds meshes or touches GL.
    // A rejected upload backs off; changes are not committed until GPU success.
    bool Update(double east,double north,double origin_east,double origin_north){
        if(!std::isfinite(east)||!std::isfinite(north)||
           !std::isfinite(origin_east)||!std::isfinite(origin_north))return false;
        const double x=east-origin_east,z=north-origin_north;
        if(!std::isfinite(x)||!std::isfinite(z)||
           std::abs(x)>1000000.||std::abs(z)>1000000.)return false;
        if(backoff_>0u){--backoff_;return false;}
        pending_=committed_;
        bool changed=false;
        for(size_t i=0;i<features_.size();++i){
            const double d2=ShoreBoundDistanceSquared(bounds_[i],x,z);
            const uint8_t previous=committed_[i];
            uint8_t tier=Hidden;
            if(previous==Detailed){
                tier=d2<=kDetailExitM*kDetailExitM?Detailed:
                    d2<=kVisibleExitM*kVisibleExitM?Massing:Hidden;
            }else if(previous==Massing){
                tier=d2<=kDetailEnterM*kDetailEnterM?Detailed:
                    d2<=kVisibleExitM*kVisibleExitM?Massing:Hidden;
            }else{
                tier=d2<=kDetailEnterM*kDetailEnterM?Detailed:
                    d2<=kVisibleEnterM*kVisibleEnterM?Massing:Hidden;
            }
            // A real footprint alone never proves its windows, awnings,
            // railings or facade proportions. With no explicit approval,
            // keep accurate site-anchored P0 massing even at arm's length.
            if(tier==Detailed && approved_details_.count(features_[i].id)==0u)
                tier=Massing;
            pending_[i]=tier;
            changed|=(tier!=previous);
        }
        staged_=changed || !initialized_;
        return staged_;
    }

    // Build only the pending visible features. Caller stages this in the
    // existing transaction-safe GL batch uploader before calling Commit().
    TerrainMesh BuildPending() const {
        if(!staged_)throw std::logic_error("No pending shore geometry");
        TerrainMesh mesh;
        std::vector<ShoreFeature> detailed;
        detailed.reserve(features_.size());
        for(size_t i=0;i<features_.size();++i){
            if(pending_[i]==Hidden)continue;
            if(features_[i].polygon)ShorePolygon(mesh,features_[i]);
            else ShoreBox(mesh,features_[i]);
            if(pending_[i]==Detailed)detailed.push_back(features_[i]);
            if(mesh.vertices.size()>kMaxVisibleVertices ||
               mesh.indices.size()/3u>kMaxVisibleTriangles)
                throw std::invalid_argument("Shore massing exceeds resident budget");
        }
        if(!detailed.empty())AppendShoreDetails(mesh,detailed);
        if(mesh.vertices.size()>kMaxVisibleVertices ||
           mesh.indices.size()/3u>kMaxVisibleTriangles)
            throw std::invalid_argument("Shore detail exceeds resident budget");
        return mesh;
    }

    void Commit(){
        if(!staged_)throw std::logic_error("No pending shore LOD commit");
        committed_=pending_;
        staged_=false;
        initialized_=true;
        ++revision_;
    }
    void Reject(){staged_=false;backoff_=kRetryFrames;}
    void Reset(){
        std::fill(committed_.begin(),committed_.end(),Hidden);
        pending_=committed_;
        initialized_=false;staged_=false;backoff_=0u;revision_=0u;
    }
    size_t visible_count()const{
        return static_cast<size_t>(std::count_if(committed_.begin(),
            committed_.end(),[](uint8_t tier){return tier!=Hidden;}));
    }
    size_t detailed_count()const{
        return static_cast<size_t>(std::count(committed_.begin(),
                                             committed_.end(),Detailed));
    }
    bool staged()const{return staged_;}
    uint64_t revision()const{return revision_;}
    unsigned retry_frames()const{return backoff_;}
    uint8_t tier(size_t i)const{return committed_.at(i);}
private:
    std::vector<ShoreFeature> features_;
    std::set<std::string> approved_details_;
    std::vector<ShoreBounds> bounds_;
    std::vector<uint8_t> committed_,pending_;
    bool initialized_=false,staged_=false;
    unsigned backoff_=0u;
    uint64_t revision_=0u;
};

}} // namespace reverie::sitka
#endif
