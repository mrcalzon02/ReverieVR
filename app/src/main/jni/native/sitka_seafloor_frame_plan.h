#ifndef REVERIE_SITKA_SEAFLOOR_FRAME_PLAN_H
#define REVERIE_SITKA_SEAFLOOR_FRAME_PLAN_H
// Source-only, deterministic between-frame seabed tile and LOD selection.
// No graphics calls, no asset reads, no allocations in this decision path.
#include "sitka_bathymetry_stream.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
namespace reverie { namespace sitka {
struct SeafloorFrameKey {
    bool valid=false;
    bool sourced=false;
    int source_region=-1;
    int64_t grid_east=0,grid_north=0;
    unsigned stride=1u;
};
inline bool SameSeafloorKey(const SeafloorFrameKey& a,
                            const SeafloorFrameKey& b){
    return a.valid==b.valid&&
        (!a.valid||(a.sourced==b.sourced&&
        a.source_region==b.source_region&&
        a.grid_east==b.grid_east&&a.grid_north==b.grid_north&&
        a.stride==b.stride));
}
class SeafloorFramePlanner {
public:
    void Reset(){
        current_={};wanted_={};
        depth_lod_=2u;cooldown_=0u;
        committed_swaps_=0u;
    }
    // Sampling distance is vertical boat/camera distance to simulated floor.
    // Enter/exit thresholds have margin so the same tile does not bounce
    // between mesh detail levels on successive swell/tide samples.
    unsigned ChooseLod(double floor_distance_m){
        if(!std::isfinite(floor_distance_m)||floor_distance_m<0.)
            return depth_lod_;
        if(depth_lod_==1u){
            if(floor_distance_m>64.)depth_lod_=2u;
        }else if(depth_lod_==2u){
            if(floor_distance_m<44.)depth_lod_=1u;
            else if(floor_distance_m>180.)depth_lod_=4u;
        }else{
            if(floor_distance_m<140.)depth_lod_=2u;
        }
        return depth_lod_;
    }
    // Returns true only for a new tile, a source-data arrival, or a stable
    // LOD threshold crossing. If on water with no source, generate a
    // separately tagged procedural tile rather than fabricating survey data.
    bool Update(double east,double north,double distance_m,
                const BathyStreamCache* cache,bool generation_allowed){
        if(!std::isfinite(east)||!std::isfinite(north)||
           std::abs(east)>1e8||std::abs(north)>1e8)
            return false;
        const bool use_source=cache&&cache->ResidentAt(east,north)!=nullptr;
        if(!use_source&&!generation_allowed){
            wanted_={};
        }else{
            SeafloorFrameKey next{};
            next.valid=true;
            next.sourced=use_source;
            if(use_source){
                next.source_region=cache->directory().Locate(east,north);
                const BathyTile* tile=cache->ResidentAt(east,north);
                if(!tile)return false;
                // A 513-sample grid at LOD0 would require >260k vertices.
                // Bound single-tile GPU upload to ~17k vertices.
                const unsigned limit=std::max(tile->rows,tile->columns);
                next.stride=std::max(ChooseLod(distance_m),
                                    limit>257u?4u:limit>129u?2u:1u);
            }else{
                // Matches GenerateTile: floor east and ceil north region.
                next.grid_east=static_cast<int64_t>(std::floor(east/256.));
                next.grid_north=static_cast<int64_t>(std::ceil(north/256.));
                next.stride=ChooseLod(distance_m);
            }
            // The boat can jitter around an authored cell border.
            // Keep the already-rendered tile until 8m INSIDE the new
            // cell, while sonar sampling itself continues across the
            // true geographic boundary without delayed updates.
            if(current_.valid&&current_.sourced==next.sourced&&
               !SameSeafloorKey(current_,next)&&
               (current_.sourced?
                 current_.source_region!=next.source_region:
                 current_.grid_east!=next.grid_east||
                 current_.grid_north!=next.grid_north)){
                double west=0.,right=0.,south=0.,top=0.;
                if(next.sourced){
                    const auto& region=cache->directory().regions[
                        static_cast<size_t>(next.source_region)];
                    west=region.first_east;right=region.EastEnd();
                    south=region.SouthEnd();top=region.first_north;
                }else{
                    west=256.*static_cast<double>(next.grid_east);
                    right=west+256.;
                    top=256.*static_cast<double>(next.grid_north);
                    south=top-256.;
                }
                const double margin=std::min({east-west,right-east,
                    north-south,top-north});
                if(margin<8.){
                    wanted_=current_;
                    return false;
                }
            }
            wanted_=next;
        }
        if(SameSeafloorKey(wanted_,current_))return false;
        // An unrelated new tile bypasses retry cooldown; a broken
        // repeated upload gets only one attempt per 30 update frames.
        if(cooldown_>0u&&SameSeafloorKey(wanted_,last_failed_)){
            --cooldown_;return false;
        }
        cooldown_=0u;
        return true;
    }
    const SeafloorFrameKey& desired()const{return wanted_;}
    const SeafloorFrameKey& committed()const{return current_;}
    void Commit(){current_=wanted_;cooldown_=0u;++committed_swaps_;}
    void Reject(){last_failed_=wanted_;cooldown_=30u;}
    size_t committed_swaps()const{return committed_swaps_;}
private:
    SeafloorFrameKey current_{},wanted_{},last_failed_{};
    unsigned depth_lod_=2u,cooldown_=0u;
    size_t committed_swaps_=0u;
};
}} // namespace
#endif
