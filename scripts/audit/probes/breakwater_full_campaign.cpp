// Replay from a fresh slot. Game state is read only to steer the virtual
// controller and check the result. All purchases, shots and waves use Update.
#include "audit_host.h"
#include "breakwater_module.cpp"

namespace {
// Optional controller recording for the Android instrumentation test. Only
// the actions and expected readback are stored. No save is supplied to Android.
std::ofstream trace;
void Step(State *s,ReverieNativeInputV1 input) {
    if(trace.is_open()) {
        trace.put((input.primary_down?1:0)|(input.secondary_down?2:0)|
                  (input.delta_seconds>.075f?4:0));
        for(float component:input.pointer_direction) {
            uint32_t bits;
            std::memcpy(&bits,&component,sizeof(bits));
            for(int shift=24;shift>=0;shift-=8)trace.put(char(bits>>shift));
        }
    }
    // The real Java/JNI host normalizes the ray once before module Update.
    ReverieNativeSanitizePointerV1(&input);
    Update(s,&input);
}
void Click(State *s,menu::Rect r) {
    auto input=audit::Pointer(r.x,r.y,kShopZ);
    Step(s,input);
    input.primary_down=1;Step(s,input);
    input.primary_down=0;Step(s,input);
}
void Buy(State *s,Upgrade u) {
    const int cost=s->campaign.price(u);
    if(cost>=0 && cost<=s->campaign.credits())Click(s,menu::ShopCard(static_cast<int>(u)));
}
void Shop(State *s,bool use_reserves) {
    if(s->round_summary)Click(s,menu::DebriefAction(0));
    if(s->campaign.integrity()<90 || use_reserves)Buy(s,Upgrade::Repair);
    Buy(s,Upgrade::SupportTurret);
    Buy(s,Upgrade::Damage);
    Buy(s,Upgrade::FireRate);
    Buy(s,Upgrade::AntiAir);
    Buy(s,Upgrade::Magazine);
    Buy(s,Upgrade::Airstrike);
    if(use_reserves) {
        if(!s->campaign.owns_weapon(WeaponKind::DirectCannon)) {
            Click(s,menu::ShopAction(9));
            Click(s,menu::ShopCard(int(WeaponKind::DirectCannon)));
            Click(s,menu::ShopCard(int(WeaponKind::MachineGun)));
            Click(s,menu::ShopAction(9));
        }
        for(int n=0;n<3;n++) {
            Buy(s,Upgrade::Airstrike);
            Buy(s,Upgrade::Artillery);
        }
    }
    Click(s,menu::ShopAction(8));
    audit::Check(s->campaign.phase()==Phase::Combat,"wave did not start");
}
ReverieNativeInputV1 Play(State *s,unsigned tick,bool use_reserves,unsigned attempt) {
    const Campaign &g=s->campaign;
    const auto st=defense::At(g.location(),s->defense_points.current);
    auto input=audit::Pointer(0.f,0.f,-1.f);
    unsigned active=0;
    float best=-1.e5f;
    const Enemy* target=nullptr;
    for(const auto &e:g.enemies()) {
        if(!e.active)continue;
        ++active;
        float priority=e.z;
        if(attempt%3==1 && e.type==UnitType::ArmoredTransport)priority+=2.f;
        if(attempt%3==2 && e.type==UnitType::Aircraft)priority+=2.f;
        if(use_reserves && e.type==UnitType::Aircraft) {
            if(e.z>-5.f)continue;
            priority+=1.f;
        }
        if(priority>best){best=priority;target=&e;}
    }
    if(target) {
        const auto kind=use_reserves && target->type==UnitType::ArmoredTransport?
            WeaponKind::DirectCannon:WeaponKind::MachineGun;
        if(kind!=g.selected_weapon() && g.owns_weapon(kind) && !g.reloading()) {
            float x=-2.4f+int(kind)*1.2f,y=-1.44f,z=-1.8f;
            console::Point(x,y,z);
            input=audit::Pointer(x,y,z);input.primary_down=tick%2;
            return input;
        }
        auto p=battlefield::Vessel(*target);
        if(use_reserves && target->type==UnitType::Aircraft) {
            const float next_z=target->z+.05f*(target->aircraft_pattern==AircraftPattern::Bomber?1.05f:1.55f);
            p.wz=battlefield::Z(next_z);
            p.wx=battlefield::X(AircraftFlightAt(target->aircraft_pattern,
                target->flight_center_x,target->flight_sign,next_z).x);
        }
        const float drop=battlefield::Gravity(g.stats().projectile_gravity)*p.wz*p.wz;
        input=audit::Pointer(p.wx-st.x,p.wy-st.y+drop+(target->type==UnitType::Aircraft?0.f:2.f),p.wz-st.z);
        input.primary_down=(!use_reserves || tick%4!=0);
    }
    if(attempt && !target && g.infantry()>0) {
        for(const auto &trooper:g.infantry_units())if(trooper.active) {
            auto p=battlefield::Land(g.location(),trooper.x,trooper.z);
            if(g.selected_weapon()!=WeaponKind::MachineGun) {
                float x=-2.4f,y=-1.44f,z=-1.8f;
                console::Point(x,y,z);input=audit::Pointer(x,y,z);
                input.primary_down=tick%2;return input;
            }
            input=audit::Pointer(p.wx,p.wy+.93f,p.wz);input.primary_down=tick%4!=0;
            return input;
        }
    }
    if(!target && !g.reloading() && g.magazine_left()<g.stats().magazine)input.secondary_down=1;
    const unsigned air_threshold=attempt?5+attempt%4:6;
    const unsigned support_threshold=attempt?4+attempt%4:5;
    const bool air=g.airstrike_charges()>0 &&
        ((!attempt && g.infantry()>0) || g.incoming_threats().armor_seconds>=0.f ||
         (use_reserves && active>=air_threshold));
    const bool support=use_reserves && !air && g.artillery_charges()>0 && active>=support_threshold;
    if(air || support) {
        float x=air?2.f:-2.f,y=.97f,z=-1.8f;console::Point(x,y,z);
        input=audit::Pointer(x,y,z);input.primary_down=tick%2;
    }
    input.delta_seconds=use_reserves?.05f:.10f;
    return input;
}
void Photograph(State *s,const std::string &name) {
    if(audit::output.empty())return;
    PrepareFrame(s);
    audit::Capture([](void *state,ReverieNativeEyeV1 *eye) {
        constexpr float near=.08f,far=battlefield::kFarClip;
        eye->projection[10]=-(far+near)/(far-near);eye->projection[14]=-2.f*far*near/(far-near);
        return Draw(state,eye);
    },s,name);
}
}
int main(int argc,char**argv) {
    unsigned tier=0;
    if(argc>=3 && std::string(argv[1])=="--tier") {
        const std::string choice=argv[2];
        audit::Check(choice.size()==1 && choice[0]>='0' && choice[0]<='3',"expected --tier 0, 1, 2 or 3");
        tier=unsigned(choice[0]-'0');argc-=2;argv+=2;
    }
    const unsigned expected_waves=366+44*tier;
    audit::output=argc>1?argv[1]:"";
    if(argc>2) {
        trace.open(argv[2],std::ios::binary);
        audit::Check(bool(trace),"open controller recording");
        trace.write("BWC1",4);
    }
    audit::GL gl;
    auto host=audit::Host();
    auto* s=static_cast<State*>(Create(&host));
    audit::Check(s && GlCreate(s),"module launch");
    Click(s,menu::SlotCard(0));Click(s,menu::DifficultyCard(tier));
    unsigned waves=0,ticks=0,boats=0,air=0,troops=0,retries=0;
    unsigned attempt=1;
    while(s->campaign.phase()==Phase::Shop && waves<500) {
        const unsigned map=s->campaign.location();
        const bool use_reserves=tier>0 || retries>0;
        Shop(s,use_reserves);
        unsigned waveTicks=0;
        while(s->campaign.phase()==Phase::Combat && waveTicks<2500) {
            auto input=Play(s,waveTicks++,use_reserves,tier>=2?attempt:0);
            Step(s,input);
            if(waveTicks==60 && s->campaign.day()==1 && s->campaign.period()==Period::Day && s->campaign.wave_index()==1)
                Photograph(s,"breakwater-theater-"+std::to_string(map+1));
        }
        ticks+=waveTicks;
        if(s->campaign.phase()==Phase::Defeat) {
            Photograph(s,"breakwater-defeat");
            if(trace.is_open())trace.put(18);
            ++retries;
            audit::Check(++attempt<=15,"repeated campaign loss");
            const auto disk=audit::saves.at(CheckpointSlot(0));
            Campaign before;
            audit::Check(before.DeserializeCheckpoint(disk.data(),disk.size()),"pre-wave checkpoint invalid");
            Click(s,menu::DebriefAction(0));
            audit::Check(s->campaign.phase()==Phase::Shop && s->campaign.location()==before.location() &&
                s->campaign.day()==before.day() && s->campaign.period()==before.period() &&
                s->campaign.wave_index()==before.wave_index() && s->campaign.credits()==before.credits() &&
                s->campaign.integrity()==before.integrity() && audit::saves.at(CheckpointSlot(0))==disk,
                "retry must restore the exact earned wave");
            std::printf("RETRY wave=%u map=%u day=%u period=%u localWave=%u credits=%d hull=%d\n",
                waves+1,map+1,unsigned(before.day()),unsigned(before.period()),unsigned(before.wave_index()),
                before.credits(),before.integrity());
            continue;
        }
        attempt=1;
        auto score=s->campaign.wave_status();
        boats+=score.boats_sunk;air+=score.aircraft_destroyed;troops+=score.troops_stopped;
        std::printf("WAVE %u map=%u phase=%u ticks=%u hull=%d credits=%d ships=%u air=%u troops=%u landed=%u\n",
            ++waves,map+1,unsigned(s->campaign.phase()),waveTicks,s->campaign.integrity(),s->campaign.credits(),
            score.boats_sunk,score.aircraft_destroyed,score.troops_stopped,score.troops_landed);
        audit::Check(waveTicks<2500,"wave stalled");
        if(trace.is_open()) {
            trace.put(16);
            trace.put(s->campaign.location());trace.put(s->campaign.day());
            trace.put(int(s->campaign.period()));trace.put(s->campaign.wave_index());
            trace.put(s->campaign.phase()==Phase::Victory?1:0);
        }
        if(s->campaign.location()!=map) {
            Photograph(s,"breakwater-transition-"+std::to_string(map+1));
            const auto disk=audit::saves.at(CheckpointSlot(0));
            if(trace.is_open())trace.put(17);
            GlRelease(s);Destroy(s);
            s=static_cast<State*>(Create(&host));
            audit::Check(s && GlCreate(s),"module reopen");
            Click(s,menu::SlotCard(0));
            audit::Check(s->campaign.phase()==Phase::Shop && s->campaign.location()==map+1 &&
                s->campaign.day()==1 && s->campaign.wave_index()==1 &&
                audit::saves.at(CheckpointSlot(0))==disk,"theater checkpoint reopen");
        }
    }
    Photograph(s,"breakwater-ending");
    std::printf("FINISH phase=%u waves=%u ticks=%u ships=%u air=%u troops=%u\n",
                unsigned(s->campaign.phase()),waves,ticks,boats,air,troops);
    audit::Check(s->campaign.phase()==Phase::Victory && waves==expected_waves,"campaign did not reach earned victory");
    const auto victory=audit::saves.at(CheckpointSlot(0));
    if(trace.is_open()) {trace.put(19);trace.put(17);}
    GlRelease(s);Destroy(s);
    s=static_cast<State*>(Create(&host));
    audit::Check(s && GlCreate(s),"terminal module reopen");
    Click(s,menu::SlotCard(0));
    audit::Check(s->campaign.phase()==Phase::Victory,"reopened terminal save");
    Click(s,menu::DebriefAction(0));
    audit::Check(s->slot_picker,"return to slots");
    Click(s,menu::SlotCard(1));
    audit::Check(s->campaign.phase()==Phase::Briefing && audit::saves.at(CheckpointSlot(0))==victory,
                 "new slot must leave the victory intact");
    GlRelease(s);Destroy(s);
    if(trace.is_open()) {trace.put(20);trace.close();audit::Check(bool(trace),"write controller recording");}
    std::printf("PASS 4 theaters, %u waves, %u in-game retries, 3 theater reloads and preserved victory\n",waves,retries);
}
