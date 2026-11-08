#ifndef REVERIE_GUMBALL_CAMPAIGN_SAVE_H
#define REVERIE_GUMBALL_CAMPAIGN_SAVE_H
#include "gumball_armada_simulation.h"
#include "gumball_ship_3d.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace reverie { namespace gumball {
// A single checked transaction contains the campaign, exact live sortie and
// construction. Old campaign/shipyard slots are import sources only. No raw
// object layout, pointers, padding, GL resources or input edges are persisted.
class CampaignSave {
public:
    static constexpr size_t kBytes=4096;
    using Bytes=std::array<uint8_t,kBytes>;
    static bool Encode(const Simulation& sim,const ShipAssembly& ship,Bytes* out){
        if(!out||!Valid(sim,ship))return false;
        Bytes bytes{};
        std::memcpy(bytes.data(),"GUMFLY04",8);bytes[8]=4;
        Writer archive{bytes};
        Simulation copy=sim;Ship3DSave hull=ship.snapshot();
        Fields(archive,copy,hull);
        if(!archive.ok)return false;
        uint64_t hash=Hash(bytes.data(),kBytes-8);
        for(size_t i=kBytes-8;i<kBytes;++i){bytes[i]=static_cast<uint8_t>(hash);hash>>=8;}
        *out=bytes;return true;
    }
    static bool Decode(Simulation* sim,ShipAssembly* ship,const uint8_t* bytes,size_t size){
        if(!sim||!ship||!bytes||size!=kBytes||std::memcmp(bytes,"GUMFLY04",8)!=0
            ||bytes[8]!=4||bytes[9]||bytes[10]||bytes[11])return false;
        uint64_t hash=Hash(bytes,kBytes-8);
        for(size_t i=kBytes-8;i<kBytes;++i){
            if(bytes[i]!=static_cast<uint8_t>(hash))return false;
            hash>>=8;
        }
        Reader archive{bytes};Simulation candidate;Ship3DSave hull{};
        Fields(archive,candidate,hull);
        if(!archive.ok)return false;
        // Reserved bytes must stay zero, not an ignored alternate state.
        for(size_t i=archive.at;i<kBytes-8;++i)if(bytes[i])return false;
        hull.checksum=Ship3DChecksum(hull);
        ShipAssembly construction;
        if(!construction.restore(hull)||!Valid(candidate,construction))return false;
        *sim=candidate;*ship=construction;return true;
    }
private:
    static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559,
                  "Gumball checkpoints use IEEE754 binary32");
    static_assert(sizeof(int)==4,"Gumball counters use signed32");
    struct Writer {
        Bytes& bytes;size_t at=12;bool ok=true;
        void Byte(uint8_t value){if(at<kBytes-8)bytes[at++]=value;else ok=false;}
        void U8(uint8_t& value){Byte(value);}
        void U16(uint16_t& value){for(unsigned i=0;i<2;++i)Byte(static_cast<uint8_t>(value>>(8*i)));}
        void U32(uint32_t& value){for(unsigned i=0;i<4;++i)Byte(static_cast<uint8_t>(value>>(8*i)));}
        void I(int& value){uint32_t bits;std::memcpy(&bits,&value,4);U32(bits);}
        void F(float& value){uint32_t bits;std::memcpy(&bits,&value,4);U32(bits);}
        void B(bool& value){Byte(value?1u:0u);}
        template<class E>void Enum(E& value){Byte(static_cast<uint8_t>(value));}
    };
    struct Reader {
        const uint8_t* bytes;size_t at=12;bool ok=true;
        uint8_t Byte(){if(at<kBytes-8)return bytes[at++];ok=false;return 0;}
        void U8(uint8_t& value){value=Byte();}
        void U16(uint16_t& value){value=Byte();value|=static_cast<uint16_t>(Byte())<<8;}
        void U32(uint32_t& value){value=0;for(unsigned i=0;i<4;++i)value|=uint32_t(Byte())<<(8*i);}
        void I(int& value){uint32_t bits;U32(bits);std::memcpy(&value,&bits,4);}
        void F(float& value){uint32_t bits;U32(bits);std::memcpy(&value,&bits,4);}
        void B(bool& value){const auto byte=Byte();if(byte>1)ok=false;value=byte!=0;}
        template<class E>void Enum(E& value){value=static_cast<E>(Byte());}
    };
    template<class A>static void Fields(A& a,Simulation& s,Ship3DSave& ship){
        a.Enum(s.phase_);a.Enum(s.failure_);
        a.U32(s.seed_);a.U32(s.node_);a.U32(s.visited_);a.U32(s.cash_);
        a.U32(s.kills_);a.U32(s.bosses_);a.U32(s.rng_);
        a.U32(s.destination_.id);a.U8(s.destination_.hazard);a.U8(s.destination_.reward);
        a.B(s.destination_.fortress);
        a.Enum(s.mission_.kind);a.I(s.mission_.permitted_escapes);
        a.I(s.mission_.required_salvage);a.F(s.mission_.time_limit);a.U32(s.mission_.bonus);
        a.I(s.salvage_.collected_);a.I(s.salvage_.expired_);a.I(s.salvage_.dropped_);
        for(auto& p:s.salvage_.items_){a.F(p.x);a.F(p.y);a.U16(p.cash);a.B(p.active);}
        uint8_t levels[4]{};s.support_.export_levels(levels);
        uint8_t booked=static_cast<uint8_t>(s.support_.booked()),kits=s.support_.kit_mask();
        for(auto& level:levels)a.U8(level);
        a.U8(booked);a.U8(kits);
        if(!s.support_.restore(levels,booked,kits))a.ok=false;
        a.Enum(s.deployed_support_);a.F(s.support_timer_);a.I(s.support_pulses_);
        a.F(s.support_x_);a.B(s.slipstream_active_);
        a.U32(s.support_deployments_);a.U32(s.kit_activations_);
        a.Enum(s.cast_.kind_);a.F(s.cast_.elapsed_);a.F(s.cast_.start_time_);
        a.F(s.cast_.active_left_);a.F(s.cast_.cooldown_);a.I(s.cast_.pulses_);a.B(s.cast_.started_);
        a.U32(s.ally_shots_);a.U32(s.bombs_defused_);a.U32(s.mines_defused_);
        a.B(s.twins_.biff_);a.B(s.twins_.baff_);a.B(s.twins_.paid_);a.F(s.ally_x_);
        a.I(s.ship_profile_.hull);a.I(s.ship_profile_.shield);a.F(s.ship_profile_.speed);
        a.F(s.ship_profile_.pickup_radius);a.I(s.ship_profile_.mass);a.I(s.ship_profile_.power);
        a.I(s.ship_profile_.demand);a.I(s.ship_profile_.mounts);
        for(auto& x:s.ship_profile_.muzzle_x)a.F(x);
        for(auto& y:s.ship_profile_.muzzle_y)a.F(y);
        a.B(s.ship_ready_);
        for(auto& level:s.levels_)a.U16(level);
        a.U32(s.owned_weapons_);a.Enum(s.weapon_);
        a.I(s.hp_);a.I(s.shield_);a.I(s.combo_);a.I(s.spawned_);a.I(s.defeated_);a.I(s.target_);
        a.B(s.bossKilled_);a.I(s.escaped_);a.I(s.max_combo_);
        a.U32(s.sortie_cash_);a.U32(s.completion_bonus_);a.U32(s.shots_fired_);a.U32(s.shots_hit_);
        a.F(s.elapsed_);a.F(s.hurt_flash_);a.F(s.invulnerable_);a.F(s.regen_delay_);a.F(s.regen_credit_);
        a.F(s.px_);a.F(s.py_);a.F(s.spawn_clock_);a.F(s.cooldown_);a.F(s.combo_clock_);
        for(auto& e:s.enemies_){
            a.F(e.x);a.F(e.y);a.F(e.age);a.F(e.firing);a.I(e.hp);a.B(e.active);a.B(e.boss);
            a.Enum(e.kind);a.Enum(e.formation);a.U8(e.formation_slot);
        }
        for(auto& p:s.shots_){
            a.F(p.x);a.F(p.y);a.F(p.vy);a.B(p.active);a.B(p.hostile);a.F(p.vx);a.I(p.damage);
            a.B(p.ally);a.B(p.bomb);a.B(p.mine);a.B(p.zone);a.F(p.warning);
        }
        for(auto& cell:ship.cells)a.U8(cell);
        for(auto& stock:ship.stock)a.U8(stock);
    }
    static bool Range(int value,int low,int high){return value>=low&&value<=high;}
    static bool Range(float value,float low,float high){return std::isfinite(value)&&value>=low&&value<=high;}
    static bool SameProfile(const ShipProfile& a,const ShipProfile& b){
        return a.hull==b.hull&&a.shield==b.shield&&a.speed==b.speed
            &&a.pickup_radius==b.pickup_radius&&a.mass==b.mass&&a.power==b.power
            &&a.demand==b.demand&&a.mounts==b.mounts&&a.muzzle_x==b.muzzle_x&&a.muzzle_y==b.muzzle_y;
    }
    static bool Valid(const Simulation& s,const ShipAssembly& ship){
        if(static_cast<unsigned>(s.phase_)>4||static_cast<unsigned>(s.failure_)>5
            ||!s.seed_||s.node_>=kLaneCount||s.visited_>10000000u||s.cash_>100000000u
            ||s.kills_>100000000u||s.bosses_>15u)return false;
        for(auto level:s.levels_)if(level>12u)return false;
        if((s.owned_weapons_&~7u)||(s.owned_weapons_&1u)==0u
            ||static_cast<unsigned>(s.weapon_)>2u||!s.owns_weapon(s.weapon_))return false;
        if(s.destination_.id>=kLaneCount||s.destination_.hazard>8u||s.destination_.reward>5u
            ||static_cast<unsigned>(s.mission_.kind)>2u
            ||!Range(s.mission_.permitted_escapes,0,2)||!Range(s.mission_.required_salvage,0,3)
            ||!Range(s.mission_.time_limit,0.f,79.f)||s.mission_.bonus>104u)return false;
        if(s.target_>0){
            if(!s.destination_.hazard||!s.destination_.reward
                ||s.destination_.fortress!=(s.destination_.id%1024u==1023u))return false;
            const auto expected=MissionFor(static_cast<unsigned>(s.mission_.kind),s.destination_.hazard);
            if(s.mission_.permitted_escapes!=expected.permitted_escapes
                ||s.mission_.required_salvage!=expected.required_salvage
                ||s.mission_.time_limit!=expected.time_limit||s.mission_.bonus!=expected.bonus)return false;
        }else if(s.phase_!=Phase::Chart)return false;
        const auto& p=s.ship_profile_;
        if(!Range(p.hull,0,5000)||!Range(p.shield,0,3000)||!Range(p.speed,-.21f,10.f)
            ||!Range(p.pickup_radius,.12f,15.f)||!Range(p.mass,0,1000)
            ||!Range(p.power,0,2000)||!Range(p.demand,0,1000)||!Range(p.mounts,0,2))return false;
        for(auto x:p.muzzle_x)if(!Range(x,-1.f,1.f))return false;
        for(auto y:p.muzzle_y)if(!Range(y,-1.f,1.f))return false;
        if(s.phase_!=Phase::Chart&&(!s.ship_ready_||!ship.ready()||!SameProfile(p,ship.profile())))return false;
        if(!Range(s.hp_,-1000,90+s.levels_[1]*20+p.hull)
            ||!Range(s.shield_,0,20+s.levels_[3]*18+p.shield)
            ||!Range(s.target_,0,64)||!Range(s.spawned_,0,s.target_)
            ||!Range(s.defeated_,0,s.spawned_)||!Range(s.escaped_,0,s.defeated_)
            ||!Range(s.max_combo_,0,64)||!Range(s.combo_,0,s.max_combo_)
            ||s.sortie_cash_>100000000u||s.completion_bonus_>1000000u
            ||s.shots_fired_>100000000u||s.shots_hit_>s.shots_fired_)return false;
        if(s.phase_==Phase::Flight&&(s.hp_<=0||s.failure_!=FailureReason::None))return false;
        if(s.phase_==Phase::Lost&&s.failure_==FailureReason::None)return false;
        if((s.phase_==Phase::Won||s.phase_==Phase::ArmadaDefeated)
            &&(s.failure_!=FailureReason::None||s.defeated_!=s.target_||s.spawned_!=s.target_))return false;
        if(s.phase_==Phase::ArmadaDefeated&&s.bosses_!=15u)return false;
        if(!Range(s.elapsed_,0.f,1000000.f)||!Range(s.px_,-1.91f,1.91f)
            ||!Range(s.py_,-1.16f,.66f)||!Range(s.hurt_flash_,0.f,1.f)
            ||!Range(s.invulnerable_,0.f,1.f)||!Range(s.regen_delay_,0.f,3.f)
            ||!Range(s.regen_credit_,0.f,1.1f)||!Range(s.spawn_clock_,-1000000.f,5.f)
            ||!Range(s.cooldown_,-1000000.f,5.f)||!Range(s.combo_clock_,-1000000.f,3.f))return false;
        if(static_cast<unsigned>(s.deployed_support_)>2u
            ||!Range(s.support_timer_,-1000000.f,10.f)||!Range(s.support_pulses_,0,6)
            ||!Range(s.support_x_,-3.f,3.f)||s.support_deployments_>100000000u
            ||s.kit_activations_>2u||s.ally_shots_>5u
            ||s.bombs_defused_>100000000u||s.mines_defused_>100000000u
            ||!Range(s.ally_x_,-3.f,3.f))return false;
        if(static_cast<unsigned>(s.cast_.kind_)>8u||!Range(s.cast_.elapsed_,0.f,1000000.f)
            ||!Range(s.cast_.start_time_,0.f,6.f)||!Range(s.cast_.active_left_,0.f,5.5f)
            ||!Range(s.cast_.cooldown_,0.f,1.f)||!Range(s.cast_.pulses_,0,5))return false;
        if(s.twins_.paid_&&(!s.twins_.biff_||!s.twins_.baff_))return false;
        int active_salvage=0,active_enemies=0;
        for(const auto& pickup:s.salvage_.items_){
            if(!Range(pickup.x,-3.f,3.f)||!Range(pickup.y,-2.f,2.f)||pickup.cash>1000)return false;
            if(pickup.active){if(!pickup.cash)return false;++active_salvage;}
        }
        if(!Range(s.salvage_.collected_,0,64)||!Range(s.salvage_.expired_,0,64)
            ||!Range(s.salvage_.dropped_,0,64)
            ||s.salvage_.dropped_!=s.salvage_.collected_+s.salvage_.expired_+active_salvage)return false;
        for(const auto& e:s.enemies_){
            if(!Range(e.x,-2.f,2.f)||!Range(e.y,-2.f,2.f)||!Range(e.age,0.f,1000000.f)
                ||!Range(e.firing,-1000000.f,5.f)||!Range(e.hp,-1000,1000)
                ||static_cast<unsigned>(e.kind)>12u||static_cast<unsigned>(e.formation)>7u
                ||e.formation_slot>4u)return false;
            if(e.active){if(e.hp<=0)return false;++active_enemies;}
        }
        if(active_enemies!=s.spawned_-s.defeated_)return false;
        for(const auto& shot:s.shots_){
            if(!Range(shot.x,-20.f,20.f)||!Range(shot.y,-3.f,3.f)
                ||!Range(shot.vx,-5.f,5.f)||!Range(shot.vy,-5.f,5.f)
                ||!Range(shot.damage,1,100)||!Range(shot.warning,0.f,1.f))return false;
        }
        const auto saved_ship=ship.snapshot();ShipAssembly checked_ship;
        return checked_ship.restore(saved_ship);
    }
    static uint64_t Hash(const uint8_t* bytes,size_t size){
        uint64_t hash=14695981039346656037ull;
        for(size_t i=0;i<size;++i)hash=(hash^bytes[i])*1099511628211ull;
        return hash;
    }
};
}}
#endif
