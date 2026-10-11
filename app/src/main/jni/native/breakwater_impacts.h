#ifndef REVERIE_BREAKWATER_IMPACTS_H
#define REVERIE_BREAKWATER_IMPACTS_H
#include "breakwater_campaign.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace reverie { namespace breakwater {
enum class ImpactKind : uint8_t { Water, Metal, Sand, Explosion, ArtilleryBlast, AirstrikeBlast };
struct ImpactPulse {
    float x=0, y=0, z=0, remaining=0;
    ImpactKind kind=ImpactKind::Water;
};
class ImpactEffects {
public:
    static constexpr size_t kCapacity=8;
    static constexpr float kLifetime=.30f;
    static constexpr float kExplosionLifetime=.58f;
    static constexpr float kArtilleryLifetime=.70f;
    static constexpr float kAirstrikeLifetime=.82f;
    static constexpr float LifetimeFor(ImpactKind kind) {
        return kind==ImpactKind::Explosion?kExplosionLifetime:
               kind==ImpactKind::ArtilleryBlast?kArtilleryLifetime:
               kind==ImpactKind::AirstrikeBlast?kAirstrikeLifetime:kLifetime;
    }
    const std::array<ImpactPulse,kCapacity> &pulses() const { return pulses_; }
    bool RecordHit(const float *origin,const float *direction,float distance,
                   ImpactKind kind) {
        if(!origin || !direction || !std::isfinite(distance) ||
           distance<=0 || distance>40) return false;
        float point[3]={};
        double magnitude=0;
        for(int axis=0;axis<3;axis++) {
            if(!std::isfinite(origin[axis]) || !std::isfinite(direction[axis]))
                return false;
            magnitude+=static_cast<double>(direction[axis])*direction[axis];
            const double position=static_cast<double>(origin[axis])+
                                  static_cast<double>(direction[axis])*distance;
            if(!std::isfinite(position) || std::abs(position)>10000) return false;
            point[axis]=static_cast<float>(position);
        }
        if(magnitude<=0 || !std::isfinite(magnitude)) return false;
        pulses_[next_]={point[0],point[1],point[2],
            LifetimeFor(kind),kind};
        next_=(next_+1)%kCapacity;
        return true;
    }
    // Only a damaging round creates an impact. In particular, basic gun
    // rounds against aircraft without anti-air must not fake a hit spark.
    bool RecordDamageHit(const float *origin,const float *direction,float distance,
                         int16_t beforeHp,int16_t afterHp,bool stillActive) {
        if(afterHp>=beforeHp) return false;
        return RecordHit(origin,direction,distance,
                         stillActive?ImpactKind::Metal:ImpactKind::Explosion);
    }
    // Compare the exact pre/post-strike enemy slots. This preserves the
    // position of a lethal target and never invents blasts for rejected calls,
    // inactive targets, or aircraft ignored by surface artillery.
    size_t RecordSupportDamage(const std::array<Enemy,Campaign::kMaxEnemies> &before,
                               const std::array<Enemy,Campaign::kMaxEnemies> &after,
                               bool airstrike) {
        size_t recorded=0;
        for(size_t i=0;i<before.size() && recorded<kCapacity;i++) {
            const Enemy &target=before[i];
            if(!target.active || (!airstrike && target.type==UnitType::Aircraft) ||
               after[i].hp>=target.hp) continue;
            const float y=target.type==UnitType::Aircraft?1.6f:-.63f;
            if(RecordWorldBlast(target.x,y,target.z,airstrike?
                ImpactKind::AirstrikeBlast:ImpactKind::ArtilleryBlast)) ++recorded;
        }
        return recorded;
    }
    bool RecordWaterMiss(const float *origin,const float *direction) {
        if(!origin || !direction || !std::isfinite(origin[1]) ||
           !std::isfinite(direction[1]) || direction[1]>=-.0001f)
            return false;
        const float distance=(-1.10f-origin[1])/direction[1];
        return RecordHit(origin,direction,distance,ImpactKind::Water);
    }
    void Tick(float seconds) {
        if(!std::isfinite(seconds) || seconds<=0) return;
        for(ImpactPulse &pulse:pulses_)
            pulse.remaining=std::fmax(0.0f,pulse.remaining-seconds);
    }
private:
    bool RecordWorldBlast(float x,float y,float z,ImpactKind kind) {
        if(!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
           std::abs(x)>10000 || std::abs(y)>10000 || std::abs(z)>10000)
            return false;
        pulses_[next_]={x,y,z,LifetimeFor(kind),kind};
        next_=(next_+1)%kCapacity;
        return true;
    }
    std::array<ImpactPulse,kCapacity> pulses_{};
    size_t next_=0;
};
}} // namespace reverie::breakwater
#endif
