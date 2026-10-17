#ifndef REVERIE_BREAKWATER_WRECKS_H
#define REVERIE_BREAKWATER_WRECKS_H

#include "breakwater_campaign.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace reverie { namespace breakwater {

// Destruction is presentation-only: an enemy must have been active before
// and actually killed by the authoritative simulation. Landing, escape,
// collision, or a merely damaged target cannot produce a fake wreck.
struct WreckPulse {
    float x=0.0f, z=0.0f, remaining=0.0f;
    UnitType type=UnitType::Rowboat;
    uint8_t variant=0;
};

class WreckEffects {
public:
    static constexpr size_t kCapacity=8;
    static constexpr float kDuration=1.8f;
    const std::array<WreckPulse,kCapacity>& pulses() const {return pulses_;}
    bool RecordDestroyed(const Enemy& before,const Enemy& after) {
        if(!before.active || after.active || before.hp<=0 || after.hp>0 ||
           !std::isfinite(before.x) || !std::isfinite(before.z) ||
           std::abs(before.x)>1000.0f || std::abs(before.z)>1000.0f ||
           (before.type!=UnitType::Rowboat &&
            before.type!=UnitType::LandingCraft &&
            before.type!=UnitType::ArmoredTransport &&
            before.type!=UnitType::Gunboat &&
            before.type!=UnitType::Aircraft)) return false;
        // Never evict a living wreck during a large simultaneous support
        // strike; an exhausted pool simply drops additional decoration.
        for(size_t n=0;n<kCapacity;++n) {
            const size_t index=(next_+n)%kCapacity;
            if(pulses_[index].remaining>0.0f)continue;
            const uint8_t variant=static_cast<uint8_t>(
                static_cast<int>(std::abs(before.x)*17.0f+
                    std::abs(before.z)*11.0f)%4);
            pulses_[index]={before.x,before.z,kDuration,before.type,variant};
            next_=(index+1)%kCapacity;
            return true;
        }
        return false;
    }
    size_t RecordTransitions(
        const std::array<Enemy,Campaign::kMaxEnemies>& before,
        const std::array<Enemy,Campaign::kMaxEnemies>& after
    ) {
        size_t count=0;
        for(size_t i=0;i<before.size();++i)
            if(RecordDestroyed(before[i],after[i]))++count;
        return count;
    }
    void Tick(float seconds) {
        if(!std::isfinite(seconds) || seconds<=0.0f)return;
        for(WreckPulse& pulse:pulses_)
            pulse.remaining=std::max(0.0f,pulse.remaining-seconds);
    }
    void Clear() {*this=WreckEffects{};}
private:
    std::array<WreckPulse,kCapacity> pulses_{};
    size_t next_=0;
};

// World-anchored, bounded fragment triangles. No RNG, particles, shaders,
// textures, clocks or per-eye simulation. All geometry is transient and
// separate from the campaign's target/collision model.
template<typename TriangleFn>
void EmitWreckFragments(const WreckPulse& wreck,TriangleFn triangle) {
    if(wreck.remaining<=0.0f || wreck.remaining>WreckEffects::kDuration ||
       !std::isfinite(wreck.x) || !std::isfinite(wreck.z) ||
       !std::isfinite(wreck.remaining) || std::abs(wreck.x)>1000.0f ||
       std::abs(wreck.z)>1000.0f)return;
    const bool aircraft=wreck.type==UnitType::Aircraft;
    if(!aircraft && wreck.type!=UnitType::Rowboat &&
       wreck.type!=UnitType::LandingCraft &&
       wreck.type!=UnitType::ArmoredTransport &&
       wreck.type!=UnitType::Gunboat)return;
    const float t=1.0f-wreck.remaining/WreckEffects::kDuration;
    const float scale=wreck.type==UnitType::ArmoredTransport?1.72f:
        wreck.type==UnitType::Gunboat?1.3f:
        wreck.type==UnitType::LandingCraft?1.13f:
        aircraft?1.10f:.75f;
    const float baseY=aircraft?1.6f:-.64f;
    const float sink=aircraft?1.55f*t*t:.60f*t;
    const float y=baseY-sink;
    const float grey=aircraft?.45f:.31f;
    // Four asymmetric metal shards diverge and tumble from the real
    // impact location, then vanish below the sea / flightline.
    for(int i=0;i<4;++i) {
        const float direction=i%2==0?1.0f:-1.0f;
        const float step=i<2?1.0f:.65f;
        const float vx=direction*(.13f+.40f*t)*scale*step;
        const float vz=(i<2?.17f:-.15f)*scale*t;
        const float spin=(i%2==0?t:-t)*.35f+
            .045f*static_cast<float>(wreck.variant%4u);
        const float w=(.24f-.09f*t)*scale;
        const float px=wreck.x+vx,pz=wreck.z+vz;
        const float py=y+(i%2==0?.12f:-.07f)*t+
            (aircraft?(i%2==0?.26f:-.14f)*t:0.0f);
        triangle(px-w,py,pz,px+w,py+spin,pz+.07f,
                 px,py+w*.65f,pz-.10f,
                 grey+.10f*(i%2),grey+.08f,grey+.06f);
        // A dark underside gives one extra depth cue at low resolution.
        triangle(px-w*.65f,py-.035f,pz+.025f,
                 px+w*.65f,py+spin-.035f,pz+.07f,
                 px,py-w*.40f,pz+.10f,
                 .13f,.16f,.17f);
    }
    // One brief, collapsing smoke ember above the original blast.
    if(t<.75f) {
        const float smokeScale=(.32f+.48f*t)*scale;
        const float fade=1.0f-t;
        triangle(wreck.x-smokeScale,y+.05f,wreck.z-.16f,
                 wreck.x,y+smokeScale,wreck.z-.12f,
                 wreck.x+smokeScale,y+.05f,wreck.z-.16f,
                 .16f+.28f*fade,.17f+.19f*fade,.17f+.10f*fade);
    }
}

}} // namespace reverie::breakwater
#endif
