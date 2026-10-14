#include "breakwater_campaign.h"
#include "breakwater_silhouettes.h"
#include "breakwater_terrain.h"
#include "breakwater_damage_feedback.h"
#include "breakwater_aircraft_feedback.h"
#include "breakwater_combat_input.h"
#include "breakwater_impacts.h"
#include "breakwater_weapons.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>
using namespace reverie::breakwater;

int main() {
    // The three battlefields render real shore, not an endless sea under
    // walking infantry. Validate budget, location identity and bounds.
    for (uint8_t location=0;location<Campaign::kLocations;location++) {
        int boxes=0, triangles=0;
        auto box=[&](float x,float y,float z,float w,float h,float d,
                     float r,float g,float b) {
            ++boxes;
            assert(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z));
            assert(w>0&&h>0&&d>0);
            assert(std::abs(x)<4.0f&&z>=-2.25f&&z<=-.80f);
            assert(r>=0&&g>=0&&b>=0&&r<=1&&g<=1&&b<=1);
        };
        auto triangle=[&](float x0,float y0,float z0,
                          float x1,float y1,float z1,
                          float x2,float y2,float z2,
                          float r,float g,float b) {
            ++triangles;
            assert(std::isfinite(x0)&&std::isfinite(y0)&&std::isfinite(z0));
            assert(std::isfinite(x1)&&std::isfinite(y1)&&std::isfinite(z1));
            assert(std::isfinite(x2)&&std::isfinite(y2)&&std::isfinite(z2));
            assert(x0>=-4.0f&&x0<=4.0f);
            assert(x1>=-4.0f&&x1<=4.0f);
            assert(x2>=-4.0f&&x2<=4.0f);
            assert(y0>=-1.10f&&y1>=-1.10f&&y2>=-1.10f);
            assert(z0>=-2.25f&&z0<=-.80f);
            assert(z1>=-2.25f&&z1<=-.80f);
            assert(z2>=-2.25f&&z2<=-.80f);
            assert(r>=0&&g>=0&&b>=0&&r<=1&&g<=1&&b<=1);
        };
        EmitCoastalTerrain(location,false,box,triangle);
        const int expectedTriangles[]={38,25,28};
        const int expectedBoxes[]={2,8,8};
        assert(triangles==expectedTriangles[location]);
        assert(boxes==expectedBoxes[location]);
        const CoastPalette day=CoastColors(location,false);
        const CoastPalette night=CoastColors(location,true);
        assert(day.sand_r>night.sand_r);
        assert(day.sand_g>night.sand_g);
        assert(day.sand_b>night.sand_b);
        boxes=triangles=0;
        EmitCoastalTerrain(location,true,box,triangle);
        assert(triangles==expectedTriangles[location]);
        assert(boxes==expectedBoxes[location]);
    }
    {
        int boxes=0,triangles=0;
        EmitCoastalTerrain(Campaign::kLocations,false,
            [&](auto...){++boxes;},[&](auto...){++triangles;});
        assert(boxes==0&&triangles==0);
    }

    // Inspect the exact low-poly vessel recipes used by the headset mesh.
    const auto vesselShape = [](UnitType kind, uint8_t passengers, bool active) {
        Enemy craft{};
        craft.type=kind;craft.passengers=passengers;
        craft.active=active;craft.x=.6f;craft.z=-5.0f;
        int boxes=0, triangles=0;
        EmitVesselSilhouette(craft,
            [&](float x,float y,float z,float w,float h,float d,
                float r,float g,float b) {
                ++boxes;
                assert(w>0.0f&&h>0.0f&&d>0.0f);
                assert(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z));
                assert(std::isfinite(r)&&std::isfinite(g)&&std::isfinite(b));
            },
            [&](float x0,float y0,float z0,float x1,float y1,float z1,
                float x2,float y2,float z2,float r,float g,float b) {
                ++triangles;
                assert(std::isfinite(x0)&&std::isfinite(y0)&&std::isfinite(z0));
                assert(std::isfinite(x1)&&std::isfinite(y1)&&std::isfinite(z1));
                assert(std::isfinite(x2)&&std::isfinite(y2)&&std::isfinite(z2));
                assert(std::isfinite(r)&&std::isfinite(g)&&std::isfinite(b));
            });
        return std::array<int,2>{{boxes,triangles}};
    };
    assert((vesselShape(UnitType::Rowboat,1,true)==std::array<int,2>{{5,4}}));
    assert((vesselShape(UnitType::LandingCraft,3,true)==std::array<int,2>{{10,4}}));
    assert((vesselShape(UnitType::LandingCraft,6,true)==std::array<int,2>{{16,4}}));
    assert((vesselShape(UnitType::LandingCraft,255,true)==std::array<int,2>{{16,4}}));
    assert((vesselShape(UnitType::Gunboat,0,true)==std::array<int,2>{{7,4}}));
    assert((vesselShape(UnitType::Aircraft,0,true)==std::array<int,2>{{0,0}}));
    assert((vesselShape(UnitType::Rowboat,1,false)==std::array<int,2>{{0,0}}));
    const Infantry activeInfantry{.3f,-2.0f,true};
    int soldierBoxes=0,soldierTriangles=0;
    EmitInfantrySilhouette(activeInfantry,
        [&](float x,float y,float z,float w,float h,float d,
            float, float, float) {
            ++soldierBoxes;
            assert(std::abs(x-activeInfantry.x)<.15f);
            assert(std::abs(z-activeInfantry.z)<.15f);
            assert(w<=.20f&&h<=.36f&&d<=.16f&&y>-.94f);
        },
        [&](float x0,float y0,float z0,float x1,float y1,float z1,
            float x2,float y2,float z2,float, float, float) {
            ++soldierTriangles;
            assert(std::isfinite(x0)&&std::isfinite(y0)&&std::isfinite(z0));
            assert(std::isfinite(x1)&&std::isfinite(y1)&&std::isfinite(z1));
            assert(std::isfinite(x2)&&std::isfinite(y2)&&std::isfinite(z2));
            assert(std::abs(x0-activeInfantry.x)<.15f);
            assert(std::abs(x1-activeInfantry.x)<.15f);
            assert(std::abs(x2-activeInfantry.x)<.15f);
        });
    assert(soldierBoxes==2&&soldierTriangles==5);
    int inactiveGeometry=0;
    EmitInfantrySilhouette(Infantry{},
        [&](auto...){++inactiveGeometry;},[&](auto...){++inactiveGeometry;});
    assert(inactiveGeometry==0);

    // The animation poses are spatially driven, deterministic, and never
    // advance state separately when the right eye renders after the left.
    const float invalidPose=std::numeric_limits<float>::quiet_NaN();
    assert(LandingRampProgress(-11.0f)==0.0f);
    assert(LandingRampProgress(-3.7f)<.0001f);
    assert(LandingRampProgress(-2.2f)>.9999f);
    assert(LandingRampProgress(invalidPose)==0.0f);
    assert(VesselWakePhase(-11.0f)==0.0f);
    assert(VesselWakePhase(-10.75f)==.5f);
    assert(VesselWakePhase(-10.5f)==0.0f);
    assert(VesselWakePhase(invalidPose)==0.0f);
    for (int i=0;i<=150;i++) {
        const float z=-3.7f+i*.01f;
        const float progress=LandingRampProgress(z);
        assert(progress>=0.0f && progress<=1.0f);
        if(i) assert(progress>=LandingRampProgress(z-.01f));
    }
    Enemy rampCraft{};
    rampCraft.active=true;
    rampCraft.type=UnitType::LandingCraft;
    rampCraft.passengers=4;
    rampCraft.z=-4.0f;
    const auto rampTip = [](const Enemy &craft) {
        float tip=100.0f;
        int triangleCount=0;
        EmitVesselSilhouette(craft,[](auto...){},
            [&](float,float,float,float,float,float,
                float,float y2,float,float,float,float) {
                if (triangleCount++==0) tip=y2;
            });
        assert(triangleCount==4);
        return tip;
    };
    const float upright=rampTip(rampCraft);
    rampCraft.z=-2.2f;
    const float lowered=rampTip(rampCraft);
    assert(upright>-.47f && lowered< -1.02f);
    assert(rampTip(rampCraft)==lowered);
    const auto movingWake = [](const Enemy &craft) {
        float zRelative=0.0f;
        int triangles=0;
        EmitVesselSilhouette(craft,[](auto...){},
            [&](float,float,float,float,float,float z1,
                float,float,float,float,float,float) {
                if(triangles++==2)zRelative=z1-craft.z;
            });
        assert(triangles==4);
        return zRelative;
    };
    rampCraft.type=UnitType::Rowboat;
    rampCraft.passengers=1;
    rampCraft.z=-10.75f;
    const float wakeA=movingWake(rampCraft);
    rampCraft.z=-10.5f;
    assert(std::abs(wakeA-movingWake(rampCraft))>.03f);
    Infantry marching{.3f,-2.2f,true};
    const auto footPosition = [](const Infantry &trooper) {
        float relativeZ=0.0f;
        int triangles=0;
        EmitInfantrySilhouette(trooper,[](auto...){},
            [&](float,float,float,float,float,float z1,
                float,float,float,float,float,float) {
                if(triangles++==2)relativeZ=z1-trooper.z;
            });
        assert(triangles==5);
        return relativeZ;
    };
    const float firstStride=InfantryMarchStride(marching);
    const float firstFoot=footPosition(marching);
    marching.z=-2.08f;
    assert(InfantryMarchStride(marching)*firstStride<0.0f);
    assert(std::abs(footPosition(marching)-firstFoot)>.05f);
    marching.z=-1.34f;
    assert(InfantryMarchStride(marching)==0.0f);
    marching.active=false;
    assert(InfantryMarchStride(marching)==0.0f);
    marching.active=true;
    marching.z=invalidPose;
    assert(InfantryMarchStride(marching)==0.0f);

    // Ballistic trajectory, ray intersection and ghost aim are pure C++.
    // In ordinary MG mode, pointing a straight ray at a far vessel must
    // miss low; raising it compensates for the simulated trajectory drop.
    const float gun_origin[]={0.0f,-.70f,0.0f};
    const float direct[]={0.0f,0.0f,-1.0f};
    const float ship[]={0.0f,-.70f,-9.0f};
    const float flat_gravity=WeaponFor(WeaponKind::MachineGun).gravity;
    assert(flat_gravity>0.0f);
    float distance=0.0f,point[3]={};
    assert(ProjectilePosition(gun_origin,direct,9.0f,flat_gravity,point));
    assert(std::abs(point[1]+.70f+flat_gravity*81.0f)<.00001f);
    assert(!ProjectileHitsSphere(gun_origin,direct,flat_gravity,ship,
                                 .65f,&distance));
    const float raised[]={0.0f,.125f,-1.0f};
    assert(ProjectileHitsSphere(gun_origin,raised,flat_gravity,ship,
                                .65f,&distance));
    assert(distance>8.0f&&distance<10.0f);
    assert(ProjectileHitsSphere(gun_origin,direct,0.0f,ship,
                                .65f,&distance));
    assert(!ProjectileHitsSphere(gun_origin,direct,flat_gravity,ship,
                                -1.0f,&distance));
    assert(!ProjectileHitsSphere(gun_origin,direct,-.1f,ship,
                                .65f,&distance));
    const float high[]={0.0f,2.0f,-.4f};
    assert(!WeaponWithinTraverse(high,WeaponKind::DirectCannon));
    assert(!WeaponWithinTraverse(high,WeaponKind::MachineGun));
    assert(WeaponWithinTraverse(raised,WeaponKind::DirectCannon));
    const float modest[]={0.0f,.75f,-1.0f};
    assert(WeaponWithinTraverse(modest,WeaponKind::Flak));
    assert(!WeaponWithinTraverse(modest,WeaponKind::DirectCannon));
    assert(WeaponFor(WeaponKind::MissileBattery).uses_ghost);
    assert(WeaponFor(WeaponKind::Flak).air_capable);
    float ghost[3]={};
    assert(GhostAimPoint(gun_origin,high,ghost));
    assert(ghost[1]>gun_origin[1]+4.0f);
    const float bad[]={0.0f,std::numeric_limits<float>::quiet_NaN(),-1.0f};
    assert(!GhostAimPoint(gun_origin,bad,ghost));

    // Support labels consume held Select after the initial action edge.
    // They must not fire the gun through the HUD or spend ammunition.
    assert(HitCombatSupportHud(-2.0f,.95f)==CombatHudTarget::Artillery);
    assert(HitCombatSupportHud(2.0f,.95f)==CombatHudTarget::Airstrike);
    assert(HitCombatSupportHud(0.0f,.95f)==CombatHudTarget::Sam);
    assert(HitCombatSupportHud(-2.4f,-1.44f)==CombatHudTarget::Weapon0);
    assert(HitCombatSupportHud(2.4f,-1.44f)==CombatHudTarget::Weapon4);
    assert(RouteCombatInput(CombatHudTarget::Sam,true)==CombatInputRoute::Sam);
    assert(RouteCombatInput(CombatHudTarget::Sam,false)==CombatInputRoute::NoAction);
    assert(RouteCombatInput(CombatHudTarget::Weapon2,true)==CombatInputRoute::Weapon2);
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

    // Battery warnings come from actual integrity loss, not predicted threats.
    BatteryDamageFeedback damage_feedback;
    assert(!damage_feedback.active());
    assert(!damage_feedback.Record(100,100));
    assert(!damage_feedback.Record(100,101));
    assert(!damage_feedback.Record(101,90));
    assert(!damage_feedback.Record(100,-1));
    assert(damage_feedback.Record(100,97));
    assert(damage_feedback.damage()==3);
    damage_feedback.Tick(.30f);
    assert(damage_feedback.active());
    assert(damage_feedback.Record(97,85));
    assert(damage_feedback.damage()==12);
    assert(damage_feedback.remaining()==BatteryDamageFeedback::kDuration);
    damage_feedback.Tick(std::numeric_limits<float>::quiet_NaN());
    assert(damage_feedback.active());
    damage_feedback.Tick(1.0f);
    assert(!damage_feedback.active());
    assert(damage_feedback.Record(2,0) && damage_feedback.damage()==2);

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
    assert(turret_game.infantry_volley().sequence==0);
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
    assert(landed.Start());
    assert(landed.incoming_threats().infantry_seconds<0.0f); // shop
    assert(landed.BeginWave());
    assert(landed.incoming_threats().infantry_seconds<0.0f); // offshore
    for(int i=0;i<4500 && landed.wave_status().troops_landed==0;i++)
        landed.Tick(0.05f);
    assert(landed.wave_status().troops_landed>0);
    assert(landed.infantry()>0);
    // Earliest live landing party still needs to traverse the beach and
    // wait a full salvo period; boats have no imaginary infantry timer.
    const IncomingThreats approaching_infantry=landed.incoming_threats();
    assert(approaching_infantry.infantry_seconds>10.0f);
    assert(approaching_infantry.aircraft_seconds<0.0f);
    // Disembarking is not an instant firing position: troops must cross the
    // exposed beach, then complete a full four-second attack countdown.
    const int32_t integrity=landed.integrity();
    for(int i=0;i<120;i++) landed.Tick(0.05f); // six seconds after first landing
    assert(landed.integrity()==integrity);
    assert(landed.infantry_volley().sequence==0);
    assert(landed.infantry_volley().shooters==0);
    assert(landed.infantry_volley().damage==0);
    // A living shore party approaches the firing line: the estimate enters
    // the visible six-second window before any damage occurs.
    const float shore_countdown=landed.incoming_threats().infantry_seconds;
    assert(shore_countdown>4.0f && shore_countdown<6.0f);
    for(const Infantry &trooper:landed.infantry_units())
        if(trooper.active) assert(trooper.z < -1.34f);
    for(int i=0;i<160;i++) landed.Tick(0.05f);
    assert(landed.integrity()<integrity);
    const InfantryVolleyEvent first_volley=landed.infantry_volley();
    assert(first_volley.sequence>=1);
    assert(landed.incoming_threats().infantry_seconds>=0.0f);
    assert(landed.incoming_threats().infantry_seconds<=4.0f);
    assert(first_volley.shooters>0 && first_volley.shooters<=landed.infantry());
    assert(first_volley.damage>0 && first_volley.damage<=first_volley.shooters*2);
    assert(std::isfinite(first_volley.x) && std::isfinite(first_volley.z));
    bool matched_shooter=false;
    for(const Infantry &trooper:landed.infantry_units())
        if(trooper.active && std::abs(trooper.x-first_volley.x)<.001f &&
           std::abs(trooper.z-first_volley.z)<.001f) matched_shooter=true;
    assert(matched_shooter);
    assert(first_volley.z>=-1.34f);
    while(landed.infantry()) {
        landed.Tick(0.05f);
        for(uint8_t i=0;i<landed.infantry_units().size();++i)
            if(landed.infantry_units()[i].active && landed.FireAtInfantry(i)) break;
    }
    assert(landed.wave_status().troops_stopped>=1);
    assert(landed.incoming_threats().infantry_seconds<0.0f);
    assert(landed.waves_this_period()==2);

    // Infantry volleys must report the real damage against the final hull
    // point, rather than the nominal damage from every ready shooter.
    Campaign last_infantry_hull(Difficulty::Casual,41);
    assert(last_infantry_hull.Start());
    uint8_t last_infantry_save[Campaign::kSerializedSize]={};
    assert(last_infantry_hull.SerializeShop(last_infantry_save,
                                           sizeof(last_infantry_save)));
    last_infantry_save[36]=1; // integrity word, little-endian
    last_infantry_save[37]=0;
    last_infantry_save[38]=0;
    last_infantry_save[39]=0;
    assert(last_infantry_hull.DeserializeShop(last_infantry_save,
                                              sizeof(last_infantry_save)));
    assert(last_infantry_hull.BeginWave());
    for(int i=0;i<1600 && last_infantry_hull.phase()==Phase::Combat;i++)
        last_infantry_hull.Tick(.05f);
    assert(last_infantry_hull.phase()==Phase::Defeat);
    assert(last_infantry_hull.integrity()==0);
    const InfantryVolleyEvent fatal_volley=last_infantry_hull.infantry_volley();
    assert(fatal_volley.sequence==1 && fatal_volley.damage==1);
    assert(fatal_volley.shooters>0);
    assert(std::isfinite(fatal_volley.x) && std::isfinite(fatal_volley.z));

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
    assert(beach.incoming_threats().infantry_seconds>6.0f);
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
    // The only landed soldier is gone: active offshore transports are not
    // counted as shooters until they really disembark.
    if (beach.infantry()==0)
        assert(beach.incoming_threats().infantry_seconds<0.0f);
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
    assert(veteran.stats().anti_air_damage_bonus==0);
    assert(veteran.price(Upgrade::AntiAir)==1050);
    assert(!veteran.Purchase(Upgrade::AntiAir)); // insufficient credits
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
    assert(firing_ship.gunboat_salvo().sequence==0);
    for(int i=0;i<25 && firing_ship.wave_status().gunboat_attacks==0;i++)
        firing_ship.Tick(0.05f);
    assert(firing_ship.wave_status().gunboat_attacks==1);
    assert(firing_ship.integrity()==97);
    const GunboatSalvoEvent first_salvo=firing_ship.gunboat_salvo();
    assert(first_salvo.sequence==1 && first_salvo.damage==3);
    assert(std::isfinite(first_salvo.x) && std::isfinite(first_salvo.z));
    assert(first_salvo.x==firing_ship.enemies()[0].x);
    assert(first_salvo.z==firing_ship.enemies()[0].z);
    BatteryDamageFeedback salvo_feedback;
    assert(salvo_feedback.Record(100,firing_ship.integrity()));
    assert(salvo_feedback.damage()==3);
    for(int i=0;i<65 && firing_ship.wave_status().gunboat_attacks<2;i++)
        firing_ship.Tick(0.05f);
    assert(firing_ship.wave_status().gunboat_attacks==2);
    assert(firing_ship.integrity()==94);
    assert(firing_ship.gunboat_salvo().sequence==2);
    assert(firing_ship.gunboat_salvo().damage==3);
    assert(salvo_feedback.Record(97,firing_ship.integrity()));
    assert(salvo_feedback.damage()==3);

    // Offshore gunfire and shoreline collision are different real events.
    // The surviving ship must cause a blast only when it reaches the shore.
    assert(firing_ship.gunboat_collision().sequence==0);
    bool ram_seen=false;
    for(int frame=0;frame<160 && !ram_seen;frame++) {
        const int32_t before=firing_ship.integrity();
        firing_ship.Tick(.05f);
        const GunboatCollisionEvent ram=firing_ship.gunboat_collision();
        if(ram.sequence>0) {
            assert(ram.sequence==1 && ram.damage>0 && ram.damage<=8);
            assert(before-firing_ship.integrity()>=ram.damage);
            assert(std::isfinite(ram.x) && std::isfinite(ram.z));
            assert(ram.z>=-2.2f && ram.z< -2.0f);
            assert(!firing_ship.enemies()[0].active);
            ram_seen=true;
        }
    }
    assert(ram_seen);
    ImpactEffects ram_fx;
    assert(!ram_fx.RecordGunboatCollision(0,-2.2f,0));
    assert(!ram_fx.RecordGunboatCollision(0,-2.2f,9));
    assert(!ram_fx.RecordGunboatCollision(
        std::numeric_limits<float>::quiet_NaN(),-2.2f,8));
    assert(ram_fx.RecordGunboatCollision(.65f,-2.19f,8));
    assert(ram_fx.pulses()[0].kind==ImpactKind::Explosion);
    assert(std::abs(ram_fx.pulses()[0].x-.65f)<.0001f);
    assert(std::abs(ram_fx.pulses()[0].z+2.19f)<.0001f);

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
    assert(stopped_ship.gunboat_salvo().sequence==0);
    assert(stopped_ship.gunboat_collision().sequence==0);
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
    assert(slow_gunboat.gunboat_salvo().sequence==
           split_gunboat.gunboat_salvo().sequence);
    assert(slow_gunboat.gunboat_salvo().z==
           split_gunboat.gunboat_salvo().z);

    // Fatal offshore salvos report the actual remaining hull integrity,
    // not the nominal three-point weapon damage.
    Campaign last_hull(Difficulty::Casual,8);
    assert(last_hull.Start());
    uint8_t low_hull_save[Campaign::kSerializedSize]={};
    assert(last_hull.SerializeShop(low_hull_save,sizeof(low_hull_save)));
    low_hull_save[16]=1; low_hull_save[20]=2;
    low_hull_save[36]=1; // Iron Quay D2, 1 HP
    assert(last_hull.DeserializeShop(low_hull_save,sizeof(low_hull_save)));
    assert(last_hull.BeginWave());
    // A fatal salvo stops the same substep before the next attacker moves.
    // Previously the rest of the enemy loop continued after lethal damage.
    bool stopped_at_fatal_salvo=false;
    for(int i=0;i<150 && last_hull.phase()==Phase::Combat;i++) {
        const bool second_alive=last_hull.enemies()[1].active;
        const float second_z=last_hull.enemies()[1].z;
        last_hull.Tick(.05f);
        if(last_hull.phase()==Phase::Defeat) {
            assert(second_alive);
            assert(last_hull.enemies()[1].active);
            assert(last_hull.enemies()[1].z==second_z);
            stopped_at_fatal_salvo=true;
        }
    }
    assert(stopped_at_fatal_salvo);
    assert(last_hull.phase()==Phase::Defeat);
    assert(last_hull.gunboat_salvo().sequence==1);
    assert(last_hull.gunboat_salvo().damage==1);

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

    // Aircraft steering is deterministic, bounded by the seeded center line,
    // and does not depend on frame splitting or the rendering eye.
    for(AircraftPattern pattern :
        {AircraftPattern::Bomber,AircraftPattern::Strafer}) {
        const float amplitude = pattern==AircraftPattern::Bomber?.24f:.76f;
        const AircraftFlightPose zero=AircraftFlightAt(pattern,1.2f,1.0f,-11.0f);
        assert(std::abs(zero.x-1.2f)<.0001f);
        assert(std::isfinite(zero.bank) && std::abs(zero.bank)<=.28f);
        for(float z=-11.0f;z<=2.4f;z+=.17f) {
            const AircraftFlightPose left=AircraftFlightAt(pattern,-.8f,1.0f,z);
            const AircraftFlightPose right=AircraftFlightAt(pattern,-.8f,-1.0f,z);
            assert(std::isfinite(left.x) && std::isfinite(left.bank));
            assert(std::abs(left.x+.8f)<=amplitude+.00001f);
            assert(std::abs(left.bank)<=.28f);
            assert(std::abs(left.x+right.x+1.6f)<.00001f);
            assert(std::abs(left.bank+right.bank)<.00001f);
        }
    }
    assert(AircraftFlightAt(AircraftPattern::None,1.2f,1.0f,-5.0f).x==1.2f);
    assert(AircraftFlightAt(AircraftPattern::Strafer,1.2f,0.0f,-5.0f).x==0);
    AircraftHitFeedback hit_feedback;
    Enemy no_aircraft;
    assert(!hit_feedback.Record(no_aircraft,no_aircraft,true));
    hit_feedback.Tick(std::numeric_limits<float>::infinity());
    assert(!hit_feedback.active());

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
    const AircraftAimFeedback unarmed_aim=InspectAircraftAim(bomber,0);
    assert(unarmed_aim.visible && !unarmed_aim.equipped);
    assert(unarmed_aim.hp==100 && unarmed_aim.pattern==AircraftPattern::Bomber);
    assert(!InspectAircraftAim(bomber,-1).visible);
    assert(!InspectAircraftAim(bomber,static_cast<int>(Campaign::kMaxEnemies)).visible);
    assert(!InspectAircraftAim(bomber,1).visible); // unspawned slot
    const Enemy &bomber_approach=bomber.enemies()[0];
    const AircraftFlightPose bomber_pose=AircraftFlightAt(
        bomber_approach.aircraft_pattern,bomber_approach.flight_center_x,
        bomber_approach.flight_sign,bomber_approach.z);
    assert(std::abs(bomber_approach.x-bomber_pose.x)<.000001f);
    assert(std::abs(bomber_approach.bank-bomber_pose.bank)<.000001f);
    const IncomingThreats approaching_bomber=bomber.incoming_threats();
    assert(approaching_bomber.aircraft_seconds>4.0f &&
           approaching_bomber.aircraft_seconds<6.0f);
    assert(approaching_bomber.gunboat_seconds<0.0f);
    assert(bomber.aircraft_attack().sequence==0);
    const int16_t bomber_hp=bomber.enemies()[0].hp;
    const uint8_t bomber_mag=bomber.magazine_left();
    const Enemy unarmed_before=bomber.enemies()[0];
    assert(bomber.FireAt(0));
    assert(!hit_feedback.Record(unarmed_before,bomber.enemies()[0],true));
    assert(!hit_feedback.active());
    assert(bomber.magazine_left()==bomber_mag-1);
    assert(bomber.enemies()[0].hp==bomber_hp);
    for(int i=0;i<140 && bomber.wave_status().aircraft_attacks==0;i++)
        bomber.Tick(0.05f);
    assert(bomber.wave_status().aircraft_attacks==1);
    const AircraftAttackEvent bomb=bomber.aircraft_attack();
    assert(bomb.sequence==1 && bomb.pattern==AircraftPattern::Bomber);
    assert(bomb.damage==12);
    assert(std::isfinite(bomb.x) && std::isfinite(bomb.z));
    assert(std::abs(bomb.x-bomber.enemies()[0].x)<.0001f);
    assert(std::abs(bomb.z-bomber.enemies()[0].z)<.0001f);
    ImpactEffects air_impact;
    const float bomb_origin[]={bomb.x,-.77f,-.93f};
    const float down[]={0.0f,-1.0f,0.0f};
    assert(air_impact.RecordHit(bomb_origin,down,.10f,ImpactKind::Explosion));
    assert(air_impact.pulses()[0].kind==ImpactKind::Explosion);
    assert(bomber.incoming_threats().aircraft_seconds<0.0f);
    assert(bomber.integrity()==88);
    // A lethal bomber hit reports only the integrity actually lost.
    Campaign fragile(Difficulty::Casual,8);
    assert(fragile.Start());
    stage_blackcap_night(fragile);
    uint8_t fragile_save[Campaign::kSerializedSize]={};
    assert(fragile.SerializeShop(fragile_save,sizeof(fragile_save)));
    fragile_save[36]=5; // little-endian integrity field (word 9)
    assert(fragile.DeserializeShop(fragile_save,sizeof(fragile_save)));
    assert(fragile.BeginWave());
    fragile.Tick(.05f);
    for(int i=0;i<140 && fragile.wave_status().aircraft_attacks==0;i++)
        fragile.Tick(.05f);
    assert(fragile.wave_status().aircraft_attacks==1);
    assert(fragile.integrity()==0 && fragile.phase()==Phase::Defeat);
    assert(fragile.aircraft_attack().sequence==1);
    assert(fragile.aircraft_attack().damage==5);

    BatteryDamageFeedback bombing_feedback;
    assert(bombing_feedback.Record(100,bomber.integrity()));
    assert(bombing_feedback.damage()==12);

    // Seed 22 opens the same stage with a strafer that peppers the position
    // repeatedly during one overhead pass instead of behaving like a sky-boat.
    Campaign strafer(Difficulty::Casual,22);
    assert(strafer.Start());
    stage_blackcap_night(strafer);
    assert(strafer.BeginWave());
    strafer.Tick(0.05f);
    assert(strafer.enemies()[0].active);
    assert(strafer.enemies()[0].aircraft_pattern==AircraftPattern::Strafer);
    const Enemy first_strafer=strafer.enemies()[0];
    const float first_x=first_strafer.x;
    for(int i=0;i<110 && strafer.wave_status().aircraft_attacks<4;i++)
        strafer.Tick(0.05f);
    assert(strafer.wave_status().aircraft_attacks==4);
    const Enemy late_strafer=strafer.enemies()[0];
    assert(std::abs(late_strafer.x-first_x)>.10f);
    assert(std::abs(late_strafer.x-late_strafer.flight_center_x)<=.76001f);
    const AircraftFlightPose late_pose=AircraftFlightAt(
        late_strafer.aircraft_pattern,late_strafer.flight_center_x,
        late_strafer.flight_sign,late_strafer.z);
    assert(std::abs(late_pose.x-late_strafer.x)<.000001f);
    assert(std::abs(late_pose.bank-late_strafer.bank)<.000001f);
    const AircraftAttackEvent strafe=strafer.aircraft_attack();
    assert(strafe.sequence==4 && strafe.pattern==AircraftPattern::Strafer);
    assert(strafe.damage==3);
    assert(std::isfinite(strafe.x) && std::isfinite(strafe.z));
    assert(strafer.integrity()==88);

    // A single 0.10-second headset frame and two 0.05-second updates
    // arrive at the same visual/target pose, including the aircraft bank.
    Campaign slow_flight(Difficulty::Casual,22);
    Campaign split_flight(Difficulty::Casual,22);
    assert(slow_flight.Start() && split_flight.Start());
    stage_blackcap_night(slow_flight);
    stage_blackcap_night(split_flight);
    assert(slow_flight.BeginWave() && split_flight.BeginWave());
    for(int step=0;step<24;step++) {
        slow_flight.Tick(.10f);
        split_flight.Tick(.05f);
        split_flight.Tick(.05f);
        const Enemy &slow=slow_flight.enemies()[0];
        const Enemy &split=split_flight.enemies()[0];
        assert(slow.type==UnitType::Aircraft && split.type==UnitType::Aircraft);
        assert(slow.x==split.x && slow.z==split.z && slow.bank==split.bank);
        assert(slow.hp==split.hp);
    }

    // Purchasing AA turns those otherwise ineffective shots into damage and
    // can destroy the bomber before its attack window begins.
    Campaign defended(Difficulty::Casual,8);
    assert(defended.Start());
    assert(defended.Purchase(Upgrade::AntiAir));
    stage_blackcap_night(defended);
    assert(defended.BeginWave());
    defended.Tick(0.05f);
    assert(defended.enemies()[0].type==UnitType::Aircraft);
    const AircraftAimFeedback armed_aim=InspectAircraftAim(defended,0);
    assert(armed_aim.visible && armed_aim.equipped && armed_aim.hp==100);
    for(int shot=0;shot<5;shot++) {
        if(shot>0) for(int i=0;i<7;i++) defended.Tick(0.05f);
        const Enemy before=defended.enemies()[0];
        assert(defended.FireAt(0));
        assert(hit_feedback.Record(before,defended.enemies()[0],true));
        assert(hit_feedback.damage()==20);
        assert(std::abs(hit_feedback.x()-before.x)<.000001f);
        assert(std::abs(hit_feedback.z()-before.z)<.000001f);
        assert(hit_feedback.destroyed()==(shot==4));
        assert(hit_feedback.active());
    }
    assert(!InspectAircraftAim(defended,0).visible);
    hit_feedback.Tick(.54f);
    assert(hit_feedback.active());
    hit_feedback.Tick(.02f);
    assert(!hit_feedback.active());
    assert(!defended.enemies()[0].active);
    assert(defended.incoming_threats().aircraft_seconds<0.0f);
    assert(defended.wave_status().aircraft_destroyed==1);
    assert(defended.wave_status().aircraft_attacks==0);
    assert(defended.aircraft_attack().sequence==0);
    assert(defended.integrity()==100);

    // Non-aircraft targets cannot generate false aircraft brackets or hits.
    Campaign no_target(Difficulty::Casual,41);
    assert(no_target.Start() && no_target.BeginWave());
    no_target.Tick(.05f);
    assert(no_target.enemies()[0].type==UnitType::Rowboat);
    assert(!InspectAircraftAim(no_target,0).visible);
    assert(!InspectAircraftAim(no_target,255).visible);
    // Recorded weapon damage must remain tied to a fired, real aircraft hit.
    AircraftHitFeedback ignored_hit;
    const Enemy already_down=defended.enemies()[0];
    assert(!ignored_hit.Record(already_down,already_down,true));
    assert(!ignored_hit.Record(unarmed_before,defended.enemies()[0],false));
    assert(!ignored_hit.active());

    // AA is a real three-tier investment: a basic engagement kit, then
    // +15 / +30 aircraft-only damage. The shop shows prices 750/1050/1350
    // and saved campaigns retain the full tier without changing save size.
    auto funded_aa=[&](uint8_t tiers) {
        Campaign game(Difficulty::Casual,8);
        assert(game.Start());
        uint8_t save[Campaign::kSerializedSize]={};
        assert(game.SerializeShop(save,sizeof(save)));
        // Supply earned campaign credits in this deterministic shop fixture.
        save[32]=0x10; save[33]=0x27; // 10000, little-endian word 8
        assert(game.DeserializeShop(save,sizeof(save)));
        assert(game.price(Upgrade::AntiAir)==750);
        assert(game.stats().anti_air_damage_bonus==0);
        assert(game.stats().gun_damage==20);
        assert(!game.stats().anti_air);
        for(uint8_t tier=0;tier<tiers;++tier) {
            assert(game.price(Upgrade::AntiAir)==750+300*tier);
            assert(game.Purchase(Upgrade::AntiAir));
        }
        assert(game.level(Upgrade::AntiAir)==tiers);
        assert(game.stats().anti_air==(tiers>0));
        assert(game.stats().anti_air_damage_bonus==
               (tiers>1?15*(tiers-1):0));
        assert(game.stats().gun_damage==20); // no surface-weapon buff
        assert(game.credits()==10000-(tiers==0?0:
            tiers==1?750:tiers==2?1800:3150));
        if(tiers==3) {
            assert(game.price(Upgrade::AntiAir)==-1);
            assert(!game.Purchase(Upgrade::AntiAir));
        }
        assert(game.SerializeShop(save,sizeof(save)));
        Campaign loaded;
        assert(loaded.DeserializeShop(save,sizeof(save)));
        assert(loaded.level(Upgrade::AntiAir)==tiers);
        assert(loaded.stats().anti_air_damage_bonus==
               game.stats().anti_air_damage_bonus);
        if(tiers==3) {
            // Invalid tier 4 is rejected transactionally; the live shop stays
            // maxed and retains its credits and legal upgraded damage.
            save[60]=4; // upgrade[3] at serialized word 15
            assert(!loaded.DeserializeShop(save,sizeof(save)));
            assert(loaded.level(Upgrade::AntiAir)==3);
            assert(loaded.credits()==game.credits());
        }
        return game;
    };
    Campaign aa_tier1=funded_aa(1);
    Campaign aa_tier2=funded_aa(2);
    Campaign aa_tier3=funded_aa(3);
    // All three start the identical deterministic bomber encounter.
    // Higher AA tiers intercept with fewer shots, without consuming airstrikes
    // or changing wave attack timing and without changing shell controls.
    auto engage_bomber=[&](Campaign &game,uint8_t shots) {
        stage_blackcap_night(game);
        assert(game.BeginWave());
        game.Tick(.05f);
        assert(game.enemies()[0].type==UnitType::Aircraft);
        assert(game.enemies()[0].hp==100);
        for(uint8_t shot=0;shot<shots;shot++) {
            if(shot>0) for(int i=0;i<7;i++) game.Tick(.05f);
            assert(game.FireAt(0));
            assert(game.enemies()[0].active==(shot+1<shots));
        }
        assert(game.wave_status().aircraft_destroyed==1);
        assert(game.incoming_threats().aircraft_seconds<0.0f);
        assert(game.integrity()==100);
    };
    engage_bomber(aa_tier1,5); // 20 damage
    engage_bomber(aa_tier2,3); // 35 damage
    engage_bomber(aa_tier3,2); // 50 damage

    // One Select press remains captured across UI drift for both the switch
    // strip and SAM launcher, never accidentally firing the basic gun.
    CombatInputLatch weapon_latch;
    assert(weapon_latch.Route(CombatHudTarget::Weapon3,true)==
           CombatInputRoute::Weapon3);
    assert(weapon_latch.Route(CombatHudTarget::None,false)==
           CombatInputRoute::NoAction);
    weapon_latch.Release();
    assert(weapon_latch.Route(CombatHudTarget::Sam,true)==CombatInputRoute::Sam);
    assert(weapon_latch.Route(CombatHudTarget::None,false)==
           CombatInputRoute::NoAction);
    weapon_latch.Release();
    assert(weapon_latch.Route(CombatHudTarget::None,true)==CombatInputRoute::Gun);

    // Armed SAM consumes exactly one Select edge, including an invalid lock.
    // Neither pointer drift nor continued holding may fire the main gun.
    CombatInputLatch sam_fire_latch;
    assert(!sam_fire_latch.CaptureOneShotPress(false));
    assert(sam_fire_latch.Route(CombatHudTarget::None,false)==CombatInputRoute::Gun);
    assert(sam_fire_latch.CaptureOneShotPress(true));
    assert(!sam_fire_latch.CaptureOneShotPress(true));
    assert(sam_fire_latch.Route(CombatHudTarget::None,false)==CombatInputRoute::NoAction);
    assert(sam_fire_latch.Route(CombatHudTarget::Artillery,false)==CombatInputRoute::NoAction);
    sam_fire_latch.Release();
    assert(sam_fire_latch.CaptureOneShotPress(true)); // failed lock is still spent
    assert(sam_fire_latch.Route(CombatHudTarget::None,false)==CombatInputRoute::NoAction);
    sam_fire_latch.Release();
    assert(sam_fire_latch.Route(CombatHudTarget::None,true)==CombatInputRoute::Gun);

    // Weapon ownership, selection and SAM supplies survive the compact
    // 80-byte v2 shop save, while v1 saves still load as a basic machine gun.
    Campaign arsenal(Difficulty::Casual,8);
    assert(arsenal.Start());
    uint8_t arsenal_save[Campaign::kSerializedSize]={};
    assert(arsenal.SerializeShop(arsenal_save,sizeof(arsenal_save)));
    arsenal_save[32]=0x10; arsenal_save[33]=0x27; // 10000 shop credits
    assert(arsenal.DeserializeShop(arsenal_save,sizeof(arsenal_save)));
    assert(arsenal.owns_weapon(WeaponKind::MachineGun));
    assert(!arsenal.owns_weapon(WeaponKind::Flak));
    assert(!arsenal.SelectWeapon(WeaponKind::MissileBattery));
    assert(arsenal.PurchaseWeapon(WeaponKind::Flak));
    assert(arsenal.selected_weapon()==WeaponKind::Flak);
    assert(arsenal.stats().anti_air);
    assert(arsenal.stats().projectile_gravity==.001f);
    assert(arsenal.PurchaseWeapon(WeaponKind::MissileBattery));
    assert(arsenal.selected_weapon()==WeaponKind::MissileBattery);
    assert(arsenal.stats().magazine==2);
    assert(arsenal.PurchaseWeapon(WeaponKind::DirectCannon));
    assert(arsenal.stats().projectile_gravity==.026f);
    assert(arsenal.PurchaseSam());
    assert(arsenal.sam_charges()==1);
    assert(arsenal.SerializeShop(arsenal_save,sizeof(arsenal_save)));
    Campaign loaded_arsenal;
    assert(loaded_arsenal.DeserializeShop(arsenal_save,sizeof(arsenal_save)));
    assert(loaded_arsenal.owns_weapon(WeaponKind::MissileBattery));
    assert(loaded_arsenal.selected_weapon()==WeaponKind::DirectCannon);
    assert(loaded_arsenal.sam_charges()==1);
    arsenal_save[64]=0; // zero owned mask invalid in v2
    assert(!loaded_arsenal.DeserializeShop(arsenal_save,sizeof(arsenal_save)));
    assert(loaded_arsenal.owns_weapon(WeaponKind::MissileBattery));
    arsenal_save[64]=static_cast<uint8_t>(31);
    arsenal_save[68]=9; // invalid selected weapon
    assert(!loaded_arsenal.DeserializeShop(arsenal_save,sizeof(arsenal_save)));
    assert(loaded_arsenal.sam_charges()==1);
    // The original v1 binary format must still restore the default MG.
    Campaign legacy(Difficulty::Casual,9);
    assert(legacy.Start());
    uint8_t legacy_save[Campaign::kSerializedSize]={};
    assert(legacy.SerializeShop(legacy_save,sizeof(legacy_save)));
    legacy_save[4]=1; legacy_save[64]=0; legacy_save[68]=0;
    legacy_save[76]=0;
    Campaign old_loaded;
    assert(old_loaded.DeserializeShop(legacy_save,sizeof(legacy_save)));
    assert(old_loaded.selected_weapon()==WeaponKind::MachineGun);
    assert(!old_loaded.owns_weapon(WeaponKind::Flak));
    assert(old_loaded.sam_charges()==0);
    // A rejected SAM target does not consume a charge; a valid aircraft
    // interception consumes one charge and destroys exactly one aircraft.
    Campaign sam_game(Difficulty::Casual,8);
    assert(sam_game.Start() && sam_game.PurchaseSam());
    stage_blackcap_night(sam_game);
    assert(sam_game.BeginWave());
    sam_game.Tick(.05f);
    assert(sam_game.enemies()[0].active &&
           sam_game.enemies()[0].type==UnitType::Aircraft);
    assert(!sam_game.CallSam(1)); // unoccupied slot
    assert(sam_game.sam_charges()==1);
    assert(sam_game.CallSam(0));
    assert(sam_game.sam_charges()==0);
    assert(!sam_game.enemies()[0].active);
    assert(sam_game.wave_status().aircraft_destroyed==1);
    assert(!sam_game.CallSam(0)); // cannot double-launch
    assert(loaded_arsenal.BeginWave());
    loaded_arsenal.Tick(.05f);
    assert(!loaded_arsenal.SelectWeapon(WeaponKind::HeavyGun));
    assert(loaded_arsenal.SelectWeapon(WeaponKind::MachineGun));
    assert(loaded_arsenal.selected_weapon()==WeaponKind::MachineGun);
    // Switch to the owned flak while the default gun is on cooldown.
    assert(loaded_arsenal.SelectWeapon(WeaponKind::Flak));
    assert(loaded_arsenal.stats().anti_air);
    assert(loaded_arsenal.SelectWeapon(WeaponKind::MissileBattery));
    assert(!loaded_arsenal.FireMiss()); // missiles require genuine lock
    assert(loaded_arsenal.SelectWeapon(WeaponKind::MachineGun));

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
