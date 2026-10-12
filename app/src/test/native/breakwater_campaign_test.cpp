#include "breakwater_campaign.h"
#include "breakwater_combat_input.h"
#include "breakwater_impacts.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>
using namespace reverie::breakwater;

int main() {
    // Support labels consume held Select after the initial action edge.
    // They must not fire the gun through the HUD or spend ammunition.
    assert(HitCombatSupportHud(-2.0f,.95f)==CombatHudTarget::Artillery);
    assert(HitCombatSupportHud(2.0f,.95f)==CombatHudTarget::Airstrike);
    assert(HitCombatSupportHud(0.0f,.95f)==CombatHudTarget::None);
    assert(HitCombatSupportHud(-2.0f,.65f)==CombatHudTarget::None);
    assert(HitCombatSupportHud(2.0f,1.25f)==CombatHudTarget::None);
    assert(HitCombatSupportHud(std::numeric_limits<float>::quiet_NaN(),.95f)
           ==CombatHudTarget::None);
    assert(RouteCombatInput(CombatHudTarget::Artillery,true)
           ==CombatInputRoute::Artillery);
    assert(RouteCombatInput(CombatHudTarget::Airstrike,true)
           ==CombatInputRoute::Airstrike);
    assert(RouteCombatInput(CombatHudTarget::Artillery,false)
           ==CombatInputRoute::NoAction);
    assert(RouteCombatInput(CombatHudTarget::Airstrike,false)
           ==CombatInputRoute::NoAction);
    assert(RouteCombatInput(CombatHudTarget::None,false)
           ==CombatInputRoute::Gun);
    assert(RouteCombatInput(CombatHudTarget::None,true)
           ==CombatInputRoute::Gun);

    // UI presses remain captured across aim drift and phase transitions.
    // Releasing Select restores normal held-fire ownership.
    CombatInputLatch latch;
    assert(latch.Route(CombatHudTarget::Artillery,true)
           ==CombatInputRoute::Artillery);
    assert(latch.Route(CombatHudTarget::Artillery,false)
           ==CombatInputRoute::NoAction);
    assert(latch.Route(CombatHudTarget::None,false)
           ==CombatInputRoute::NoAction); // drag off ARTY while held
    assert(latch.Route(CombatHudTarget::Airstrike,false)
           ==CombatInputRoute::NoAction); // cannot activate AIR by dragging
    latch.Release();
    assert(latch.Route(CombatHudTarget::None,true)
           ==CombatInputRoute::Gun);
    assert(latch.Route(CombatHudTarget::None,false)
           ==CombatInputRoute::Gun); // held gunfire still works
    assert(latch.Route(CombatHudTarget::Airstrike,false)
           ==CombatInputRoute::NoAction); // hovering over HUD blocks gun
    latch.Release();
    latch.CaptureUiPress(); // shop START or secondary wave-start with Select held
    assert(latch.Route(CombatHudTarget::None,false)
           ==CombatInputRoute::NoAction); // no accidental first-wave shot
    latch.Release();
    assert(latch.Route(CombatHudTarget::None,true)
           ==CombatInputRoute::Gun);
    latch.Release();
    assert(latch.Route(CombatHudTarget::Airstrike,true)
           ==CombatInputRoute::Airstrike);
    assert(latch.Route(CombatHudTarget::None,false)
           ==CombatInputRoute::NoAction);

    // Procedural hit feedback uses finite positions and a bounded pulse pool.
    ImpactEffects effects;
    const float muzzle[]={0,0,0}, downward[]={0,-.2f,-1};
    assert(effects.RecordHit(muzzle,downward,4.0f,ImpactKind::Metal));
    assert(std::abs(effects.pulses()[0].y+.8f)<.0001f);
    assert(effects.pulses()[0].kind==ImpactKind::Metal);
    assert(effects.RecordWaterMiss(muzzle,downward));
    assert(std::abs(effects.pulses()[1].y+1.10f)<.0001f);
    assert(effects.pulses()[1].kind==ImpactKind::Water);
    const float horizontal[]={0,0,-1}, zeroVector[]={0,0,0};
    assert(!effects.RecordWaterMiss(muzzle,horizontal));
    assert(!effects.RecordHit(muzzle,zeroVector,2,ImpactKind::Sand));
    assert(!effects.RecordHit(muzzle,horizontal,41,ImpactKind::Metal));
    const float nanVector[]={0,std::numeric_limits<float>::quiet_NaN(),-1};
    assert(!effects.RecordHit(muzzle,nanVector,2,ImpactKind::Metal));
    effects.Tick(.15f);
    assert(effects.pulses()[0].remaining>0.14f);
    effects.Tick(.15f);
    assert(effects.pulses()[0].remaining==0.0f);
    for(size_t i=0;i<ImpactEffects::kCapacity+2;i++)
        assert(effects.RecordHit(muzzle,horizontal,static_cast<float>(i+1),
                                 ImpactKind::Sand));
    size_t liveEffects=0;
    for(const ImpactPulse &pulse:effects.pulses())
        if(pulse.remaining>0) ++liveEffects;
    assert(liveEffects==ImpactEffects::kCapacity);

    // Minor impact saturation cannot erase a live explosion.
    ImpactEffects priority;
    assert(priority.RecordHit(muzzle,horizontal,1,ImpactKind::Explosion));
    for(size_t i=0;i<ImpactEffects::kCapacity+6;i++)
        assert(priority.RecordHit(muzzle,horizontal,2,ImpactKind::Sand));
    size_t retained=0;
    for(const auto &pulse:priority.pulses())
        if(pulse.remaining>0 && pulse.kind==ImpactKind::Explosion) ++retained;
    assert(retained==1);
    ImpactEffects allBlasts;
    for(size_t i=0;i<ImpactEffects::kCapacity;i++)
        assert(allBlasts.RecordHit(muzzle,horizontal,1,ImpactKind::Explosion));
    assert(!allBlasts.RecordHit(muzzle,horizontal,2,ImpactKind::Water));
    assert(allBlasts.RecordHit(muzzle,horizontal,2,ImpactKind::AirstrikeBlast));
    allBlasts.Tick(ImpactEffects::kAirstrikeLifetime);
    assert(allBlasts.RecordHit(muzzle,horizontal,2,ImpactKind::Water));

    // A hit spark requires real damage; lethal hits have longer blast pulses.
    // The unupgraded gun consumes ammo against aircraft without dealing damage.
    ImpactEffects blasts;
    assert(!blasts.RecordDamageHit(muzzle,horizontal,5.0f,100,100,true));
    assert(blasts.RecordDamageHit(muzzle,horizontal,5.0f,100,80,true));
    assert(blasts.pulses()[0].kind==ImpactKind::Metal);
    assert(blasts.RecordDamageHit(muzzle,horizontal,5.0f,20,-10,false));
    assert(blasts.pulses()[1].kind==ImpactKind::Explosion);
    assert(std::abs(blasts.pulses()[1].remaining-
                    ImpactEffects::kExplosionLifetime)<.0001f);
    assert(!blasts.RecordDamageHit(muzzle,zeroVector,5.0f,20,0,false));
    assert(!blasts.RecordDamageHit(muzzle,horizontal,41.0f,20,0,false));
    blasts.Tick(ImpactEffects::kLifetime);
    assert(blasts.pulses()[0].remaining==0.0f);
    assert(blasts.pulses()[1].remaining>0.0f);
    blasts.Tick(ImpactEffects::kExplosionLifetime);
    assert(blasts.pulses()[1].remaining==0.0f);

    // Support strikes produce visible blasts only for enemies whose HP
    // actually changed; lethal targets retain their pre-destruction position.
    Campaign artillery_game(Difficulty::Casual,41);
    assert(artillery_game.Start());
    assert(artillery_game.Purchase(Upgrade::Artillery));
    assert(artillery_game.BeginWave());
    artillery_game.Tick(.05f);
    const auto artillery_before=artillery_game.enemies();
    assert(artillery_before[0].active);
    ImpactEffects support_fx;
    assert(artillery_game.CallArtillery());
    assert(support_fx.RecordSupportDamage(artillery_before,
                                        artillery_game.enemies(),false)==1);
    assert(support_fx.pulses()[0].kind==ImpactKind::ArtilleryBlast);
    assert(std::abs(support_fx.pulses()[0].x-artillery_before[0].x)<.0001f);
    assert(std::abs(support_fx.pulses()[0].z-artillery_before[0].z)<.0001f);
    assert(!artillery_game.enemies()[0].active);
    assert(std::abs(support_fx.pulses()[0].remaining-
                    ImpactEffects::kArtilleryLifetime)<.0001f);
    assert(!artillery_game.CallArtillery());
    assert(support_fx.RecordSupportDamage(artillery_game.enemies(),
                                         artillery_game.enemies(),false)==0);
    support_fx.Tick(ImpactEffects::kArtilleryLifetime);
    assert(support_fx.pulses()[0].remaining==0.0f);

    // An actual purchased turret produces a position-accurate shot event,
    // and a killing shot becomes an explosion rather than a generic spark.
    Campaign without_turret(Difficulty::Casual,41);
    assert(without_turret.Start() && without_turret.BeginWave());
    without_turret.Tick(.05f);
    assert(without_turret.turret_shot().sequence==0);

    Campaign turret_game(Difficulty::Casual,41);
    assert(turret_game.Start());
    assert(turret_game.Purchase(Upgrade::SupportTurret));
    assert(turret_game.BeginWave());
    assert(turret_game.turret_shot().sequence==0);
    turret_game.Tick(.05f);
    const TurretShotEvent first_turret_shot=turret_game.turret_shot();
    assert(first_turret_shot.sequence==1 && !first_turret_shot.destroyed);
    assert(turret_game.enemies()[0].active && turret_game.enemies()[0].hp==5);
    assert(std::abs(first_turret_shot.z+11.0f)<.0001f);
    ImpactEffects turret_fx;
    assert(turret_fx.RecordTurretImpact(first_turret_shot.x,
                                        first_turret_shot.z,false));
    assert(turret_fx.pulses()[0].kind==ImpactKind::Metal);
    assert(std::abs(turret_fx.pulses()[0].z-first_turret_shot.z)<.0001f);
    for(int frame=0;frame<35 && turret_game.turret_shot().sequence<2;frame++)
        turret_game.Tick(.05f);
    const TurretShotEvent lethal_turret_shot=turret_game.turret_shot();
    assert(lethal_turret_shot.sequence==2 && lethal_turret_shot.destroyed);
    assert(!turret_game.enemies()[0].active);
    assert(turret_game.wave_status().boats_sunk==1);
    assert(turret_fx.RecordTurretImpact(lethal_turret_shot.x,
                                        lethal_turret_shot.z,true));
    assert(turret_fx.pulses()[1].kind==ImpactKind::Explosion);
    assert(std::abs(turret_fx.pulses()[1].remaining-
                    ImpactEffects::kExplosionLifetime)<.0001f);
    assert(!turret_fx.RecordTurretImpact(
        std::numeric_limits<float>::quiet_NaN(),-11.0f,false));

    // Surface intersection ordering must beat center-distance ordering.
    // The farther center's larger hull can be the first struck surface.
    const float rayOrigin[]={0,0,5}, rayForward[]={0,0,-1};
    const float smallNear[]={0,0,2}, largeFar[]={0,0,1};
    float smallDepth=0, largeDepth=0;
    assert(RaySphereSurfaceDistance(rayOrigin,rayForward,smallNear,.2f,&smallDepth));
    assert(RaySphereSurfaceDistance(rayOrigin,rayForward,largeFar,1.4f,&largeDepth));
    assert(largeDepth<smallDepth);
    const float center[]={0,0,0}, tangent[]={1,0,0};
    float hit=0;
    assert(RaySphereSurfaceDistance(rayOrigin,rayForward,center,1,&hit));
    assert(std::abs(hit-4.0f)<0.0001f);
    assert(RaySphereSurfaceDistance(rayOrigin,rayForward,tangent,1,&hit));
    assert(std::abs(hit-5.0f)<0.0001f);
    const float inside[]={0,0,0}, exitRay[]={0,0,1};
    assert(RaySphereSurfaceDistance(inside,exitRay,center,1,&hit));
    assert(std::abs(hit-1.0f)<0.0001f);
    const float scaledRay[]={0,0,-2};
    assert(RaySphereSurfaceDistance(rayOrigin,scaledRay,center,1,&hit));
    assert(std::abs(hit-2.0f)<0.0001f);
    const float behind[]={0,0,10}, zero[]={0,0,0};
    const float invalid[]={std::numeric_limits<float>::quiet_NaN(),0,5};
    assert(!RaySphereSurfaceDistance(rayOrigin,rayForward,behind,1,&hit));
    assert(!RaySphereSurfaceDistance(rayOrigin,zero,center,1,&hit));
    assert(!RaySphereSurfaceDistance(invalid,rayForward,center,1,&hit));
    assert(!RaySphereSurfaceDistance(rayOrigin,rayForward,center,-1,&hit));
    assert(!RaySphereSurfaceDistance(rayOrigin,rayForward,center,1,nullptr));

    Campaign c(Difficulty::Casual, 41);
    assert(c.Start() && c.phase() == Phase::Shop);
    assert(c.waves_this_period() == 2);
    assert(c.Purchase(Upgrade::FireRate));
    assert(c.stats().fire_delay_ms == 265);
    assert(c.Purchase(Upgrade::Magazine));
    assert(c.stats().magazine == 16);
    assert(c.BeginWave());
    for (int i=0;i<50;i++) c.Tick(0.05f);
    bool saw_rowboat=false;
    for (const Enemy &e:c.enemies()) {
        if (e.active) {
            assert(e.type == UnitType::Rowboat && e.passengers == 1);
            saw_rowboat=true;
        }
    }
    assert(saw_rowboat);
    assert(c.incoming_threats().aircraft_seconds<0.0f);
    assert(c.incoming_threats().gunboat_seconds<0.0f);
    // The SDK permits a 0.10-second frame delta. One slow frame must
    // simulate the same world as two 0.05-second frames, without letting
    // an arbitrarily long resume pause skip an entire battle.
    Campaign slow_frame(Difficulty::Casual,41);
    Campaign split_frame(Difficulty::Casual,41);
    Campaign long_pause(Difficulty::Casual,41);
    for(Campaign *game : {&slow_frame,&split_frame,&long_pause}) {
        assert(game->Start() && game->BeginWave());
    }
    for(int frame=0;frame<81;frame++) {
        slow_frame.Tick(0.10f);
        split_frame.Tick(0.05f);
        split_frame.Tick(0.05f);
        long_pause.Tick(5.0f);
        assert(slow_frame.phase()==split_frame.phase());
        assert(slow_frame.integrity()==split_frame.integrity());
        assert(slow_frame.infantry()==split_frame.infantry());
        assert(slow_frame.wave_status().boats_sunk==split_frame.wave_status().boats_sunk);
        for(size_t i=0;i<Campaign::kMaxEnemies;i++) {
            const Enemy &a=slow_frame.enemies()[i], &b=split_frame.enemies()[i];
            const Enemy &c=long_pause.enemies()[i];
            assert(a.active==b.active && a.active==c.active);
            if(a.active) {
                assert(a.type==b.type && a.hp==b.hp);
                assert(a.z==b.z && a.z==c.z);
            }
        }
    }
    assert(slow_frame.enemies()[0].active);
    assert(slow_frame.enemies()[0].z>-11.0f);
    Campaign sunk(Difficulty::Casual,41);
    assert(sunk.Start() && sunk.BeginWave());
    for (int i=0;i<180;i++) {
        sunk.Tick(0.05f);
        for (uint8_t n=0;n<sunk.enemies().size();++n) {
            if (sunk.enemies()[n].active) sunk.FireAt(n);
        }
        if (sunk.phase()!=Phase::Combat) break;
    }
    assert(sunk.wave_status().boats_sunk>=1);
    assert(sunk.wave_status().troops_landed==0);
    assert(sunk.infantry()==0);

    // Trigger pulls are physical actions, not free hit-probes: misses spend
    // ammunition, respect fire cadence and enter the same timed reload cycle.
    Campaign firing(Difficulty::Casual,19);
    assert(firing.Start() && firing.BeginWave());
    const uint8_t full_mag=firing.magazine_left();
    assert(full_mag==firing.stats().magazine);
    assert(!firing.reloading());
    assert(firing.reload_fraction()==0.0f);
    assert(firing.FireMiss());
    assert(firing.magazine_left()==full_mag-1);
    assert(!firing.reloading());
    assert(firing.reload_fraction()==0.0f);
    assert(!firing.FireMiss());
    while(firing.magazine_left()>0) {
        for(int i=0;i<7;i++) firing.Tick(0.05f);
        assert(firing.FireMiss());
    }
    assert(firing.reloading());
    assert(firing.reload_fraction()==0.0f);
    assert(!firing.FireMiss());
    firing.Tick(0.05f);
    const float early_progress=firing.reload_fraction();
    assert(early_progress>0.0f && early_progress<1.0f);
    assert(firing.magazine_left()==0);
    assert(!firing.FireMiss());
    for(int i=0;i<9;i++) firing.Tick(0.05f);
    assert(firing.reloading());
    assert(firing.reload_fraction()>early_progress);
    assert(firing.reload_fraction()<1.0f);
    assert(!firing.FireMiss());
    for(int i=0;i<14;i++) firing.Tick(0.05f);
    assert(!firing.reloading());
    assert(firing.reload_fraction()==0.0f);
    assert(firing.magazine_left()==firing.stats().magazine);
    assert(firing.FireMiss());

    // The combat secondary action allows a tactical partial-magazine reload,
    // but never an instant refill, a second concurrent reload, or a full-mag
    // exploit. The existing timed reload path remains authoritative.
    Campaign tactical(Difficulty::Casual,19);
    assert(!tactical.RequestReload()); // briefing
    assert(tactical.Start());
    assert(!tactical.RequestReload()); // shop
    assert(tactical.BeginWave());
    const uint8_t tactical_full=tactical.magazine_left();
    assert(!tactical.RequestReload()); // full magazine
    assert(tactical.FireMiss());
    assert(tactical.magazine_left()==tactical_full-1);
    assert(tactical.RequestReload());
    assert(tactical.reloading());
    assert(tactical.magazine_left()==tactical_full-1);
    assert(!tactical.RequestReload());
    assert(!tactical.FireMiss());
    tactical.Tick(.05f);
    assert(tactical.reload_fraction()>0.0f);
    for(int i=0;i<24;i++) tactical.Tick(.05f);
    assert(!tactical.reloading());
    assert(tactical.magazine_left()==tactical_full);
    assert(!tactical.RequestReload());
    assert(tactical.FireMiss());

    Campaign landed(Difficulty::Casual,41);
    assert(landed.Start() && landed.BeginWave());
    for(int i=0;i<4500 && landed.wave_status().troops_landed==0;i++)
        landed.Tick(0.05f);
    assert(landed.wave_status().troops_landed>0);
    assert(landed.infantry()>0);
    // Disembarking is not an instant firing position: troops must cross the
    // exposed beach, then complete a full four-second attack countdown.
    const int32_t integrity=landed.integrity();
    for(int i=0;i<120;i++) landed.Tick(0.05f); // six seconds after first landing
    assert(landed.integrity()==integrity);
    for(const Infantry &trooper:landed.infantry_units())
        if(trooper.active) assert(trooper.z < -1.34f);
    for(int i=0;i<160;i++) landed.Tick(0.05f);
    assert(landed.integrity()<integrity);
    while(landed.infantry()) {
        landed.Tick(0.05f);
        for(uint8_t i=0;i<landed.infantry_units().size();++i)
            if(landed.infantry_units()[i].active && landed.FireAtInfantry(i)) break;
    }
    assert(landed.wave_status().troops_stopped>=1);
    assert(landed.waves_this_period()==2);

    // The campaign allocates independently moving soldiers with real targets.
    Campaign beach(Difficulty::Casual,41);
    assert(beach.Start() && beach.BeginWave());
    for(int i=0;i<360 && beach.infantry()==0;i++) beach.Tick(0.05f);
    assert(beach.infantry()>0);
    uint8_t first=255, active=0;
    for(uint8_t i=0;i<beach.infantry_units().size();i++) {
        if(!beach.infantry_units()[i].active) continue;
        if(first==255) first=i;
        ++active;
    }
    assert(active==beach.infantry() && first!=255);
    const Infantry soldier=beach.infantry_units()[first];
    const float origin[]={soldier.x,-0.66f,soldier.z+1.0f};
    const float missOrigin[]={soldier.x+0.6f,-0.66f,soldier.z+1.0f};
    const float ray[]={0,0,-1};
    float hitDepth=0;
    assert(RayHitsInfantry(soldier,origin,ray,&hitDepth));
    assert(hitDepth>0.69f && hitDepth<0.71f);
    assert(!RayHitsInfantry(soldier,missOrigin,ray,&hitDepth));
    assert(!RayHitsInfantry(soldier,origin,nullptr,&hitDepth));
    for(int i=0;i<10;i++) beach.Tick(0.05f);
    assert(beach.infantry_units()[first].z>soldier.z);
    const uint8_t ammoBefore=beach.magazine_left();
    assert(!beach.FireAtInfantry(static_cast<uint8_t>(Campaign::kMaxInfantry-1)));
    assert(beach.magazine_left()==ammoBefore);
    assert(beach.FireAtInfantry(first));
    assert(!beach.infantry_units()[first].active);
    assert(beach.infantry()==active-1);
    assert(beach.magazine_left()==ammoBefore-1);
    assert(!beach.FireAtInfantry(first));
    assert(beach.magazine_left()==ammoBefore-1);

    // Higher difficulties add more boats and waves, never skip the
    // opening single-passenger rowboat introduction.
    for(Difficulty d : {Difficulty::Casual, Difficulty::Regular,
                        Difficulty::Veteran, Difficulty::Siege}) {
        Campaign opening(d,17);
        assert(opening.Start() && opening.BeginWave());
        opening.Tick(0.05f);
        assert(opening.enemies()[0].active);
        assert(opening.enemies()[0].type==UnitType::Rowboat);
        assert(opening.enemies()[0].passengers==1);
    }

    Campaign veteran(Difficulty::Siege,7);
    assert(veteran.Start());
    assert(veteran.waves_this_period()==5);
    assert(veteran.price(Upgrade::AntiAir)>0);
    assert(!veteran.FireAt(0));
    assert(veteran.Purchase(Upgrade::AntiAir));
    assert(veteran.stats().anti_air);
    assert(!veteran.Purchase(Upgrade::AntiAir));
    assert(veteran.Purchase(Upgrade::Artillery));
    assert(veteran.Purchase(Upgrade::Airstrike)==false); // insufficient credits
    assert(veteran.BeginWave());
    assert(!veteran.CallArtillery()); // no vessel spawned yet
    assert(veteran.artillery_charges()==1);
    veteran.Tick(0.05f);
    assert(veteran.enemies()[0].active);
    assert(veteran.CallArtillery());
    assert(veteran.artillery_charges()==0);
    assert(!veteran.CallArtillery());
    assert(!veteran.Purchase(Upgrade::Damage));

    // Support charges are never consumed by a call with no eligible target.
    Campaign empty_support(Difficulty::Casual,41);
    assert(empty_support.Start());
    assert(empty_support.Purchase(Upgrade::Artillery));
    assert(empty_support.Purchase(Upgrade::Airstrike));
    assert(!empty_support.CallArtillery()); // shop phase
    assert(!empty_support.CallAirstrike());
    assert(empty_support.BeginWave());
    assert(!empty_support.CallArtillery()); // no active enemies
    assert(!empty_support.CallAirstrike());
    assert(empty_support.artillery_charges()==1);
    assert(empty_support.airstrike_charges()==1);
    empty_support.Tick(0.05f);
    assert(empty_support.enemies()[0].active);
    assert(empty_support.CallArtillery());
    assert(empty_support.artillery_charges()==0);
    assert(!empty_support.CallAirstrike()); // artillery cleared the only target
    assert(empty_support.airstrike_charges()==1);
    for(int i=0;i<33;i++) empty_support.Tick(0.05f);
    bool next_target=false;
    for(const Enemy &e:empty_support.enemies()) if(e.active) next_target=true;
    assert(next_target);
    assert(empty_support.CallAirstrike());
    assert(empty_support.airstrike_charges()==0);
    assert(!empty_support.CallAirstrike());

    Campaign turret(Difficulty::Casual,41);
    assert(turret.Start());
    assert(turret.Purchase(Upgrade::SupportTurret));
    assert(turret.stats().turret);
    assert(turret.BeginWave());
    for(int i=0;i<160;i++) turret.Tick(0.05f);
    assert(turret.wave_status().boats_sunk>=1);
    assert(turret.wave_status().troops_landed==0);

    // Snapshot only stable shop state. Corrupt/mismatched snapshots must
    // leave the live campaign untouched, including purchased upgrades.
    Campaign writer(Difficulty::Veteran,1234);
    assert(writer.Start());
    assert(writer.Purchase(Upgrade::Damage));
    assert(writer.Purchase(Upgrade::Artillery));
    uint8_t encoded[Campaign::kSerializedSize]={};
    assert(writer.SerializeShop(encoded,sizeof(encoded)));
    Campaign restored;
    assert(restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.phase()==Phase::Shop);
    assert(restored.credits()==writer.credits());
    assert(restored.level(Upgrade::Damage)==1);
    assert(restored.artillery_charges()==1);
    assert(restored.waves_this_period()==4);
    const int credit_before=restored.credits();
    encoded[0]^=0xffu;
    assert(!restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.credits()==credit_before);
    encoded[0]^=0xffu;
    encoded[28]=0xffu; // invalid wave index, at word 7
    assert(!restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.credits()==credit_before);
    assert(writer.BeginWave());
    assert(!writer.SerializeShop(encoded,sizeof(encoded)));

    // A location-stage fixture proves the military landing craft is an
    // actual transport containing 3–6 soldiers, not a decorative rowboat.
    Campaign stage(Difficulty::Casual,6);
    assert(stage.Start());
    uint8_t stage_bytes[Campaign::kSerializedSize]={};
    assert(stage.SerializeShop(stage_bytes,sizeof(stage_bytes)));
    stage_bytes[16]=1; // advance location from Shingle Bay to Iron Quay
    Campaign landing_stage;
    assert(landing_stage.DeserializeShop(stage_bytes,sizeof(stage_bytes)));
    assert(landing_stage.location()==1);
    assert(landing_stage.BeginWave());
    landing_stage.Tick(0.05f);
    const Enemy first_craft=landing_stage.enemies()[0];
    assert(first_craft.active && first_craft.type==UnitType::LandingCraft);
    assert(first_craft.passengers>=3 && first_craft.passengers<=6);
    for(int i=0;i<35;i++) {
        landing_stage.FireAt(0);
        landing_stage.Tick(0.05f);
    }
    assert(landing_stage.wave_status().boats_sunk>=1);
    assert(landing_stage.wave_status().troops_landed==0);


    // Iron Quay day two seed 8 starts with a gunboat. It must be able to
    // fire from offshore, and early interception must prevent those salvos.
    auto prepare_gunboat=[](Campaign &game) {
        uint8_t bytes[Campaign::kSerializedSize]={};
        assert(game.SerializeShop(bytes,sizeof(bytes)));
        bytes[16]=1;
        bytes[20]=2;
        assert(game.DeserializeShop(bytes,sizeof(bytes)));
        assert(game.BeginWave());
        game.Tick(0.05f);
        assert(game.enemies()[0].type==UnitType::Gunboat);
    };
    Campaign firing_ship(Difficulty::Casual,8);
    assert(firing_ship.Start());
    prepare_gunboat(firing_ship);
    const IncomingThreats approaching_gunboat=firing_ship.incoming_threats();
    assert(approaching_gunboat.aircraft_seconds<0.0f);
    assert(approaching_gunboat.gunboat_seconds>3.0f &&
           approaching_gunboat.gunboat_seconds<5.0f);
    for(int i=0;i<70;i++) firing_ship.Tick(0.05f);
    assert(firing_ship.wave_status().gunboat_attacks==0);
    assert(firing_ship.integrity()==100);
    for(int i=0;i<25 && firing_ship.wave_status().gunboat_attacks==0;i++)
        firing_ship.Tick(0.05f);
    assert(firing_ship.wave_status().gunboat_attacks==1);
    assert(firing_ship.integrity()==97);
    for(int i=0;i<65 && firing_ship.wave_status().gunboat_attacks<2;i++)
        firing_ship.Tick(0.05f);
    assert(firing_ship.wave_status().gunboat_attacks==2);
    assert(firing_ship.integrity()==94);

    Campaign stopped_ship(Difficulty::Casual,8);
    assert(stopped_ship.Start());
    prepare_gunboat(stopped_ship);
    for(int shot=0;shot<7;shot++) {
        if(shot>0) for(int i=0;i<7;i++) stopped_ship.Tick(0.05f);
        assert(stopped_ship.FireAt(0));
    }
    assert(!stopped_ship.enemies()[0].active);
    assert(stopped_ship.incoming_threats().gunboat_seconds<0.0f);
    for(int i=0;i<140;i++) stopped_ship.Tick(0.05f);
    assert(stopped_ship.wave_status().gunboat_attacks==0);
    assert(stopped_ship.integrity()==100);

    // Offshore gunboat salvos also retain their timing across 10 FPS frames.
    Campaign slow_gunboat(Difficulty::Casual,8), split_gunboat(Difficulty::Casual,8);
    assert(slow_gunboat.Start() && split_gunboat.Start());
    prepare_gunboat(slow_gunboat);
    prepare_gunboat(split_gunboat);
    for(int frame=0;frame<100;frame++) {
        slow_gunboat.Tick(0.10f);
        split_gunboat.Tick(0.05f);
        split_gunboat.Tick(0.05f);
    }
    assert(slow_gunboat.wave_status().gunboat_attacks>=1);
    assert(slow_gunboat.wave_status().gunboat_attacks==
           split_gunboat.wave_status().gunboat_attacks);
    assert(slow_gunboat.integrity()==split_gunboat.integrity());
    assert(slow_gunboat.enemies()[0].z==split_gunboat.enemies()[0].z);

    auto stage_blackcap_night=[](Campaign &game) {
        uint8_t bytes[Campaign::kSerializedSize]={};
        assert(game.SerializeShop(bytes,sizeof(bytes)));
        bytes[16]=2; // Blackcap Island
        bytes[20]=4; // final day
        bytes[24]=1; // night
        bytes[28]=1; // first wave in period
        assert(game.DeserializeShop(bytes,sizeof(bytes)));
    };

    // An aircraft-only wave is ineligible for artillery, but air support
    // may destroy it without purchasing the ordinary anti-air upgrade.
    Campaign air_only(Difficulty::Casual,8);
    assert(air_only.Start());
    assert(air_only.Purchase(Upgrade::Artillery));
    assert(air_only.Purchase(Upgrade::Airstrike));
    stage_blackcap_night(air_only);
    assert(air_only.BeginWave());
    air_only.Tick(0.05f);
    assert(air_only.enemies()[0].type==UnitType::Aircraft);
    const auto aircraft_before=air_only.enemies();
    ImpactEffects air_fx;
    assert(!air_only.CallArtillery());
    assert(air_only.artillery_charges()==1);
    assert(air_fx.RecordSupportDamage(aircraft_before,air_only.enemies(),false)==0);
    assert(air_only.CallAirstrike());
    assert(air_fx.RecordSupportDamage(aircraft_before,air_only.enemies(),true)==1);
    assert(air_fx.pulses()[0].kind==ImpactKind::AirstrikeBlast);
    assert(std::abs(air_fx.pulses()[0].y-1.6f)<.0001f);
    assert(std::abs(air_fx.pulses()[0].z-aircraft_before[0].z)<.0001f);
    assert(std::abs(air_fx.pulses()[0].remaining-
                    ImpactEffects::kAirstrikeLifetime)<.0001f);
    assert(air_only.airstrike_charges()==0);
    assert(air_only.wave_status().aircraft_destroyed==1);

    // Seed 8 deterministically opens late Blackcap with a bomber. Without AA
    // the gun still spends its round, but cannot damage the aircraft.
    Campaign bomber(Difficulty::Casual,8);
    assert(bomber.Start());
    stage_blackcap_night(bomber);
    assert(bomber.BeginWave());
    bomber.Tick(0.05f);
    assert(bomber.enemies()[0].active);
    assert(bomber.enemies()[0].type==UnitType::Aircraft);
    assert(bomber.enemies()[0].aircraft_pattern==AircraftPattern::Bomber);
    const IncomingThreats approaching_bomber=bomber.incoming_threats();
    assert(approaching_bomber.aircraft_seconds>4.0f &&
           approaching_bomber.aircraft_seconds<6.0f);
    assert(approaching_bomber.gunboat_seconds<0.0f);
    const int16_t bomber_hp=bomber.enemies()[0].hp;
    const uint8_t bomber_mag=bomber.magazine_left();
    assert(bomber.FireAt(0));
    assert(bomber.magazine_left()==bomber_mag-1);
    assert(bomber.enemies()[0].hp==bomber_hp);
    for(int i=0;i<140 && bomber.wave_status().aircraft_attacks==0;i++)
        bomber.Tick(0.05f);
    assert(bomber.wave_status().aircraft_attacks==1);
    assert(bomber.incoming_threats().aircraft_seconds<0.0f);
    assert(bomber.integrity()==88);

    // Seed 22 opens the same stage with a strafer that peppers the position
    // repeatedly during one overhead pass instead of behaving like a sky-boat.
    Campaign strafer(Difficulty::Casual,22);
    assert(strafer.Start());
    stage_blackcap_night(strafer);
    assert(strafer.BeginWave());
    strafer.Tick(0.05f);
    assert(strafer.enemies()[0].active);
    assert(strafer.enemies()[0].aircraft_pattern==AircraftPattern::Strafer);
    for(int i=0;i<110 && strafer.wave_status().aircraft_attacks<4;i++)
        strafer.Tick(0.05f);
    assert(strafer.wave_status().aircraft_attacks==4);
    assert(strafer.integrity()==88);

    // Purchasing AA turns those otherwise ineffective shots into damage and
    // can destroy the bomber before its attack window begins.
    Campaign defended(Difficulty::Casual,8);
    assert(defended.Start());
    assert(defended.Purchase(Upgrade::AntiAir));
    stage_blackcap_night(defended);
    assert(defended.BeginWave());
    defended.Tick(0.05f);
    assert(defended.enemies()[0].type==UnitType::Aircraft);
    for(int shot=0;shot<5;shot++) {
        if(shot>0) for(int i=0;i<7;i++) defended.Tick(0.05f);
        assert(defended.FireAt(0));
    }
    assert(!defended.enemies()[0].active);
    assert(defended.incoming_threats().aircraft_seconds<0.0f);
    assert(defended.wave_status().aircraft_destroyed==1);
    assert(defended.wave_status().aircraft_attacks==0);
    assert(defended.integrity()==100);

    // Day and night each contain their own shop-separated waves.
    Campaign clock(Difficulty::Casual,41);
    assert(clock.Start());
    for(int i=0;i<1800 && clock.period()!=Period::Night;i++) {
        if(clock.phase()==Phase::Shop) assert(clock.BeginWave());
        clock.Tick(0.05f);
        for(uint8_t n=0;n<clock.enemies().size();n++)
            if(clock.enemies()[n].active) clock.FireAt(n);
        if(clock.infantry()>0)
            for(uint8_t i=0;i<clock.infantry_units().size();++i)
                if(clock.infantry_units()[i].active && clock.FireAtInfantry(i)) break;
    }
    assert(clock.period()==Period::Night && clock.day()==1);
    assert(clock.wave_index()==1);
    assert(clock.waves_this_period()==1);

    assert(Campaign::Locations()[0].days==2);
    assert(Campaign::Locations()[1].days==3);
    assert(Campaign::Locations()[2].days==4);
    std::cout << "Breakwater boat / landing / upgrade smoke passed\n";
}
