#include "breakwater_campaign.h"
#include <cassert>
#include <iostream>
using namespace reverie::breakwater;

int main() {
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
    assert(firing.FireMiss());
    assert(firing.magazine_left()==full_mag-1);
    assert(!firing.FireMiss());
    while(firing.magazine_left()>0) {
        for(int i=0;i<7;i++) firing.Tick(0.05f);
        assert(firing.FireMiss());
    }
    assert(!firing.FireMiss());
    for(int i=0;i<24;i++) firing.Tick(0.05f);
    assert(firing.magazine_left()==firing.stats().magazine);

    Campaign landed(Difficulty::Casual,41);
    assert(landed.Start() && landed.BeginWave());
    for(int i=0;i<4500 && landed.wave_status().troops_landed==0;i++)
        landed.Tick(0.05f);
    assert(landed.wave_status().troops_landed>0);
    assert(landed.infantry()>0);
    const int32_t integrity=landed.integrity();
    for(int i=0;i<82;i++) landed.Tick(0.05f);
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
    assert(hitDepth>0.9f && hitDepth<1.1f);
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
    assert(veteran.CallArtillery());
    assert(!veteran.CallArtillery());
    assert(!veteran.Purchase(Upgrade::Damage));

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
    for(int i=0;i<140;i++) stopped_ship.Tick(0.05f);
    assert(stopped_ship.wave_status().gunboat_attacks==0);
    assert(stopped_ship.integrity()==100);

    auto stage_blackcap_night=[](Campaign &game) {
        uint8_t bytes[Campaign::kSerializedSize]={};
        assert(game.SerializeShop(bytes,sizeof(bytes)));
        bytes[16]=2; // Blackcap Island
        bytes[20]=4; // final day
        bytes[24]=1; // night
        bytes[28]=1; // first wave in period
        assert(game.DeserializeShop(bytes,sizeof(bytes)));
    };

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
    const int16_t bomber_hp=bomber.enemies()[0].hp;
    const uint8_t bomber_mag=bomber.magazine_left();
    assert(bomber.FireAt(0));
    assert(bomber.magazine_left()==bomber_mag-1);
    assert(bomber.enemies()[0].hp==bomber_hp);
    for(int i=0;i<140 && bomber.wave_status().aircraft_attacks==0;i++)
        bomber.Tick(0.05f);
    assert(bomber.wave_status().aircraft_attacks==1);
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
