#ifndef REVERIE_BREAKWATER_IMPACTS_H
#define REVERIE_BREAKWATER_IMPACTS_H
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace reverie { namespace breakwater {
enum class ImpactKind : uint8_t { Water, Metal, Sand };
struct ImpactPulse {
    float x=0, y=0, z=0, remaining=0;
    ImpactKind kind=ImpactKind::Water;
};
class ImpactEffects {
public:
    static constexpr size_t kCapacity=8;
    static constexpr float kLifetime=.30f;
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
        pulses_[next_]={point[0],point[1],point[2],kLifetime,kind};
        next_=(next_+1)%kCapacity;
        return true;
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
    std::array<ImpactPulse,kCapacity> pulses_{};
    size_t next_=0;
};
}} // namespace reverie::breakwater
#endif
