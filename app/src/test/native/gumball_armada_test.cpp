#include "gumball_armada_simulation.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace reverie::gumball;
int main(){
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
    Shipyard brokenShip;
    assert(brokenShip.remove(3,1));
    assert(!brokenShip.ready());
    Simulation blocked(55);
    assert(!blocked.configure_ship(brokenShip));
    assert(!blocked.launch(0));
    assert(brokenShip.place(3,1,Part::Thruster));
    assert(blocked.configure_ship(brokenShip));
    assert(blocked.launch(0));
    std::cout<<"Gumball Armada deterministic combat and save tests passed\n";
}
