#ifndef REVERIE_OMEGA_FRONT_CAMPAIGN_SAVE_H
#define REVERIE_OMEGA_FRONT_CAMPAIGN_SAVE_H

#include "omega_front_checkpoint.h"
#include "omega_front_session.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace reverie { namespace omega {

// One versioned, padding-free snapshot for the *whole first strategic slice*.
// An earlier standalone battle slot is deliberately never treated as a
// campaign save; it has no owner, supplies, deployment, or consequence history.
class CampaignSave {
public:
    static constexpr size_t kBytes=2048;
    using Bytes=std::array<uint8_t,kBytes>;
    static bool Encode(const Session &s,Bytes *out){
        if(!out)return false;
        Bytes bytes{};
        Writer w{bytes};
        for(char c : "OFSTRAT1") {if(!c)break;w.U8(static_cast<uint8_t>(c));}
        w.U16(5);w.U16(0);
        const Frontier &f=s.frontier;
        w.U32(f.seed_);
        w.U8(static_cast<uint8_t>(s.focused));
        w.U8(static_cast<uint8_t>(s.view));
        w.U8(static_cast<uint8_t>(s.action));
        w.U32(s.orders);
        w.U8(static_cast<uint8_t>(f.location_));
        w.U32(static_cast<uint32_t>(f.day_));
        w.U8(static_cast<uint8_t>(f.operation_));
        w.U16(static_cast<uint16_t>(f.population_));
        w.U8(static_cast<uint8_t>(f.guards_));
        w.U16(static_cast<uint16_t>(f.rations_));
        w.U32(static_cast<uint32_t>(f.salvage_));
        w.U8(static_cast<uint8_t>(f.trust_));
        w.U8(static_cast<uint8_t>(f.pump_health_));
        w.U8(f.bridge_?1:0);
        w.U8(f.deployed_?1:0);
        w.U32(f.morphology_epoch_);
        w.U8(static_cast<uint8_t>(f.last_.operation+1));
        w.U8(static_cast<uint8_t>(f.last_.outcome));
        w.U8(f.last_.defenders_lost);
        w.U8(f.last_.enemies_lost);
        w.U8(f.last_.pump_remaining);
        w.U32(f.last_.battle_ticks);
        for(const auto &sector:f.sectors_){
            w.U8(static_cast<uint8_t>(sector.ecology));
            w.U8(static_cast<uint8_t>(sector.previous));
            w.U8(sector.owner);w.U8(sector.anomaly);w.U8(sector.stability);
            w.U16(sector.residents);w.U16(sector.food);w.U16(sector.water);
            w.U16(sector.wreck_mass);w.U8(sector.claims);
        }
        Checkpoint::Bytes combat{};
        if(!Checkpoint::Encode(s.tactical,&combat))return false;
        for(uint8_t c:combat)w.U8(c);
        for(int faction=1;faction<=kMajorFactions;++faction)
            w.U8(static_cast<uint8_t>(f.relations_[faction]));
        for(auto level:f.intel_.levels())w.U8(level);
        for(auto talk:f.intel_.talks())w.U8(talk);
        w.U8(f.accords_.mask());
        w.U8(static_cast<uint8_t>(f.accords_.last_operation()+1));
        for(auto stage:f.obligations_.stages())w.U8(stage);
        for(auto day:f.obligations_.days())w.U16(day);
        if(!w.ok)return false;
        uint64_t hash=Hash(bytes.data(),kBytes-8);
        for(size_t i=kBytes-8;i<kBytes;++i){bytes[i]=static_cast<uint8_t>(hash);hash>>=8;}
        *out=bytes;
        return true;
    }
    static bool Decode(Session *output,const uint8_t *data,size_t length){
        if(!output||!data||length!=kBytes)return false;
        uint64_t hash=Hash(data,kBytes-8);
        for(size_t i=kBytes-8;i<kBytes;++i){
            if(data[i]!=static_cast<uint8_t>(hash))return false;
            hash>>=8;
        }
        Reader r{data,kBytes-8};
        for(char c:"OFSTRAT1"){if(!c)break;if(r.U8()!=static_cast<uint8_t>(c))return false;}
        const uint16_t schema=r.U16();
        if((schema!=1 && schema!=2 && schema!=3 && schema!=4 && schema!=5)||r.U16()!=0)return false;
        uint32_t seed=r.U32();
        const int focus=r.U8();
        const unsigned view=r.U8(),action=r.U8();
        const uint32_t orders=r.U32();
        if(!seed||focus>=16||view>1||action>(schema>=4?7u:schema>=3?6u:4u))return false;
        Session next(seed);
        next.focused=focus;
        next.view=static_cast<FrontMode>(view);
        next.action=static_cast<FieldAction>(action);
        next.orders=orders;
        Frontier &f=next.frontier;
        f.location_=r.U8();f.day_=static_cast<int>(r.U32());
        f.operation_=r.U8();f.population_=r.U16();f.guards_=r.U8();
        f.rations_=r.U16();f.salvage_=static_cast<int>(r.U32());
        f.trust_=r.U8();f.pump_health_=r.U8();
        const uint8_t bridge=r.U8(),deployed=r.U8();
        f.bridge_=bridge!=0;f.deployed_=deployed!=0;
        f.morphology_epoch_=r.U32();
        f.last_.operation=static_cast<int>(r.U8())-1;
        const unsigned outcome=r.U8();
        f.last_.outcome=static_cast<Result>(outcome);
        f.last_.defenders_lost=r.U8();f.last_.enemies_lost=r.U8();
        f.last_.pump_remaining=r.U8();f.last_.battle_ticks=r.U32();
        if(!Frontier::Valid(f.location_)||f.day_<1||f.day_>1000000||
           f.operation_<0||f.operation_>kHearthfallOperations||
           (schema==1 && f.operation_>3)||f.population_<1||f.population_>1000||
           f.guards_<0||f.guards_>12||f.rations_<0||f.rations_>65535||
           f.salvage_<0||f.salvage_>1000000||f.trust_<0||f.trust_>100||
           f.pump_health_<0||f.pump_health_>18||bridge>1||deployed>1||
           f.morphology_epoch_>1||outcome>2||
           f.last_.operation<-1||f.last_.operation>=kHearthfallOperations||
           f.last_.operation>f.operation_||
           f.last_.defenders_lost>4||f.last_.enemies_lost>6||
           f.last_.pump_remaining>18||f.last_.battle_ticks>160||
           (f.last_.operation==-1 && (outcome || f.last_.battle_ticks))||
           (f.operation_==kHearthfallOperations && f.deployed_)||
           (f.deployed_ != (next.view==FrontMode::Tactical)))return false;
        for(auto &sector:f.sectors_){
            const unsigned ecology=r.U8(),prev=r.U8(),owner=r.U8(),anomaly=r.U8(),stability=r.U8();
            sector.residents=r.U16();sector.food=r.U16();sector.water=r.U16();
            sector.wreck_mass=r.U16();sector.claims=r.U8();
            if(ecology>2||prev>2||owner>kIndependentOwner||anomaly>1||
               stability>100||sector.residents>20000||sector.claims>100)return false;
            sector.ecology=static_cast<Ecology>(ecology);
            sector.previous=static_cast<Ecology>(prev);
            sector.owner=static_cast<uint8_t>(owner);
            sector.anomaly=static_cast<uint8_t>(anomaly);
            sector.stability=static_cast<uint8_t>(stability);
        }
        // First scripted emergence has exactly one known transition.
        if((f.morphology_epoch_==0 && (f.day_>=4||f.sectors_[8].ecology!=Ecology::Wasteland)) ||
           (f.morphology_epoch_==1 && (f.day_<4||f.sectors_[8].ecology!=Ecology::Eden)))return false;
        Checkpoint::Bytes combat{};
        for(auto &c:combat)c=r.U8();
        if(!Checkpoint::Decode(&next.tactical,combat.data(),combat.size()))return false;
        if(schema>=2){
            for(int faction=1;faction<=kMajorFactions;++faction){
                const int value=static_cast<int8_t>(r.U8());
                if(value<-100||value>100)return false;
                f.relations_[faction]=static_cast<int8_t>(value);
            }
        }
        if(schema>=3){
            std::array<uint8_t,kFrontierSectors> levels{},talks{};
            for(auto &level:levels)level=r.U8();
            for(auto &talk:talks)talk=r.U8();
            if(!f.intel_.Restore(levels,talks,f.location_))return false;
        }else{
            // Legacy saves lack recon history. Only observe the player's
            // actual position, not an invented explored frontier.
            f.intel_.ObserveArrival(f.location_);
        }
        if(schema>=4){
            const uint8_t mask=r.U8();
            const int lastAccord=static_cast<int>(r.U8())-1;
            if(!f.accords_.Restore(mask,lastAccord,f.operation_))return false;
            for(int op=3;op<=5;++op){
                if(!f.accords_.completed(op))continue;
                const int sector=Frontier::TargetForOperation(op);
                if(!f.intel_.parleyed(sector))return false;
            }
        }
        if(schema>=5){
            std::array<uint8_t,3> stages{};
            std::array<uint16_t,3> dates{};
            for(auto &stage:stages)stage=r.U8();
            for(auto &date:dates)date=r.U16();
            if(!f.obligations_.Restore(stages,dates,f.accords_.mask(),f.day_))
                return false;
        }else if(!f.obligations_.Migrate(f.accords_.mask()))return false;
        while(r.position<r.capacity){if(r.U8()!=0)return false;}
        if(!r.ok)return false;
        if(f.deployed_){
            // v1 nested checkpoints from earlier releases remain playable as
            // their originally authored pump-defense missions. New v2
            // checkpoints may not spoof a different operation's rules.
            const uint8_t nested_schema=static_cast<uint8_t>(combat[8]);
            if(f.operation_>=kHearthfallOperations || (nested_schema>=2 &&
               next.tactical.battle.mission()!=Frontier::MissionForOperation(f.operation_)))return false;
            if(f.operation_>=3 && (nested_schema<3 ||
               static_cast<uint8_t>(next.tactical.battle.doctrine())!=
               Frontier::FactionForOperation(f.operation_)))return false;
            f.battle_=next.tactical.battle;
        }
        next.ResetEdges();next.rejected=false;
        *output=next;
        return true;
    }
private:
    static uint64_t Hash(const uint8_t *data,size_t length){
        uint64_t h=UINT64_C(14695981039346656037);
        for(size_t i=0;i<length;++i){h^=data[i];h*=UINT64_C(1099511628211);}
        return h;
    }
    struct Writer{
        Bytes &bytes;size_t position=0;bool ok=true;
        void U8(uint8_t v){if(position>=kBytes-8){ok=false;return;}bytes[position++]=v;}
        void U16(uint16_t v){U8(static_cast<uint8_t>(v));U8(static_cast<uint8_t>(v>>8));}
        void U32(uint32_t v){U16(static_cast<uint16_t>(v));U16(static_cast<uint16_t>(v>>16));}
    };
    struct Reader{
        const uint8_t *data;size_t capacity,position=0;bool ok=true;
        uint8_t U8(){if(position>=capacity){ok=false;return 0;}return data[position++];}
        uint16_t U16(){uint16_t lo=U8(),hi=U8();return static_cast<uint16_t>(lo|(hi<<8));}
        uint32_t U32(){uint32_t lo=U16(),hi=U16();return lo|(hi<<16);}
    };
};

}} // namespace reverie::omega
#endif
