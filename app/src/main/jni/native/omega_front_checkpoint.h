#ifndef REVERIE_OMEGA_FRONT_CHECKPOINT_H
#define REVERIE_OMEGA_FRONT_CHECKPOINT_H

#include "omega_front_controls.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstring>

namespace reverie { namespace omega {

// Fixed, little-endian and padding-free. Always validate into a temporary
// Controller before assignment; a damaged slot cannot partially restore.
class Checkpoint {
public:
    static constexpr size_t kBytes = 1152;
    using Bytes = std::array<uint8_t, kBytes>;

    static bool Encode(const Controller &control, Bytes *output) {
        if (!output) return false;
        Bytes bytes{};
        Codec w(bytes.data(), kBytes - 8);
        for (uint8_t byte : {uint8_t('O'),uint8_t('F'),uint8_t('H'),uint8_t('B'),
                             uint8_t('E'),uint8_t('L'),uint8_t('0'),uint8_t('1')}) w.U8(byte);
        w.U16(1); w.U16(0); // v1; reserved, must remain zero
        w.U32(control.seed_);
        w.U8(static_cast<uint8_t>(control.cursor_x));
        w.U8(static_cast<uint8_t>(control.cursor_y));
        w.U8(static_cast<uint8_t>(control.selected + 1));
        w.U8(static_cast<uint8_t>(control.mode));
        w.U8(control.paused ? 1 : 0);
        w.U32(control.issued);
        w.F32(control.clock_);
        const Battle &battle = control.battle;
        w.U32(battle.rng_); w.U32(battle.ticks_);
        w.U8(static_cast<uint8_t>(battle.result_));
        w.U8(static_cast<uint8_t>(battle.count_));
        w.U8(static_cast<uint8_t>(battle.pump_hp_));
        w.U8(static_cast<uint8_t>(battle.components_));
        w.U8(static_cast<uint8_t>(battle.ammo_stock_));
        for (auto ground : battle.cells_) w.U8(static_cast<uint8_t>(ground));
        for (int i = 0; i < kUnitLimit; ++i) {
            const Unit u = i < battle.count_ ? battle.units_[i] : Unit{};
            w.U8(u.id);w.U8(static_cast<uint8_t>(u.side));
            w.U8(static_cast<uint8_t>(u.role));
            w.U8(static_cast<uint8_t>(u.x));w.U8(static_cast<uint8_t>(u.y));
            w.U8(static_cast<uint8_t>(u.hp));w.U8(static_cast<uint8_t>(u.ammo));
        }
        for (int i = 0; i < kUnitLimit; ++i) {
            const Battle::Queue q = i < battle.count_ ? battle.queues_[i] : Battle::Queue{};
            w.U8(static_cast<uint8_t>(q.size));
            for (int j = 0; j < kOrdersPerUnit; ++j) {
                const Order o = j < q.size ? q.entries[j] : Order{};
                w.U8(static_cast<uint8_t>(o.type));
                w.U16(static_cast<uint16_t>(o.a));w.U16(static_cast<uint16_t>(o.b));
            }
        }
        w.U32(battle.last_.tick);
        w.U8(battle.last_.actor);w.U8(battle.last_.target);
        w.U16(static_cast<uint16_t>(battle.last_.damage));
        w.U8(battle.last_.pump ? 1 : 0);
        if (!w.valid || w.cursor > kBytes - 8) return false;
        uint64_t hash = Hash(bytes.data(), kBytes - 8);
        for (size_t i = kBytes - 8; i < kBytes; ++i) {
            bytes[i] = static_cast<uint8_t>(hash);
            hash >>= 8;
        }
        *output = bytes;
        return true;
    }

    static bool Decode(Controller *control, const uint8_t *data, size_t size) {
        if (!control || !data || size != kBytes) return false;
        uint64_t digest = Hash(data, kBytes - 8);
        for (size_t i = kBytes - 8; i < kBytes; ++i) {
            if (data[i] != static_cast<uint8_t>(digest)) return false;
            digest >>= 8;
        }
        Codec r(data, kBytes - 8);
        const char magic[]="OFHBEL01";
        for (int i = 0; i < 8; ++i) if (r.U8() != static_cast<uint8_t>(magic[i])) return false;
        if (r.U16()!=1 || r.U16()!=0) return false;
        uint32_t seed=r.U32();
        const int cursor_x=r.U8(),cursor_y=r.U8(),selected=r.U8()-1;
        const uint8_t mode=r.U8(),pause=r.U8();
        const uint32_t issued=r.U32();
        const float clock=r.F32();
        if (!seed || !Battle::Inside(cursor_x,cursor_y) || selected >= kUnitLimit ||
            mode>static_cast<uint8_t>(CommandMode::Resupply) || pause>1 ||
            !std::isfinite(clock) || clock<0.f || clock>=.25f) return false;
        Controller temp(seed);
        temp.cursor_x=cursor_x;temp.cursor_y=cursor_y;
        temp.selected=selected;temp.mode=static_cast<CommandMode>(mode);
        temp.paused=pause!=0;temp.issued=issued;temp.clock_=clock;
        // Physical button edge history is transient, not persistent.
        temp.primary_down_=false;temp.secondary_down_=false;
        temp.repeat_=0;temp.axis_x_=temp.axis_y_=0;
        temp.last_pointer_x_=temp.last_pointer_y_=-1;
        temp.rejected=false;
        Battle &battle=temp.battle;
        battle.rng_=r.U32();battle.ticks_=r.U32();
        uint8_t result=r.U8();battle.count_=r.U8();
        battle.pump_hp_=r.U8();battle.components_=r.U8();battle.ammo_stock_=r.U8();
        if(battle.ticks_>160 || result>static_cast<uint8_t>(Result::Overrun) ||
           battle.count_<2 || battle.count_>kUnitLimit || battle.pump_hp_>18 ||
           battle.components_>4 || battle.ammo_stock_>18) return false;
        battle.result_=static_cast<Result>(result);
        for (Ground &ground : battle.cells_) {
            uint8_t g=r.U8();
            if (g>static_cast<uint8_t>(Ground::DeepWater)) return false;
            ground=static_cast<Ground>(g);
        }
        if(!battle.Walkable(2,12) || !battle.Walkable(5,12)) return false;
        int friends=0,foes=0;
        for (int i = 0; i < kUnitLimit; ++i) {
            const uint8_t id=r.U8(),side=r.U8(),role=r.U8();
            const uint8_t x=r.U8(),y=r.U8(),hp=r.U8(),ammo=r.U8();
            if (i >= battle.count_) {
                if(id||side||role||x||y||hp||ammo) return false;
                continue;
            }
            if (id!=i || side>1 || role>2 || !Battle::Inside(x,y) ||
                hp>(role==static_cast<uint8_t>(Class::Heavy)?13:9) ||
                ammo>(role==static_cast<uint8_t>(Class::Heavy)?5:8)) return false;
            auto &u=battle.units_[i];
            u=Unit{id,static_cast<Side>(side),static_cast<Class>(role),
                   static_cast<int16_t>(x),static_cast<int16_t>(y),hp,ammo};
            if (hp) {
                if(!battle.Walkable(x,y) || (x==2&&y==12)) return false;
                // Comparing directly avoids treating as-yet unloaded units as actors.
                for(int j=0;j<i;++j)if(battle.units_[j].alive() &&
                    battle.units_[j].x==x && battle.units_[j].y==y) return false;
                if(side==0)++friends;else ++foes;
            }
        }
        if (selected >= battle.count_ || (selected>=0 &&
            battle.units_[selected].side!=Side::Hearthwell))
            return false;
        for (int i=0;i<kUnitLimit;++i) {
            uint8_t n=r.U8();
            if(n>kOrdersPerUnit || (i>=battle.count_ && n)) return false;
            Battle::Queue &q=battle.queues_[i];q.size=n;
            for(int j=0;j<kOrdersPerUnit;++j) {
                uint8_t type=r.U8();int16_t a=static_cast<int16_t>(r.U16());
                int16_t b=static_cast<int16_t>(r.U16());
                if(j>=n){if(type || a || b)return false;continue;}
                if(i>=battle.count_ || battle.units_[i].side!=Side::Hearthwell ||
                   type<1 || type>4) return false;
                const OrderType t=static_cast<OrderType>(type);
                if(t==OrderType::Move && (!battle.Walkable(a,b) || (a==2&&b==12)))return false;
                if(t==OrderType::Attack && (a<0 || a>=battle.count_ ||
                   battle.units_[a].side!=Side::Raiders || b!=0))return false;
                if(t==OrderType::RepairPump && (battle.units_[i].role!=Class::Engineer ||a||b))return false;
                if(t==OrderType::Resupply && (a||b))return false;
                q.entries[j]=Order{t,a,b};
            }
        }
        battle.last_.tick=r.U32();battle.last_.actor=r.U8();
        battle.last_.target=r.U8();
        battle.last_.damage=static_cast<int16_t>(r.U16());
        uint8_t pump=r.U8();battle.last_.pump=pump!=0;
        if(pump>1 || battle.last_.tick>battle.ticks_ ||
           (battle.last_.tick && battle.last_.actor>=battle.count_) ||
           battle.last_.damage<0 || battle.last_.damage>8 ||
           (battle.last_.pump && battle.last_.target!=255)) return false;
        // Writer zero pads the unused part of a versioned slot.
        while(r.cursor<kBytes-8) if(r.U8()!=0) return false;
        if(!r.valid || (battle.result_==Result::Active &&
           (battle.ticks_>=160 || !friends || !foes || !battle.pump_hp_)) ||
           (battle.result_==Result::Defended && (!friends || foes || !battle.pump_hp_)) ||
           (battle.result_==Result::Overrun && battle.ticks_<160 &&
             friends && foes && battle.pump_hp_)) return false;
        *control=temp;
        return true;
    }
private:
    struct Codec {
        uint8_t *writer=nullptr;
        const uint8_t *reader=nullptr;
        size_t capacity=0,cursor=0;
        bool valid=true;
        Codec(uint8_t *b,size_t n):writer(b),capacity(n){}
        Codec(const uint8_t *b,size_t n):reader(b),capacity(n){}
        void U8(uint8_t b) {
            if(cursor==capacity){valid=false;return;}
            writer[cursor++]=b;
        }
        uint8_t U8() {
            if(cursor==capacity){valid=false;return 0;}
            return reader[cursor++];
        }
        void U16(uint16_t b){U8(static_cast<uint8_t>(b));U8(static_cast<uint8_t>(b>>8));}
        uint16_t U16(){const uint16_t a=U8(),b=U8();return static_cast<uint16_t>(a|(b<<8));}
        void U32(uint32_t b){U16(static_cast<uint16_t>(b));U16(static_cast<uint16_t>(b>>16));}
        uint32_t U32(){const uint32_t a=U16(),b=U16();return a|(b<<16);}
        void F32(float v){uint32_t bits=0;static_assert(sizeof(bits)==sizeof(v),"float32 required");
            std::memcpy(&bits,&v,sizeof(bits));U32(bits);}
        float F32(){uint32_t bits=U32();float f=0;std::memcpy(&f,&bits,sizeof(bits));return f;}
    };
    static uint64_t Hash(const uint8_t *data,size_t size){
        uint64_t hash=UINT64_C(14695981039346656037);
        for(size_t i=0;i<size;++i){hash^=data[i];hash*=UINT64_C(1099511628211);}
        return hash;
    }
};
}} // namespace reverie::omega
#endif
