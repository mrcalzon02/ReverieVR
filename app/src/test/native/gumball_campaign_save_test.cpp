#include "gumball_shipyard_readiness.h"
#include <cstring>
#include "gumball_campaign_save.h"
#include <cassert>
#include <cstdio>
#include <iostream>
using namespace reverie::gumball;
static CampaignSave::Bytes Bytes(const Simulation& s,const ShipAssembly& ship){
    CampaignSave::Bytes bytes{};
    if(!CampaignSave::Encode(s,ship,&bytes)){
        std::fprintf(stderr,"ENCODE_FAILED phase=%d time=%f target=%d spawned=%d defeated=%d hp=%d\n",
            static_cast<int>(s.phase()),s.elapsed(),s.target(),s.spawned(),s.defeated(),s.hp());
        assert(false);
    }
    return bytes;
}
static void Continue(Simulation& game,const ShipAssembly& ship,unsigned ticks){
    Simulation resumed;ShipAssembly rebuilt;
    auto before=Bytes(game,ship);
    assert(CampaignSave::Decode(&resumed,&rebuilt,before.data(),before.size()));
    assert(Bytes(resumed,rebuilt)==before);
    for(unsigned i=0;i<ticks;++i){
        const float x=i%61<31?.7f:-.7f,y=i%113<57?.2f:-.2f;
        const bool fire=i%7!=0;
        game.update(.025f,x,y,fire);resumed.update(.025f,x,y,fire);
        assert(Bytes(game,ship)==Bytes(resumed,rebuilt));
    }
}
static void FlySortie(Simulation& game,const ShipAssembly& ship){
    for(int i=0;i<12000&&game.phase()==Phase::Flight;++i){
        float nearest=100.f,x=game.player_x();
        for(const auto& e:game.enemies())if(e.active&&e.y<nearest){nearest=e.y;x=e.x;}
        const auto axes=FlightAxesToTarget(game.player_x(),game.player_y(),{x,-.52f,true},
                                           .035f,game.pointer_move_speed());
        game.update(.035f,axes.x,axes.y,true,true,&ship);
    }
}
// Ordinary campaign entry only. Buy upgrades with earned currency, take the
// Sweep lane, and aim at the foremost live enemy. Failed sorties can be retried.
static void CampaignFromNewGame(){
    Simulation game;
    ShipAssembly ship;
    game.configure_ship_profile(ship.profile(),ship.ready());
    int attempts=0;
    while(!game.is_rescue_route(1)&&attempts<100){
        ++attempts;
        if(!game.owns_weapon(Weapon::RockCannon))game.buy_weapon(Weapon::RockCannon);
        else if(game.level(Upgrade::Blaster)<6)game.buy(Upgrade::Blaster);
        for(const auto upgrade:{Upgrade::Thruster,Upgrade::Hull,Upgrade::Shield})game.buy(upgrade);
        assert(game.launch(0));
        const auto oldFlags=game.bosses();
        FlySortie(game,ship);
        if(game.phase()==Phase::Lost)assert(oldFlags==game.bosses());
        if(game.bosses()!=oldFlags){
            assert(game.node()%1024==1023);
            assert(game.bosses()==(1u<<game.generals_defeated())-1u);
            assert(game.phase()==(game.bosses()==15?Phase::ArmadaDefeated:Phase::Won));
            Continue(game,ship,0);
        }
        assert(game.recover());
    }
    assert(game.bosses()==15&&game.is_rescue_route(1)&&!game.is_rescue_route(0));
    const auto cash=game.cash();
    assert(game.launch(1)&&game.phase()==Phase::Rescue);
    for(int i=0;i<400&&game.phase()==Phase::Rescue;++i){
        const auto axes=FlightAxesToTarget(game.player_x(),game.player_y(),game.rescue_target(),
                                           .05f,game.pointer_move_speed());
        game.update(.05f,axes.x,axes.y,true,true,&ship);
    }
    assert(game.phase()==Phase::Homecoming&&game.king_rescued()&&game.cash()==cash);
    assert(game.recover()&&game.phase()==Phase::Credits);
    const auto saved=Bytes(game,ship);
    Simulation postgame;ShipAssembly restored;
    assert(CampaignSave::Decode(&postgame,&restored,saved.data(),saved.size()));
    assert(postgame.recover()&&postgame.phase()==Phase::Chart&&!postgame.is_rescue_route(1));
    unsigned regions=1u<<(postgame.node()/1024);
    for(unsigned choice=0;choice<3;++choice){
        const auto lane=postgame.lane(choice);
        assert(!lane.fortress&&lane.id<kLaneCount);
        regions|=1u<<(lane.id/1024);
    }
    assert(regions==15u);
    const auto destination=postgame.lane(0).id;
    assert(postgame.launch(0));
    FlySortie(postgame,restored);
    assert(postgame.phase()==Phase::Won&&postgame.node()==destination&&postgame.bosses()==15);
    assert(postgame.recover()&&postgame.king_rescued());
    Continue(postgame,restored,0);
    assert(postgame.launch(1)&&postgame.phase()==Phase::Flight&&postgame.king_rescued());
    std::printf("PASS cold start to credits: %d sorties, %u clears, %u kills, %u generals, postgame intercept open\n",
                attempts,game.visited(),game.kills(),game.generals_defeated());
}

// A rejected Chart-phase purchase is not a deferred debit or ship edit.
// The committed balance comes from an actual affordable purchase.
static void RejectUnaffordableShipyardUpgradeAtomically(){
    ShipAssembly ship;
    const auto original=ship.snapshot();
    assert(ship.edit(4,2,2,Part::Armor)==ShipEdit::Placed);
    assert(ship.ready());
    const auto fitted=ship.snapshot();
    assert(ship.stock(Part::Armor)+1u==
           original.stock[static_cast<unsigned>(Part::Armor)]);
    Simulation game(0xBAD5EEDu);
    assert(game.configure_ship_profile(ship.profile(),ship.ready()));
    assert(game.phase()==Phase::Chart);
    const uint32_t startingCash=game.cash();
    const unsigned startingNode=game.node(),startingVisits=game.visited();
    const uint32_t firstPrice=static_cast<uint32_t>(game.price(Upgrade::Hull));
    assert(firstPrice>0u&&firstPrice<=startingCash);
    assert(game.buy(Upgrade::Hull));
    const uint32_t paidCash=startingCash-firstPrice;
    assert(game.cash()==paidCash&&game.level(Upgrade::Hull)==1);
    assert(game.price(Upgrade::Hull)>static_cast<int>(paidCash));
    assert(game.price(Upgrade::Shield)>static_cast<int>(paidCash));
    const auto beforeRejected=Bytes(game,ship);
    const auto beforeCampaign=game.snapshot();
    const auto beforeShip=ship.snapshot();
    const auto beforeProfile=game.ship_profile();
    // Neither the higher level nor a different new upgrade is affordable.
    assert(!game.buy(Upgrade::Hull));
    assert(!game.buy(Upgrade::Shield));
    assert(game.cash()==paidCash&&game.level(Upgrade::Hull)==1
           &&game.level(Upgrade::Shield)==0);
    assert(game.node()==startingNode&&game.visited()==startingVisits
           &&game.phase()==Phase::Chart);
    assert(game.snapshot().checksum==beforeCampaign.checksum);
    const auto afterRejectedShip=ship.snapshot();
    assert(std::memcmp(&beforeShip,&fitted,sizeof(fitted))==0);
    assert(std::memcmp(&beforeShip,&afterRejectedShip,sizeof(beforeShip))==0);
    assert(game.ship_profile().hull==beforeProfile.hull
           &&game.ship_profile().mass==beforeProfile.mass
           &&game.ship_profile().power==beforeProfile.power
           &&game.ship_profile().demand==beforeProfile.demand);
    assert(Bytes(game,ship)==beforeRejected);
    Simulation loaded;ShipAssembly loadedShip;
    assert(CampaignSave::Decode(&loaded,&loadedShip,
                                beforeRejected.data(),beforeRejected.size()));
    assert(Bytes(loaded,loadedShip)==beforeRejected);
    const auto loadedFitted=loadedShip.snapshot();
    assert(std::memcmp(&beforeShip,&loadedFitted,sizeof(beforeShip))==0);
    assert(loaded.ship_ready()&&loaded.cash()==paidCash
           &&loaded.level(Upgrade::Hull)==1
           &&loaded.level(Upgrade::Shield)==0);
    assert(loaded.node()==startingNode&&loaded.visited()==startingVisits
           &&loaded.phase()==Phase::Chart);
    assert(!loaded.buy(Upgrade::Hull)&&!loaded.buy(Upgrade::Shield));
    assert(Bytes(loaded,loadedShip)==beforeRejected);
    assert(loadedShip.stock(Part::Armor)==ship.stock(Part::Armor));
    std::cout<<"PASS Gumball unaffordable upgrades preserve cash, stock, fitted hull, chart and reload; one valid debit only\\n";
}

static void ShipyardSortieTransaction(){
    // Shipyard armor spends finite parts stock; the Blaster is a paid upgrade.
    // Their identities must stay atomic across flight, return and reload.
    ShipAssembly fitted;
    const auto previous=fitted.snapshot();
    assert(fitted.edit(4,2,2,Part::Armor)==ShipEdit::Placed);
    const auto armored=fitted.snapshot();
    assert(armored.checksum!=previous.checksum&&fitted.ready());
    assert(fitted.stock(Part::Armor)+1u==previous.stock[static_cast<unsigned>(Part::Armor)]);
    Simulation game(0xC0FFEEu);
    assert(game.configure_ship_profile(fitted.profile(),fitted.ready()));
    const uint32_t startingCash=game.cash();
    const uint32_t price=static_cast<uint32_t>(game.price(Upgrade::Blaster));
    assert(price>0u&&startingCash>=price&&game.buy(Upgrade::Blaster));
    const uint32_t paidCash=game.cash();
    assert(paidCash==startingCash-price&&game.level(Upgrade::Blaster)==1);
    assert(game.launch(0)&&game.phase()==Phase::Flight);
    assert(!game.buy(Upgrade::Blaster)&&game.cash()==paidCash);
    for(unsigned i=0;i<80u;++i)game.update(.025f,0.f,0.f,false,false,&fitted);
    assert(game.phase()==Phase::Flight);
    const auto flight=Bytes(game,fitted);
    Simulation resumed;ShipAssembly restoredHull;
    assert(CampaignSave::Decode(&resumed,&restoredHull,flight.data(),flight.size()));
    assert(Bytes(resumed,restoredHull)==flight);
    const auto restoredShape=restoredHull.snapshot();
    assert(std::memcmp(&armored,&restoredShape,sizeof(armored))==0);
    assert(resumed.level(Upgrade::Blaster)==1&&resumed.cash()==paidCash);
    assert(resumed.ship_profile().hull==fitted.profile().hull);
    // Intentionally idle through real enemy/escape rules. A failed sortie
    // must neither refund the upgrade nor restore the pre-edit ship.
    for(unsigned i=0;i<12000u&&resumed.phase()==Phase::Flight;++i)
        resumed.update(.035f,0.f,0.f,false,false,&restoredHull);
    assert(resumed.phase()==Phase::Lost&&resumed.cash()==paidCash);
    assert(resumed.recover()&&resumed.phase()==Phase::Chart);
    const auto returned=Bytes(resumed,restoredHull);
    Simulation docked;ShipAssembly dockHull;
    assert(CampaignSave::Decode(&docked,&dockHull,returned.data(),returned.size()));
    const auto dockShape=dockHull.snapshot();
    assert(std::memcmp(&armored,&dockShape,sizeof(armored))==0);
    assert(docked.level(Upgrade::Blaster)==1&&docked.cash()==paidCash);
    assert(!docked.buy(Upgrade::Blaster));
    assert(Bytes(docked,dockHull)==returned);
    assert(docked.configure_ship_profile(dockHull.profile(),dockHull.ready()));
    assert(docked.launch(1)&&docked.ship_profile().hull==fitted.profile().hull);
    assert(docked.level(Upgrade::Blaster)==1&&docked.cash()==paidCash);
    std::cout<<"PASS Gumball paid Blaster and stocked armor survive sortie, return, reload and relaunch without duplicate debit\n";
}



// Test real editable 3D hulls against the campaign launch and save boundary.
// A readiness-only UI test cannot cover a restored unready fitted ship.
static void UnreadyShipyardCannotLaunchAfterReload(){
    for(int mode=0;mode<2;++mode){
        const bool powerDeficit=mode==0;
        ShipAssembly hull;
        Simulation game(powerDeficit?0xA101u:0xA102u);
        assert(hull.ready());
        assert(game.configure_ship_profile(hull.profile(),hull.ready()));
        const auto initialCash=game.cash();
        const auto initialNode=game.node();
        const auto initialVisits=game.visited();
        if(powerDeficit){
            // Second thruster keeps mass legal while demand exceeds supply.
            assert(hull.edit(4,2,2,Part::Thruster)==ShipEdit::Placed);
            assert(hull.edit(2,2,2,Part::Mount)==ShipEdit::Placed);
            assert(hull.edit(4,4,2,Part::Cargo)==ShipEdit::Placed);
            assert(hull.edit(2,4,2,Part::Cargo)==ShipEdit::Placed);
            assert(hull.profile().power<hull.profile().demand);
            assert(hull.profile().mass<=18*hull.count(Part::Thruster));
            assert(std::strcmp(FlightReadiness(hull),
                "POWER DEFICIT / ADD BATTERY OR REACTOR")==0);
        }else{
            // The original reactor is sufficient; one thruster is not.
            assert(hull.edit(4,2,2,Part::Armor)==ShipEdit::Placed);
            assert(hull.edit(4,4,2,Part::Armor)==ShipEdit::Placed);
            assert(hull.edit(2,2,2,Part::Armor)==ShipEdit::Placed);
            assert(hull.profile().power>=hull.profile().demand);
            assert(hull.profile().mass>18*hull.count(Part::Thruster));
            assert(std::strcmp(FlightReadiness(hull),
                "OVERWEIGHT / ADD THRUSTER OR REMOVE PARTS")==0);
        }
        assert(hull.connected()&&!hull.ready());
        const Ship3DSave edited=hull.snapshot();
        assert(!game.configure_ship_profile(hull.profile(),hull.ready()));
        assert(!game.ship_ready());
        const auto blocked=Bytes(game,hull);
        for(unsigned choice=0;choice<3;++choice)
            assert(!game.launch(choice));
        assert(game.phase()==Phase::Chart&&game.cash()==initialCash
               &&game.node()==initialNode&&game.visited()==initialVisits);
        assert(Bytes(game,hull)==blocked);
        const auto stillEdited=hull.snapshot();
        assert(std::memcmp(&edited,&stillEdited,sizeof(edited))==0);

        Simulation loaded;ShipAssembly restored;
        assert(CampaignSave::Decode(&loaded,&restored,blocked.data(),blocked.size()));
        assert(Bytes(loaded,restored)==blocked);
        assert(!loaded.ship_ready()&&!restored.ready());
        const auto restoredSnapshot=restored.snapshot();
        assert(std::memcmp(&edited,&restoredSnapshot,sizeof(edited))==0);
        for(unsigned choice=0;choice<3;++choice)
            assert(!loaded.launch(choice));
        assert(Bytes(loaded,restored)==blocked);
        assert(loaded.cash()==initialCash&&loaded.node()==initialNode
               &&loaded.visited()==initialVisits&&loaded.phase()==Phase::Chart);

        // Editor repairs refund their own stock; rejected launches spent none.
        if(powerDeficit){
            assert(restored.edit(2,2,2,Part::Mount)==ShipEdit::Removed);
            assert(restored.edit(2,4,2,Part::Cargo)==ShipEdit::Removed);
        }else{
            assert(restored.edit(2,2,2,Part::Armor)==ShipEdit::Removed);
            assert(restored.edit(4,4,2,Part::Armor)==ShipEdit::Removed);
        }
        assert(restored.ready());
        assert(loaded.configure_ship_profile(restored.profile(),restored.ready()));
        const auto repaired=Bytes(loaded,restored);
        assert(repaired!=blocked);
        assert(loaded.launch(0)&&loaded.phase()==Phase::Flight);
        assert(!loaded.launch(0)&&!loaded.launch(1));
        assert(loaded.cash()==initialCash&&loaded.node()==initialNode
               &&loaded.visited()==initialVisits);
        const auto flying=Bytes(loaded,restored);
        Simulation inFlight;ShipAssembly flownHull;
        assert(CampaignSave::Decode(&inFlight,&flownHull,flying.data(),flying.size()));
        assert(Bytes(inFlight,flownHull)==flying);
        assert(inFlight.phase()==Phase::Flight&&!inFlight.launch(0));
    }
    std::cout<<"PASS Gumball unpowered/overweight 3D hulls cannot launch before or after reload; editor repair launches once\n";
}

static void FatalHostileHitCheckpoint(){
    // The flight-to-loss transition must itself be a valid atomic checkpoint.
    // Prior tests cover 3D hostile contact and ordinary sortie saves separately;
    // this crosses those two boundaries without a synthetic damage snapshot.
    ShipAssembly ship;
    Simulation game(81);
    assert(game.configure_ship_profile(ship.profile(),ship.ready()));
    assert(game.launch(0)&&game.phase()==Phase::Flight);
    const int startingHp=game.hp(),startingShield=game.shield();
    const uint32_t startingCash=game.cash(),startingKills=game.kills();
    const uint32_t startingVisited=game.visited();
    const ShipVolumePoint center{game.player_x(),game.player_y(),-2.55f};
    const ShipVolumePoint nearA{center.x,center.y-.30f,center.z};
    const ShipVolumePoint nearB{center.x,center.y+.30f,center.z};
    const ShipVolumePoint farA{center.x,center.y-.30f,center.z-.80f};
    const ShipVolumePoint farB{center.x,center.y+.30f,center.z-.80f};
    const auto pristineFlight=Bytes(game,ship);
    assert(!game.receive_hostile_segment(ship,farA,farB,500));
    assert(Bytes(game,ship)==pristineFlight);
    assert(game.hp()==startingHp&&game.shield()==startingShield);
    assert(game.receive_hostile_segment(ship,nearA,nearB,500));
    assert(game.phase()==Phase::Lost&&game.failure()==FailureReason::HullDestroyed);
    assert(game.hp()==0&&game.shield()==0);
    const auto lostCheckpoint=Bytes(game,ship);
    Simulation restored;ShipAssembly restoredShip;
    assert(CampaignSave::Decode(&restored,&restoredShip,
                                lostCheckpoint.data(),lostCheckpoint.size()));
    assert(Bytes(restored,restoredShip)==lostCheckpoint);
    assert(restored.phase()==Phase::Lost
           &&restored.failure()==FailureReason::HullDestroyed);
    assert(restored.hp()==0&&restored.shield()==0);
    // No delayed secondary impact, award, flight tick or relaunch after reload.
    assert(!restored.receive_hostile_segment(restoredShip,nearA,nearB,500));
    assert(!restored.launch(0));
    restored.update(.05f,0.f,0.f,true);
    assert(Bytes(restored,restoredShip)==lostCheckpoint);
    assert(restored.cash()==startingCash&&restored.kills()==startingKills);
    assert(restored.visited()==startingVisited);
    assert(restored.recover()&&restored.phase()==Phase::Chart);
    assert(!restored.recover());
    const auto recoveredCheckpoint=Bytes(restored,restoredShip);
    Simulation returned;ShipAssembly returnedShip;
    assert(CampaignSave::Decode(&returned,&returnedShip,
                                recoveredCheckpoint.data(),recoveredCheckpoint.size()));
    assert(Bytes(returned,returnedShip)==recoveredCheckpoint);
    assert(returned.phase()==Phase::Chart);
    assert(returned.cash()==startingCash&&returned.kills()==startingKills);
    assert(returned.visited()==startingVisited);
    assert(!returned.recover());
    assert(returned.launch(0)&&returned.phase()==Phase::Flight);
    assert(returned.hp()==startingHp&&returned.shield()==startingShield);
    assert(returned.failure()==FailureReason::None);
    std::cout<<"PASS Gumball lethal real 3D hostile hit saves in Lost, rejects postmortem effects and recovers exactly once after reload\\n";
}

int main(){
    RejectUnaffordableShipyardUpgradeAtomically();
    UnreadyShipyardCannotLaunchAfterReload();
    FatalHostileHitCheckpoint();
    ShipyardSortieTransaction();
    CampaignFromNewGame();
    ShipAssembly ship;
    assert(ship.edit(4,2,2,Part::Armor)==ShipEdit::Placed);
    Simulation starter(109);assert(starter.configure_ship_profile(ship.profile(),ship.ready()));
    assert(starter.hire_support(SupportKind::CapitaineEclair));
    Continue(starter,ship,0);
    assert(starter.launch(0));
    for(int i=0;i<240;++i)starter.update(.025f,0.f,0.f,false);
    assert(starter.phase()==Phase::Flight&&starter.support_pulses()>0);
    const auto active=Bytes(starter,ship);
    Continue(starter,ship,1800);
    assert(starter.phase()!=Phase::Flight);
    const auto cash=starter.cash();Continue(starter,ship,100);
    assert(starter.cash()==cash); // Result reload cannot award again.
    assert(starter.recover());Continue(starter,ship,0);
    // An actual pilot-shot volume hit must survive a full flight checkpoint,
    // not merely a flat XY test or a pre-combat save round trip.
    Simulation volumeHit(4);
    auto trained=volumeHit.snapshot();
    for(auto& level:trained.levels)level=12;
    trained.checksum=SnapshotChecksum(trained);
    assert(volumeHit.restore(trained));
    assert(volumeHit.configure_ship_profile(ship.profile(),ship.ready()));
    assert(volumeHit.launch(0));
    volumeHit.update(.025f,0.f,0.f,false);
    // Negative control: same XY sweep beside a live generated enemy, but
    // 0.8 world units away in Z, must not reduce that physical hull's HP.
    const Enemy liveEnemy=volumeHit.enemies()[0];
    assert(liveEnemy.active);
    Enemy wrongDepth=liveEnemy;
    const ShipVolumePoint at=EnemyWorldPoint(wrongDepth);
    assert(!DamageEnemyVolume(wrongDepth,volumeHit.node(),
        {at.x,at.y-.30f,at.z+.80f},
        {at.x,at.y+.30f,at.z+.80f},2));
    assert(wrongDepth.hp==liveEnemy.hp);
    const uint32_t hitsBefore=volumeHit.shots_hit();
    for(unsigned tick=0;tick<12000&&volumeHit.phase()==Phase::Flight
        &&volumeHit.shots_hit()==hitsBefore;++tick){
        float nearest=100.f,targetX=volumeHit.player_x();
        for(const auto& enemy:volumeHit.enemies())if(enemy.active){
            const float distance=std::fabs(enemy.y-volumeHit.player_y());
            if(distance<nearest){nearest=distance;targetX=enemy.x;}
        }
        const float steering=targetX>volumeHit.player_x()+.025f?1.f:
            (targetX<volumeHit.player_x()-.025f?-1.f:0.f);
        volumeHit.update(.025f,steering,0.f,true);
    }
    assert(volumeHit.phase()==Phase::Flight);
    assert(volumeHit.shots_hit()>hitsBefore);
    assert(volumeHit.shots_fired()>=volumeHit.shots_hit());
    const auto hitCheckpoint=Bytes(volumeHit,ship);
    Simulation loadedHit;ShipAssembly loadedHull;
    assert(CampaignSave::Decode(&loadedHit,&loadedHull,
                                hitCheckpoint.data(),hitCheckpoint.size()));
    assert(loadedHit.shots_hit()==volumeHit.shots_hit());
    assert(loadedHit.defeated()==volumeHit.defeated());
    assert(loadedHit.kills()==volumeHit.kills());
    assert(loadedHit.cash()==volumeHit.cash());
    assert(Bytes(loadedHit,loadedHull)==hitCheckpoint);
    Continue(volumeHit,ship,120); // subsequent physical combat stays bit-exact
    // Repeated snapshots include all named guest and mission cases, varied
    // RNG, salvage, shots and timers; identical future input must stay exact.
    unsigned guests=0;
    for(unsigned seed=1;seed<=36;++seed){
        Simulation game(seed);auto progress=game.snapshot();
        progress.visited=3;progress.cash=5000;progress.levels[1]=8;progress.levels[3]=7;
        if(seed%7==0)progress.node=1022;
        progress.checksum=SnapshotChecksum(progress);assert(game.restore(progress));
        assert(game.configure_ship_profile(ship.profile(),ship.ready()));
        assert(game.hire_support(seed%2?SupportKind::CapitaineEclair:SupportKind::MademoiselleMacaron));
        assert(game.buy_support_kit(SupportKit::HullPatch));
        assert(game.buy_support_kit(SupportKit::ShieldTonic));
        const unsigned route=seed%3;
        guests|=1u<<static_cast<unsigned>(ChooseCastGuest(seed,game.lane(route).id,3,game.lane(route).hazard));
        assert(game.launch(route));
        for(unsigned i=0;i<seed*5;++i)game.update(.025f,.35f,0.f,i%3!=0);
        Continue(game,ship,800);
    }
    assert(guests==511u);
    Simulation unchanged(77);ShipAssembly unchangedShip;
    const auto unchangedBytes=Bytes(unchanged,unchangedShip);
    for(size_t i=0;i<active.size();++i){
        auto broken=active;broken[i]^=1u;
        assert(!CampaignSave::Decode(&unchanged,&unchangedShip,broken.data(),broken.size()));
        assert(Bytes(unchanged,unchangedShip)==unchangedBytes);
    }
    assert(!CampaignSave::Decode(&unchanged,&unchangedShip,active.data(),active.size()-1));
    // A correctly rehashed impossible phase must also fail semantic checks.
    auto invalid=active;invalid[12]=255;
    uint64_t hash=14695981039346656037ull;
    for(size_t i=0;i<invalid.size()-8;++i)hash=(hash^invalid[i])*1099511628211ull;
    for(size_t i=invalid.size()-8;i<invalid.size();++i){invalid[i]=static_cast<uint8_t>(hash);hash>>=8;}
    assert(!CampaignSave::Decode(&unchanged,&unchangedShip,invalid.data(),invalid.size()));
    auto invalidFloat=unchangedBytes;
    // Mission time limit is binary32 at offset 58, including a fresh chart.
    invalidFloat[58]=0;invalidFloat[59]=0;invalidFloat[60]=0xc0;invalidFloat[61]=0x7f;
    hash=14695981039346656037ull;
    for(size_t i=0;i<invalidFloat.size()-8;++i)hash=(hash^invalidFloat[i])*1099511628211ull;
    for(size_t i=invalidFloat.size()-8;i<invalidFloat.size();++i){invalidFloat[i]=static_cast<uint8_t>(hash);hash>>=8;}
    assert(!CampaignSave::Decode(&unchanged,&unchangedShip,invalidFloat.data(),invalidFloat.size()));
    std::cout<<"PASS Gumball full live sortie/ship/checkpoint next-state determinism, all nine guests, paid support, corrupt transactional rejection\n";
}
