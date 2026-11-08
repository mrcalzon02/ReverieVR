#ifndef REVERIE_BREAKWATER_AUDIO_H
#define REVERIE_BREAKWATER_AUDIO_H

// Game-private intent scheduling only. The shell owns decoded library samples,
// mixing, volume, loop leases, lifecycle shutdown and the physical audio device.
#include "reverie_native_sdk.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater { namespace audio {

enum class Priority : uint8_t { Outcome, Action, Threat, Impact };
inline Priority CuePriority(uint32_t cue) {
    if(cue>=REVERIE_NATIVE_SOUND_WAVE_CLEAR &&
       cue<=REVERIE_NATIVE_SOUND_BATTERY_LOST)return Priority::Outcome;
    if(cue<=REVERIE_NATIVE_SOUND_WEAPON_CANNON ||
       cue==REVERIE_NATIVE_SOUND_SAM_LAUNCH ||
       cue==REVERIE_NATIVE_SOUND_WAVE_START ||
       cue==REVERIE_NATIVE_SOUND_SHOP_PURCHASE ||
       (cue>=REVERIE_NATIVE_SOUND_BREAKWATER_WEAPON_EQUIP &&
        cue<=REVERIE_NATIVE_SOUND_BREAKWATER_SUPPORT_UNAVAILABLE &&
        cue!=REVERIE_NATIVE_SOUND_BREAKWATER_SAND_IMPACT))
        return Priority::Action;
    if(cue==REVERIE_NATIVE_SOUND_HULL_IMPACT ||
       cue==REVERIE_NATIVE_SOUND_WATER_SPLASH ||
       cue==REVERIE_NATIVE_SOUND_BOAT_DESTROYED ||
       cue==REVERIE_NATIVE_SOUND_ARMOR_DESTROYED ||
       cue==REVERIE_NATIVE_SOUND_BREAKWATER_SAND_IMPACT)
        return Priority::Impact;
    return Priority::Threat;
}
inline bool ControlConfirmation(uint32_t cue) {
    switch(cue) {
        case REVERIE_NATIVE_SOUND_SHOP_PURCHASE:
        case REVERIE_NATIVE_SOUND_BREAKWATER_WEAPON_EQUIP:
        case REVERIE_NATIVE_SOUND_BREAKWATER_SUPPORT_ARM:
        case REVERIE_NATIVE_SOUND_BREAKWATER_SUPPORT_DISARM:
        case REVERIE_NATIVE_SOUND_BREAKWATER_STATION_MOVE:
        case REVERIE_NATIVE_SOUND_BREAKWATER_MENU_OPEN:
        case REVERIE_NATIVE_SOUND_BREAKWATER_MENU_BACK:
        case REVERIE_NATIVE_SOUND_BREAKWATER_DIFFICULTY_SELECT:
        case REVERIE_NATIVE_SOUND_BREAKWATER_RESTART:return true;
        default:return false;
    }
}

// Exactly the host's eight one-shot slots. Stable priority insertion keeps the
// player's report ahead of its impact even when simulation events arrive first.
// Multiple same-frame hits share one report instead of clipping eight identical
// explosions. Nothing is carried into a later, semantically unrelated frame.
class Frame {
public:
    void Begin() { count_=0u;feedback_=0u; }
    void Feedback(uint32_t flags) { feedback_|=flags; }
    void Cue(uint32_t cue,uint32_t fallback=0u) {
        Cue(cue,CuePriority(cue),fallback);
    }
    void Cue(uint32_t cue,Priority priority,uint32_t fallback=0u) {
        for(unsigned i=0;i<count_;++i)if(events_[i].cue==cue) {
            fallback|=events_[i].fallback;
            priority=std::min(priority,events_[i].priority);
            for(unsigned j=i+1;j<count_;++j)events_[j-1]=events_[j];
            --count_;
            break;
        }
        unsigned at=0u;
        while(at<count_ && events_[at].priority<=priority)++at;
        if(at==events_.size())return;
        const unsigned end=std::min<unsigned>(count_,events_.size()-1u);
        for(unsigned i=end;i>at;--i)events_[i]=events_[i-1u];
        events_[at]={cue,fallback,priority};
        count_=std::min<unsigned>(count_+1u,events_.size());
    }
    void Flush(const ReverieNativeHostV1 *host) {
        uint32_t flags=feedback_;
        bool failed=(flags&REVERIE_NATIVE_FEEDBACK_FAILURE)!=0u;
        for(unsigned i=0;i<count_;++i)
            failed=failed || (events_[i].fallback&REVERIE_NATIVE_FEEDBACK_FAILURE)!=0u;
        bool action=false;
        for(unsigned i=0;i<count_;++i) {
            const Event &event=events_[i];
            // Decide before dispatch: stripping only the later bitmask cannot
            // retract a success sample already queued before a failed save.
            // Actual gunfire, damage and wave/outcome events remain audible.
            if(failed && ControlConfirmation(event.cue))continue;
            if(!ReverieNativeRequestSoundV1(host,event.cue))
                flags|=event.fallback;
            else if(event.priority<=Priority::Action)action=true;
        }
        // Focus never competes with a click/rejection/report on the same frame.
        // Legacy hosts without the optional sound service retain their original
        // feedback callbacks. The v1 contract cannot query a host's cue bank.
        if(action || (flags & ~REVERIE_NATIVE_FEEDBACK_FOCUS))
            flags&=~REVERIE_NATIVE_FEEDBACK_FOCUS;
        if(action)flags&=~REVERIE_NATIVE_FEEDBACK_ACTIVATION;
        if(flags & REVERIE_NATIVE_FEEDBACK_FAILURE)
            flags&=~REVERIE_NATIVE_FEEDBACK_ACTIVATION;
        if(flags)ReverieNativeRequestFeedbackV1(host,flags);
        Begin();
    }
private:
    struct Event {uint32_t cue=0u,fallback=0u;Priority priority=Priority::Impact;};
    std::array<Event,8> events_{};
    unsigned count_=0u;
    uint32_t feedback_=0u;
};

// Controller boundary jitter must not become a stream of ticks. Announce a
// stable actionable target once, rearming on a genuine leave/change of target.
class Focus {
public:
    bool Update(int target,float seconds,bool pressed) {
        cooldown_=std::max(0.f,cooldown_-seconds);
        if(target!=candidate_) { candidate_=target;stable_=0.f;announced_=false; }
        stable_+=seconds;
        if(target<0 || pressed || announced_ || stable_<.045f || cooldown_>0.f)
            return false;
        announced_=true;cooldown_=.10f;
        return true;
    }
    void Reset() {candidate_=-1;stable_=0.f;cooldown_=0.f;announced_=false;}
private:
    int candidate_=-1;
    float stable_=0.f,cooldown_=0.f;
    bool announced_=false;
};

struct Stereo {
    float left=0.f,right=0.f;
    bool Audible() const {return left>0.f || right>0.f;}
};
inline Stereo At(float x,float y,float z,const float *listener,
                 const float *right,float gain,float reach) {
    const float dx=x-listener[0],dy=y-listener[1],dz=z-listener[2];
    const float distance2=dx*dx+dy*dy+dz*dz;
    if(!std::isfinite(distance2) || !std::isfinite(gain) ||
       !std::isfinite(reach) || gain<=0.f || reach<=0.f)return {};
    const float level=std::clamp(gain/(1.f+distance2/(reach*reach)),0.f,1.f);
    if(level<.008f)return {};
    const float planar=std::hypot(dx,dz);
    const float pan=right && planar>.001f?
        std::clamp((dx*right[0]+dz*right[2])/planar,-1.f,1.f):0.f;
    return {level*(pan>0.f?1.f-.75f*pan:1.f),
            level*(pan<0.f?1.f+.75f*pan:1.f)};
}

enum Layer : unsigned { Surf, Wind, Night, Boat, Rowing, Aircraft, Traverse, Count };
static_assert(REVERIE_NATIVE_SOUND_BREAKWATER_GUN_TRAVERSE-
              REVERIE_NATIVE_SOUND_BREAKWATER_AMBIENCE_SURF+1u==Count,
              "Breakwater loop IDs are contiguous");

// The separate native spatial packet holds TWO events, not eight. Zero-gain
// stops get priority; all active layers then share a fair rotating two-slot
// refresh every 120 ms. Seven continuous layers revisit within 480 ms at the
// intended frame cadence, below the shell's 900 ms lease. Missing updates expire
// in the shell; its explicit pause/exit shutdown does not wait for this queue.
class Loops {
public:
    void ClearDesired() { desired_.fill({}); }
    void Set(Layer layer,Stereo volume) {
        desired_[layer]={Safe(volume.left),Safe(volume.right)};
    }
    void Reset() {
        desired_.fill({});playing_.fill(false);cursor_=0u;pulse_=0.f;
    }
    void Flush(const ReverieNativeHostV1 *host,float seconds) {
        pulse_=std::min(.24f,pulse_+seconds);
        unsigned budget=2u;
        // At most two stops per Update; unsent stops persist in playing_.
        for(unsigned i=0;i<Count && budget;++i) {
            if(!playing_[i] || desired_[i].Audible())continue;
            if(!Send(host,i,{}))return;
            playing_[i]=false;--budget;
        }
        const bool refresh=pulse_>=.12f;
        unsigned visited=0u;
        while(budget && visited<Count) {
            const unsigned i=(cursor_+visited)%Count;
            ++visited;
            if(!desired_[i].Audible() || (!refresh && playing_[i]))continue;
            if(!Send(host,i,desired_[i]))return;
            playing_[i]=true;--budget;
        }
        if(visited)cursor_=(cursor_+visited)%Count;
        if(refresh && visited)pulse_=std::fmod(pulse_,.12f);
    }
private:
    static float Safe(float value) {
        return std::isfinite(value)?std::clamp(value,0.f,1.f):0.f;
    }
    static bool Send(const ReverieNativeHostV1 *host,unsigned layer,Stereo volume) {
        return ReverieNativeRequestSpatialSoundV1(host,
            REVERIE_NATIVE_SOUND_BREAKWATER_AMBIENCE_SURF+layer,
            volume.left,volume.right)!=0;
    }
    std::array<Stereo,Count> desired_{};
    std::array<bool,Count> playing_{};
    unsigned cursor_=0u;
    float pulse_=0.f;
};

}}} // namespace reverie::breakwater::audio
#endif
