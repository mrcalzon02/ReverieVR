#ifndef REVERIE_OMEGA_FRONT_SOUND_H
#define REVERIE_OMEGA_FRONT_SOUND_H

#include "omega_front_tactics.h"
#include "reverie_native_module.h"
#include <cstdint>

namespace reverie { namespace omega_audio {

// Audio is a *consumer* of combat state. No RNG, simulation mutation,
// PCM generation, or stereo-eye callbacks occur here. IDs reuse the
// host's existing native sound bank; a later licensed sample bank can
// replace the host waveforms without changing these game event contracts.
inline uint32_t WeaponCue(omega::Class role) {
    switch(role) {
    case omega::Class::Rifle:     return REVERIE_NATIVE_SOUND_INFANTRY_VOLLEY;
    case omega::Class::Engineer:  return REVERIE_NATIVE_SOUND_WEAPON_MACHINE;
    case omega::Class::Heavy:     return REVERIE_NATIVE_SOUND_ARMORED_GUN;
    case omega::Class::Salvager:  return REVERIE_NATIVE_SOUND_WEAPON_HEAVY;
    case omega::Class::Sentinel:  return REVERIE_NATIVE_SOUND_WEAPON_MACHINE;
    case omega::Class::Legionary: return REVERIE_NATIVE_SOUND_HULL_IMPACT;
    }
    return 0u;
}

// One event survives per logical tick in Battle::last_event. Skip stale
// checkpoints, orders that do not fire, and bad actor IDs; never replay
// audio when the same paused frame renders for both VR eyes.
inline uint32_t TickFireCue(const omega::Battle &battle,uint32_t before_tick) {
    if(battle.ticks()<=before_tick)return 0u;
    const omega::Event event=battle.last_event();
    if(event.tick!=battle.ticks())return 0u;
    const omega::Unit *actor=battle.unit(event.actor);
    if(!actor)return 0u;
    return WeaponCue(actor->role);
}

inline uint32_t OutcomeCue(omega::Result before,omega::Result after) {
    if(before!=omega::Result::Active||after==omega::Result::Active)return 0u;
    return after==omega::Result::Defended?
           REVERIE_NATIVE_SOUND_WAVE_CLEAR:
           REVERIE_NATIVE_SOUND_BATTERY_LOST;
}

}} // namespace reverie::omega_audio
#endif
