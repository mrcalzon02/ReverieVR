#include "omega_front_sound.h"
#include <cassert>
#include <iostream>

using namespace reverie::omega;
namespace audio=reverie::omega_audio;
int main(){
    assert(audio::WeaponCue(Class::Rifle)==REVERIE_NATIVE_SOUND_INFANTRY_VOLLEY);
    assert(audio::WeaponCue(Class::Heavy)==REVERIE_NATIVE_SOUND_ARMORED_GUN);
    assert(audio::WeaponCue(Class::Engineer)==REVERIE_NATIVE_SOUND_WEAPON_MACHINE);
    assert(audio::WeaponCue(Class::Salvager)==REVERIE_NATIVE_SOUND_WEAPON_HEAVY);
    assert(audio::WeaponCue(Class::Sentinel)==REVERIE_NATIVE_SOUND_WEAPON_MACHINE);
    assert(audio::WeaponCue(Class::Legionary)==REVERIE_NATIVE_SOUND_HULL_IMPACT);
    assert(audio::OutcomeCue(Result::Active,Result::Active)==0);
    assert(audio::OutcomeCue(Result::Active,Result::Defended)==REVERIE_NATIVE_SOUND_WAVE_CLEAR);
    assert(audio::OutcomeCue(Result::Active,Result::Overrun)==REVERIE_NATIVE_SOUND_BATTERY_LOST);
    assert(audio::OutcomeCue(Result::Defended,Result::Defended)==0);
    assert(audio::OutcomeCue(Result::Overrun,Result::Active)==0);

    // A real, deterministic combat pulse produces an audible event just
    // once. Merely rendering twice/observing a saved tick cannot replay it.
    Battle battle(148);
    battle.ClearUnits();
    const int rifle=battle.AddUnit(Side::Hearthwell,Class::Rifle,4,10);
    const int heavy=battle.AddUnit(Side::Raiders,Class::Heavy,5,10);
    assert(rifle==0 && heavy==1);
    assert(battle.QueueAttack(rifle,heavy));
    assert(audio::TickFireCue(battle,battle.ticks())==0);
    battle.Step();
    const Event shot=battle.last_event();
    assert(shot.tick==battle.ticks());
    assert(shot.actor<static_cast<uint8_t>(battle.unit_count()));
    const Unit *shooter=battle.unit(shot.actor);
    assert(shooter);
    const uint32_t cue=audio::TickFireCue(battle,0);
    assert(cue==audio::WeaponCue(shooter->role));
    assert(cue!=0);
    assert(audio::TickFireCue(battle,battle.ticks())==0);

    // An enemy approaching from out of range without firing is silent.
    Battle quiet(151);
    quiet.ClearUnits();
    assert(quiet.AddUnit(Side::Hearthwell,Class::Rifle,0,0)==0);
    assert(quiet.AddUnit(Side::Raiders,Class::Rifle,23,23)==1);
    quiet.Step();
    assert(quiet.last_event().tick==0);
    assert(audio::TickFireCue(quiet,0)==0);
    std::cout<<"Omega Front event-driven combat audio contract passed\n";
}
