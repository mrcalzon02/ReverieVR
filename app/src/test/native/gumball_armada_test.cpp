#include "gumball_armada_simulation.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::gumball;
int main(){
    // Combat formations cover all eight deterministic doctrines.
    bool doctrines[8]={};
    for(unsigned lane=0;lane<64;++lane)for(int group=0;group<8;++group){
        const int index=group*5;
        const WaveEntry entry=PlanWaveEntry(42u,lane,index,40,13u);
        assert(entry.formation==FormationFor(42u,lane,index));
        assert(entry.x>=-1.7f&&entry.x<=1.7f);
        doctrines[static_cast<unsigned>(entry.formation)]=true;
    }
    for(bool seen:doctrines)assert(seen);
    // The live spawn loop must assign the seed-selected formation and slot,
    // rather than only displaying a formation name on the HUD.
    Simulation liveDoctrine(23);
    assert(liveDoctrine.launch(0));
    liveDoctrine.update(.05f,0.f,0.f,false);
    assert(liveDoctrine.spawned()==1);
    const Enemy& first=liveDoctrine.enemies()[0];
    assert(first.active&&!first.boss&&first.formation_slot==0u);
    assert(first.formation==FormationFor(23u,liveDoctrine.lane(0).id,0));
    // The tracked controller steers without a held touchpad. Higher
    // thruster level and genuine built ship thrust shorten target lag.
    const float rayOrigin[3]={0.f,0.f,0.f};
    const float rayDirection[3]={.60f,-.15f,-1.f};
    const FlightAim desired=FlightAimFromRay(rayOrigin,rayDirection);
    assert(desired.valid&&desired.x>1.5f);
    Simulation pointerStarter(99),pointerUpgrade(99),pointerBuilt(99);
    auto fasterSave=pointerUpgrade.snapshot();
    fasterSave.levels[2]=10;
    fasterSave.checksum=SnapshotChecksum(fasterSave);
    assert(pointerUpgrade.restore(fasterSave));
    Shipyard fasterHull;
    assert(fasterHull.place(4,2,Part::Thruster));
    assert(fasterHull.ready());
    assert(pointerBuilt.configure_ship(fasterHull));
    assert(pointerStarter.launch(0)&&pointerUpgrade.launch(0)
           &&pointerBuilt.launch(0));
    assert(pointerUpgrade.pointer_move_speed()>pointerStarter.pointer_move_speed());
    assert(pointerBuilt.pointer_move_speed()>pointerStarter.pointer_move_speed());
    Simulation* controlledShips[]={&pointerStarter,&pointerUpgrade,&pointerBuilt};
    for(int frame=0;frame<8;++frame){
        for(Simulation* ship:controlledShips){
            const auto axes=FlightAxesToTarget(ship->player_x(),ship->player_y(),
                                               desired,.025f,ship->pointer_move_speed());
            ship->update(.025f,axes.x,axes.y,false,true);
        }
    }
    assert(pointerStarter.player_x()>0.f);
    assert(pointerUpgrade.player_x()>pointerBuilt.player_x());
    assert(pointerBuilt.player_x()>pointerStarter.player_x());
    assert(pointerStarter.shots_fired()==0u);
    // A touchpad press still fires while pointing the ship elsewhere.
    const auto activeAxes=FlightAxesToTarget(pointerStarter.player_x(),
        pointerStarter.player_y(),desired,.025f,pointerStarter.pointer_move_speed());
    pointerStarter.update(.025f,activeAxes.x,activeAxes.y,true,true);
    assert(pointerStarter.shots_fired()>0u);
    Simulation game(42), replay(42);
    assert(game.node()==0 && game.cash()==100 && kLaneCount==4096);
    for(unsigned i=0;i<3;++i){
        const auto a=game.lane(i), b=replay.lane(i);
        assert(a.id==b.id && a.hazard==b.hazard && a.id<kLaneCount);
    }
    assert(game.buy(Upgrade::Blaster));
    assert(game.level(Upgrade::Blaster)==1 && game.cash()==30);
    assert(!game.buy(Upgrade::Blaster));
    const auto save=game.snapshot();
    assert(replay.restore(save));
    assert(replay.cash()==30 && replay.level(Upgrade::Blaster)==1);
    auto tampered=save;tampered.cash=9999;
    assert(!replay.restore(tampered));

    // Steering must not depend on the fire button.
    // Route choice changes mission rules rather than selecting renamed waves.
    Simulation interception(42), recovery(42);
    assert(interception.launch(1));
    assert(interception.mission().kind==MissionKind::Intercept);
    assert(interception.mission().permitted_escapes==0);
    const float initialCountdown=interception.remaining_time();
    interception.update(.05f,0.f,0.f,false);
    assert(interception.remaining_time()<initialCountdown);
    assert(recovery.launch(2));
    assert(recovery.mission().kind==MissionKind::Recovery);
    assert(recovery.mission().required_salvage==3);
    assert(recovery.salvage_collected()==0);
    assert(recovery.salvage().empty());
    assert(game.mission().kind==MissionKind::Sweep);
    Simulation pad(9001);assert(pad.launch(0));
    const float startX=pad.player_x();
    pad.update(.05f,1.0f,0.0f,false);
    assert(pad.player_x()>startX && pad.kills()==0);
    // Fortress routes cannot be completed by slipping past the boss.
    Simulation fortress(7);
    auto edge=fortress.snapshot();
    edge.node=1022;edge.checksum=SnapshotChecksum(edge);
    assert(fortress.restore(edge));
    for(unsigned i=0;i<3;++i)
        assert(fortress.lane(i).id==1023 && fortress.lane(i).fortress);
    assert(fortress.launch(0));
    for(int i=0;i<24000&&fortress.phase()==Phase::Flight;++i)
        fortress.update(.05f,0,0,false);
    assert(fortress.phase()==Phase::Lost);
    assert(fortress.bosses()==0u && fortress.node()==1022u);
    edge=fortress.snapshot();
    edge.node=1023;edge.bosses=1u;edge.checksum=SnapshotChecksum(edge);
    assert(fortress.restore(edge));
    assert(fortress.lane(1).id==1024u);
    assert(!game.launch(3));
    assert(game.launch(0) && replay.launch(0));
    assert(!game.buy(Upgrade::Shield));
    for(int i=0;i<200;++i){
        const float x=i%23<11?.5f:-.5f;
        game.update(.025f,x,0,true);
        replay.update(.025f,x,0,true);
        assert(game.hp()==replay.hp() && game.cash()==replay.cash());
        assert(game.phase()==replay.phase());
        assert(std::fabs(game.player_x()-replay.player_x())<0.0001f);
    }
    for(int i=0;i<30000 && game.phase()==Phase::Flight;++i)
        game.update(.05f,0,0,false);
    assert(game.phase()==Phase::Lost || game.phase()==Phase::Won);
    assert(game.recover() && game.phase()==Phase::Chart);
    assert(!game.recover());
    Simulation idle(321);assert(idle.launch(0));
    for(int i=0;i<12000 && idle.phase()==Phase::Flight;++i)
        idle.update(.05f,0,0,false);
    assert(idle.phase()==Phase::Lost);
    const auto finalSave=game.snapshot();
    assert(replay.restore(finalSave));
    assert(replay.node()==game.node() && replay.bosses()==game.bosses());
    assert(!replay.restore(tampered));
    // Verify that an actual pilot can win by steering and hitting enemies.
    Simulation ace(4);
    auto equipped=ace.snapshot();
    for(auto &lv:equipped.levels)lv=12;
    equipped.checksum=SnapshotChecksum(equipped);
    assert(ace.restore(equipped)&&ace.launch(0));
    assert(ace.wave()>=1&&ace.max_combo()==0&&ace.sortie_cash()==0);
    int scouts=0,interceptors=0,bombers=0;
    for(int tick=0;tick<12000&&ace.phase()==Phase::Flight;++tick){
        float nearest=100.f,targetX=ace.player_x();
        for(const auto& e:ace.enemies())if(e.active){
            if(e.kind==EnemyKind::Scout)++scouts;
            if(e.kind==EnemyKind::Interceptor)++interceptors;
            if(e.kind==EnemyKind::Bomber)++bombers;
            const float distance=std::fabs(e.y-ace.player_y());
            if(distance<nearest){nearest=distance;targetX=e.x;}
        }
        const float steering=targetX>ace.player_x()+.025f?1.f:
            (targetX<ace.player_x()-.025f?-1.f:0.f);
        ace.update(.025f,steering,0.f,true);
    }
    assert(ace.phase()==Phase::Won);
    assert(scouts>0&&interceptors>0&&bombers>0);
    assert(ace.escaped()==0&&ace.defeated()==ace.target());
    assert(ace.shots_fired()>ace.shots_hit()&&ace.shots_hit()>0);
    assert(ace.completion_bonus()>0&&ace.sortie_cash()>ace.completion_bonus());
    assert(ace.max_combo()>0&&ace.wave()==3&&ace.visited()==1);
    const uint32_t wonCash=ace.cash();
    assert(ace.recover()&&ace.cash()==wonCash);
    assert(ace.launch(1)&&ace.sortie_cash()==0&&ace.completion_bonus()==0);

    // Three escapes fail the mission even with fully enhanced protection.
    Simulation leaked(17);auto shielded=leaked.snapshot();
    shielded.levels[3]=12;shielded.levels[1]=12;
    shielded.checksum=SnapshotChecksum(shielded);
    assert(leaked.restore(shielded)&&leaked.launch(0));
    for(int i=0;i<12000&&leaked.phase()==Phase::Flight;++i)
        leaked.update(.025f,0.f,0.f,false);
    assert(leaked.phase()==Phase::Lost&&leaked.escaped()==3);
    assert(leaked.failure()==FailureReason::Escapes);
    assert(leaked.completion_bonus()==0&&leaked.node()==0);
    // Three functional weapons, paid ownership, equipment and v1 save migration.
    Simulation arsenal(7);
    assert(arsenal.weapon()==Weapon::PeaRepeater);
    assert(arsenal.owns_weapon(Weapon::PeaRepeater));
    assert(!arsenal.select_weapon(Weapon::MintSpread));
    assert(!arsenal.buy_weapon(Weapon::RockCannon));
    auto funded=arsenal.snapshot();funded.cash=500;
    funded.checksum=SnapshotChecksum(funded);
    assert(arsenal.restore(funded));
    assert(arsenal.buy_weapon(Weapon::RockCannon));
    assert(arsenal.cash()==380&&arsenal.weapon()==Weapon::RockCannon);
    assert(arsenal.buy_weapon(Weapon::MintSpread)&&arsenal.cash()==220);
    assert(arsenal.select_weapon(Weapon::PeaRepeater));
    assert(arsenal.select_weapon(Weapon::MintSpread));
    const Snapshot weaponSave=arsenal.snapshot();
    Simulation fresh(12);
    assert(fresh.restore(weaponSave));
    assert(fresh.cash()==220&&fresh.owns_weapon(Weapon::RockCannon));
    assert(fresh.weapon()==Weapon::MintSpread);
    auto invalidLoadout=weaponSave;
    invalidLoadout.owned_weapons=0;
    invalidLoadout.checksum=SnapshotChecksum(invalidLoadout);
    assert(!fresh.restore(invalidLoadout));
    SnapshotV1 legacy{};
    legacy.seed=7;legacy.cash=150;legacy.node=42;legacy.levels[0]=5;
    legacy.checksum=SnapshotV1Checksum(legacy);
    Simulation migrated;
    assert(migrated.restore(legacy)&&migrated.node()==42);
    assert(migrated.cash()==150&&migrated.level(Upgrade::Blaster)==5);
    assert(migrated.weapon()==Weapon::PeaRepeater);
    assert(!migrated.owns_weapon(Weapon::RockCannon));
    SnapshotV1 invalidLegacy=legacy;
    invalidLegacy.cash=999;
    assert(!migrated.restore(invalidLegacy));
    Simulation cannon(123),spread(123);
    auto bank=cannon.snapshot();bank.cash=1000;bank.checksum=SnapshotChecksum(bank);
    assert(cannon.restore(bank)&&spread.restore(bank));
    assert(cannon.buy_weapon(Weapon::RockCannon)&&cannon.launch(0));
    assert(spread.buy_weapon(Weapon::MintSpread)&&spread.launch(0));
    cannon.update(.01f,0,0,true);
    spread.update(.01f,0,0,true);
    assert(cannon.shots_fired()==1&&spread.shots_fired()==3);
    int cannonPower=0,spreadPower=0;
    for(const auto& shot:cannon.shots())if(shot.active&&!shot.hostile)cannonPower=shot.damage;
    for(const auto& shot:spread.shots())if(shot.active&&!shot.hostile)spreadPower+=shot.damage;
    assert(cannonPower==4&&spreadPower==3);
    // Even an otherwise flawless run fails if its mandatory ace escapes.
    Simulation skippedAce(4);
    auto strong=skippedAce.snapshot();
    for(auto& lv:strong.levels)lv=12;
    strong.checksum=SnapshotChecksum(strong);
    assert(skippedAce.restore(strong)&&skippedAce.launch(0));
    for(int tick=0;tick<18000&&skippedAce.phase()==Phase::Flight;++tick){
        float near=99.f,x=skippedAce.player_x();
        bool shoot=false;
        for(const auto& enemy:skippedAce.enemies())if(enemy.active&&enemy.kind!=EnemyKind::Ace){
            const float distance=std::fabs(enemy.y-skippedAce.player_y());
            if(distance<near){near=distance;x=enemy.x;shoot=true;}
        }
        const float steer=x>skippedAce.player_x()+.025f?1.f:
            (x<skippedAce.player_x()-.025f?-1.f:0.f);
        skippedAce.update(.025f,steer,0.f,shoot);
    }
    assert(skippedAce.phase()==Phase::Lost&&skippedAce.escaped()==1);
    assert(skippedAce.failure()==FailureReason::EliteEscaped);
    assert(skippedAce.node()==0&&skippedAce.completion_bonus()==0);
    // Default unpaid starter ship must be able to complete the first
    // reasonable-risk lane under deterministic steering/held-fire input.
    Simulation starter;
    assert(starter.cash()==100&&starter.weapon()==Weapon::PeaRepeater);
    assert(starter.lane(0).hazard<=3&&starter.launch(0));
    for(int frame=0;frame<15000&&starter.phase()==Phase::Flight;++frame){
        float target=starter.player_x(),near=100.f;
        for(const auto& e:starter.enemies())if(e.active){
            const float distance=std::fabs(e.y-starter.player_y());
            if(distance<near){near=distance;target=e.x;}
        }
        const float axis=target>starter.player_x()+.025f?1.f:
            (target<starter.player_x()-.025f?-1.f:0.f);
        starter.update(.025f,axis,0.f,true);
    }
    assert(starter.phase()==Phase::Won);
    assert(starter.failure()==FailureReason::None);
    assert(starter.hp()>0&&starter.escaped()<3);
    assert(starter.completion_bonus()>0&&starter.visited()==1);
    // Game integration: legal installed blocks materially change combat state.
    Shipyard starterShip,customShip;
    assert(starterShip.ready()&&customShip.ready());
    assert(customShip.place(4,2,Part::Armor));
    assert(customShip.place(2,2,Part::Cargo));
    assert(customShip.ready());
    Simulation basic(55),built(55);
    assert(basic.configure_ship(starterShip));
    assert(built.configure_ship(customShip));
    assert(basic.launch(0)&&built.launch(0));
    assert(built.hp()==basic.hp()+14);
    assert(built.ship_profile().pickup_radius>basic.ship_profile().pickup_radius);
    assert(built.ship_profile().speed<basic.ship_profile().speed);
    Shipyard turretShip;
    assert(turretShip.place(2,2,Part::Mount));
    assert(turretShip.ready()&&turretShip.profile().mounts==2);
    Simulation doubleBarrel(88);
    assert(doubleBarrel.configure_ship(turretShip)&&doubleBarrel.launch(0));
    doubleBarrel.update(.01f,0.f,0.f,true);
    assert(doubleBarrel.shots_fired()==2);
    float firstY=0.f,secondY=0.f;
    int gunshots=0;
    for(const auto& shot:doubleBarrel.shots())if(shot.active&&!shot.hostile){
        if(gunshots++==0)firstY=shot.y;else secondY=shot.y;
    }
    assert(gunshots==2&&std::fabs(firstY-secondY)>.05f);
    Shipyard brokenShip;
    assert(brokenShip.remove(3,1));
    assert(!brokenShip.ready());
    Simulation blocked(55);
    assert(!blocked.configure_ship(brokenShip));
    assert(!blocked.launch(0));
    assert(brokenShip.place(3,1,Part::Thruster));
    assert(blocked.configure_ship(brokenShip));
    assert(blocked.launch(0));
    // Seeded guest encounters are real, bounded events after a completed run.
    assert(starter.guest()==CastGuest::None);
    bool foundPip=false,foundLolly=false,foundBlort=false,foundNib=false,foundTwins=false;
    for(uint32_t seed=1;seed<512&&!(foundPip&&foundLolly&&foundBlort&&foundNib&&foundTwins);++seed){
        Simulation trial(seed);
        const auto lane=trial.lane(0);
        const CastGuest expected=ChooseCastGuest(seed,lane.id,2u,lane.hazard);
        if(expected==CastGuest::None
            ||(expected==CastGuest::PipParfait&&foundPip)
            ||(expected==CastGuest::LollyWren&&foundLolly)
            ||(expected==CastGuest::BlortBlister&&foundBlort)
            ||(expected==CastGuest::NibNoxious&&foundNib)
            ||(expected==CastGuest::CrumbclankTwins&&foundTwins))continue;
        auto progressed=trial.snapshot();progressed.visited=2;
        progressed.levels[1]=12;progressed.levels[3]=12;
        // Guest tests require a player who fights. An idle ship may fail a
        // Sweep after its third escaped enemy, before twins even spawn.
        const bool needsMidwaveFoe=expected==CastGuest::CrumbclankTwins
            ||expected==CastGuest::BlortBlister
            ||expected==CastGuest::NibNoxious;
        if(needsMidwaveFoe)
            progressed.levels[0]=12; // real max-level blaster to keep sortie alive

        progressed.checksum=SnapshotChecksum(progressed);
        assert(trial.restore(progressed)&&trial.launch(0));
        assert(trial.guest()==expected);
        bool sawAssist=false,sawGrenadier=false,sawBomb=false;
        bool sawNib=false,sawMine=false,sawBiff=false,sawBaff=false;
        for(int tick=0;tick<750&&trial.phase()==Phase::Flight;++tick){
            float move=0.f;
            // Clear early regular ships, then let each mid-wave named
            // encounter (Blort/Nib/Biff/Baff) actually spawn and fire.
            // Never idle through the third sweep escape before the guest,
            // and never destroy the named target before its hazard appears.
            const bool combat=needsMidwaveFoe &&
                trial.spawned()<((6+static_cast<int>(lane.hazard)*2)/2-1);
            if(combat){
                // Aim at the nearest real enemy rather than idling until
                // the third escape ends the sortie before the guest wave.
                const Enemy* closest=nullptr;
                for(const auto& enemy:trial.enemies())if(enemy.active){
                    if(!closest||enemy.y<closest->y)closest=&enemy;
                }
                if(closest){
                    const float error=closest->x-trial.player_x();
                    move=error>.035f?1.f:error<-.035f?-1.f:0.f;
                }
            }
            trial.update(.025f,move,0.f,combat);
            sawAssist|=trial.guest_active();
            for(const auto& enemy:trial.enemies())if(enemy.active){
                sawGrenadier|=enemy.kind==EnemyKind::Blort;
                sawNib|=enemy.kind==EnemyKind::Nib;
                sawBiff|=enemy.kind==EnemyKind::Biff;
                sawBaff|=enemy.kind==EnemyKind::Baff;
            }
            for(const auto& shot:trial.shots())if(shot.active){
                sawBomb|=shot.bomb;sawMine|=shot.mine;
            }
        }
        assert(trial.guest_pulses()<=5);
        if(expected==CastGuest::PipParfait){
            assert(sawAssist&&trial.ally_shots()>0u);
            assert(trial.shots_hit()<=trial.shots_fired());
            foundPip=true;
        }else if(expected==CastGuest::LollyWren){
            assert(sawAssist);
            assert(trial.ally_shots()==0u);
            foundLolly=true;
        }else if(expected==CastGuest::BlortBlister){
            assert(sawGrenadier&&sawBomb&&!sawAssist);
            foundBlort=true;
        }else if(expected==CastGuest::NibNoxious){
            assert(sawNib&&sawMine&&!sawAssist);
            foundNib=true;
        }else if(expected==CastGuest::CrumbclankTwins){
            if(!(sawBiff&&sawBaff&&sawBomb&&!sawAssist))
                std::cerr<<"Crumbclank encounter diagnostic: seed="<<seed
                         <<" destination="<<lane.id
                         <<" hazard="<<static_cast<unsigned>(lane.hazard)
                         <<" spawned="<<trial.spawned()
                         <<" wave="<<trial.wave()
                         <<" phase="<<static_cast<int>(trial.phase())
                         <<" hp="<<trial.hp()
                         <<" Biff="<<sawBiff<<" Baff="<<sawBaff
                         <<" bomb="<<sawBomb
                         <<" assist="<<sawAssist<<'\n';
            assert(sawBiff&&sawBaff&&sawBomb&&!sawAssist);
            assert(trial.twin_bonus_paid()==0u); // cannot claim kill bonus while no weapons fired
            foundTwins=true;
        }
    }
    assert(foundPip&&foundLolly&&foundBlort&&foundNib&&foundTwins);
    // Two paid French-inspired allies keep upgrades and mission charge in one
    // checksummed V3 campaign balance rather than in a non-atomic second slot.
    Simulation contracts(109);
    assert(contracts.hire_support(SupportKind::CapitaineEclair));
    assert(contracts.cash()==55u);
    assert(!contracts.hire_support(SupportKind::MademoiselleMacaron));
    const auto booked=contracts.snapshot();
    assert(booked.version==3u&&sizeof(booked)==56u);
    Simulation reread(109);
    assert(reread.restore(booked));
    assert(reread.support().booked()==SupportKind::CapitaineEclair);
    assert(reread.cash()==55u);
    assert(reread.launch(0));
    assert(reread.support().booked()==SupportKind::None);
    assert(reread.deployed_support()==SupportKind::CapitaineEclair);
    assert(reread.snapshot().support_state>>8u==0u);
    for(int i=0;i<520&&reread.phase()==Phase::Flight;++i)
        reread.update(.025f,0.f,0.f,false);
    assert(reread.support_pulses()>0);
    auto funds=contracts.snapshot();
    funds.cash=900u;funds.checksum=SnapshotChecksum(funds);
    Simulation boosted(109);assert(boosted.restore(funds));
    assert(boosted.upgrade_support(SupportSkill::EclairVolley));
    assert(boosted.upgrade_support(SupportSkill::MacaronRelief));
    const auto enhanced=boosted.snapshot();
    Simulation updated(109);
    assert(updated.restore(enhanced));
    assert(updated.support().volley_damage()==3);
    assert(updated.support().shield_repair()==10);
    auto invalidPaid=enhanced;invalidPaid.support_state|=0x80000000u;
    invalidPaid.checksum=SnapshotChecksum(invalidPaid);
    assert(!updated.restore(invalidPaid));
    invalidPaid=enhanced;invalidPaid.support_state=(invalidPaid.support_state&~(3u<<8u))|(3u<<8u);
    invalidPaid.checksum=SnapshotChecksum(invalidPaid);
    assert(!updated.restore(invalidPaid));
    SnapshotV2 oldV2{};
    oldV2.seed=443u;oldV2.cash=88u;oldV2.visited=2u;
    oldV2.owned_weapons=1u;oldV2.active_weapon=0u;
    oldV2.checksum=SnapshotV2Checksum(oldV2);
    Simulation migratedV2;
    assert(migratedV2.restore(oldV2));
    assert(migratedV2.cash()==88u);
    assert(migratedV2.snapshot().version==3u);
    assert(migratedV2.support().booked()==SupportKind::None);
    // Purchased emergency consumables live in the same checked V3 record
    // as the wallet, independent of the one booked paid companion.
    Simulation emergencyGame(449u);
    auto kitBank=emergencyGame.snapshot();
    kitBank.cash=300u;kitBank.checksum=SnapshotChecksum(kitBank);
    assert(emergencyGame.restore(kitBank));
    assert(emergencyGame.buy_support_kit(SupportKit::HullPatch));
    assert(emergencyGame.buy_support_kit(SupportKit::ShieldTonic));
    assert(emergencyGame.cash()==190u);
    assert(!emergencyGame.buy_support_kit(SupportKit::HullPatch));
    assert(emergencyGame.support().kit_mask()==3u);
    const auto emergencySave=emergencyGame.snapshot();
    assert((emergencySave.support_state>>10u&3u)==3u);
    Simulation emergencyReload(449u);
    assert(emergencyReload.restore(emergencySave));
    assert(emergencyReload.support().kit_mask()==3u);
    assert(emergencyReload.launch(0));
    assert(!emergencyReload.buy_support_kit(SupportKit::ShieldTonic));
    emergencyReload.update(.02f,0.f,0.f,false);
    assert(emergencyReload.kit_activations()==0u); // don't consume while healthy
    assert(emergencyReload.support().kit_mask()==3u);
    auto corruptKits=emergencySave;
    corruptKits.support_state|=1u<<12u;
    corruptKits.checksum=SnapshotChecksum(corruptKits);
    Simulation emergencyTamper(449u);
    assert(!emergencyTamper.restore(corruptKits));
    assert(migratedV2.support().kit_mask()==0u);
    // Route chart must report the same later-run guest as the combat director.
    bool cinnamon=false,sydney=false,jules=false;
    for(uint32_t seed=1;seed<=512&&!(cinnamon&&sydney&&jules);++seed){
        Simulation probe(seed);
        auto saveProbe=probe.snapshot();saveProbe.visited=2u;
        saveProbe.levels[0]=12u;
        saveProbe.levels[1]=12u;saveProbe.levels[3]=12u;
        saveProbe.checksum=SnapshotChecksum(saveProbe);
        assert(probe.restore(saveProbe));
        const CastGuest next=probe.route_guest(0);
        if(next!=CastGuest::CinnamonCharles&&next!=CastGuest::SydneySlipstream
           &&next!=CastGuest::JawbreakerJules)continue;
        assert(probe.launch(0)&&probe.guest()==next);
        bool zoned=false,shifting=false,heavy=false;
        const auto castLane=probe.lane(0);
        const int warmupLimit=(6+static_cast<int>(castLane.hazard)*2)/2-1;
        for(int i=0;i<820&&probe.phase()==Phase::Flight;++i){
            // Protect the sortie from its three-escape failure during the
            // early ordinary formation, then stop shooting BEFORE the
            // named encounter to observe actual live hazards and movement.
            const bool defending=probe.spawned()<warmupLimit;
            float move=next==CastGuest::SydneySlipstream?.8f:0.f;
            if(defending){
                const Enemy* closest=nullptr;
                for(const auto& e:probe.enemies())if(e.active)
                    if(!closest||e.y<closest->y)closest=&e;
                if(closest){
                    const float delta=closest->x-probe.player_x();
                    move=delta>.035f?1.f:delta<-.035f?-1.f:0.f;
                }
            }
            probe.update(.025f,move,0.f,defending);
            shifting|=probe.slipstream_active()&&
                (probe.movement_multiplier()>=1.40f||probe.movement_multiplier()<=.60f);
            for(const auto& e:probe.enemies())if(e.active&&e.kind==EnemyKind::Jules)
                heavy=true;
            for(const auto& shot:probe.shots())if(shot.active&&shot.zone)
                zoned=true;
        }
        if(next==CastGuest::CinnamonCharles){assert(zoned);cinnamon=true;}
        if(next==CastGuest::SydneySlipstream){assert(shifting);sydney=true;}
        if(next==CastGuest::JawbreakerJules){assert(heavy);jules=true;}
    }
    assert(cinnamon&&sydney&&jules);
    // All four provisional general flags must remain recoverable on load:
    // the missing King storyline is unfinished, not a permanent results trap.
    Simulation postgame(88);
    auto allGenerals=postgame.snapshot();allGenerals.bosses=15u;
    allGenerals.checksum=SnapshotChecksum(allGenerals);
    assert(postgame.restore(allGenerals)&&postgame.phase()==Phase::Chart);
    std::cout<<"Gumball Armada deterministic combat and save tests passed\n";
}
