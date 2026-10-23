#ifndef REVERIE_OMEGA_FRONT_TACTICS_H
#define REVERIE_OMEGA_FRONT_TACTICS_H

#include <array>
#include <cstdint>
#include <cstdlib>

// Omega Front P1: deterministic, allocation-free combat. Rendering and host
// input are consumers; no GLES, clock, random_device, or Android dependencies.
namespace reverie { namespace omega {

constexpr int kWidth = 24;
constexpr int kCells = kWidth * kWidth;
constexpr int kUnitLimit = 16;
constexpr int kOrdersPerUnit = 4;

enum class Ground : uint8_t { Plain, Road, Rubble, Wall, DeepWater };
enum class Side : uint8_t { Hearthwell, Raiders };
// Roles grow without reinterpreting the three original unit IDs.
enum class Class : uint8_t { Rifle, Engineer, Heavy, Salvager, Sentinel, Legionary };
// 0 remains the original unaligned raider behaviour. Numeric IDs follow the
// sixteen-sector campaign faction table; these are not extra factions.
enum class Doctrine : uint8_t { Unaligned=0, Hearthward=1, Meridian=2, Scrapline=4, LostNinth=6 };
inline bool ValidDoctrine(uint8_t v) { return v==0 || v==1 || v==2 || v==4 || v==6; }

enum class OrderType : uint8_t { None, Move, Attack, RepairPump, Resupply };
enum class Result : uint8_t { Active, Defended, Overrun };
// All three first-arc operations have distinct, real win conditions.
enum class Mission : uint8_t { HoldPump, CaptureOrchard, DisableRelay };

struct Unit {
    uint8_t id = 0;
    Side side = Side::Hearthwell;
    Class role = Class::Rifle;
    int16_t x = 0, y = 0;
    int16_t hp = 0;
    int16_t ammo = 0;
    bool alive() const { return hp > 0; }
};
struct Order {
    OrderType type = OrderType::None;
    int16_t a = 0, b = 0;
};
struct Event {
    uint32_t tick = 0;
    uint8_t actor = 0, target = 0;
    int16_t damage = 0;
    bool pump = false;
};

class Battle {
public:
    explicit Battle(uint32_t seed = 1) : rng_(seed ? seed : 1) {
        cells_.fill(Ground::Plain);
        for (int y = 1; y < 23; ++y) cells_[Index(12,y)] = Ground::Rubble;
        for (int y = 8; y < 16; ++y) cells_[Index(12,y)] = Ground::Road;
        for (int x = 0; x < 24; ++x) cells_[Index(x,12)] = Ground::Road;
        cells_[Index(9,9)] = Ground::Wall;
        cells_[Index(9,10)] = Ground::Wall;
        cells_[Index(9,11)] = Ground::Wall;
        cells_[Index(9,12)] = Ground::Road;
        cells_[Index(9,13)] = Ground::Wall;
        cells_[Index(9,14)] = Ground::Wall;
        AddUnit(Side::Hearthwell, Class::Rifle, 4, 11);
        AddUnit(Side::Hearthwell, Class::Rifle, 4, 13);
        AddUnit(Side::Hearthwell, Class::Engineer, 3, 12);
        AddUnit(Side::Hearthwell, Class::Heavy, 6, 12);
        AddUnit(Side::Raiders, Class::Rifle, 19, 10);
        AddUnit(Side::Raiders, Class::Rifle, 19, 14);
        AddUnit(Side::Raiders, Class::Heavy, 21, 12);
        AddUnit(Side::Raiders, Class::Engineer, 20, 12);
    }

    static bool Inside(int x, int y) { return x >= 0 && x < kWidth && y >= 0 && y < kWidth; }
    static int Index(int x, int y) { return y * kWidth + x; }
    Ground ground(int x, int y) const { return Inside(x,y) ? cells_[Index(x,y)] : Ground::Wall; }
    bool SetGround(int x, int y, Ground g) {
        if (ticks_ != 0 || !Inside(x,y) || (x == pump_x_ && y == pump_y_) || Occupant(x,y) >= 0 ||
            static_cast<unsigned>(g) > static_cast<unsigned>(Ground::DeepWater)) return false;
        cells_[Index(x,y)] = g;
        return true;
    }
    void ClearUnits() {
        count_ = 0;
        queues_ = {};
        ticks_ = 0;
        result_ = Result::Active;
        last_ = {};
        pump_hp_ = 18;
        components_ = 4;
        ammo_stock_ = 18;
        mission_ = Mission::HoldPump;
        objective_progress_ = 0;
        doctrine_ = Doctrine::Unaligned;
        stolen_rounds_ = 0;
    }
    int AddUnit(Side s, Class role, int x, int y) {
        if (result_ != Result::Active || ticks_ != 0 || count_ >= kUnitLimit || !Walkable(x,y) ||
            Occupant(x,y) >= 0 || (x == pump_x_ && y == pump_y_) ||
            static_cast<unsigned>(role) > static_cast<unsigned>(Class::Legionary) ||
            static_cast<unsigned>(s) > static_cast<unsigned>(Side::Raiders)) return -1;
        int id = count_++;
        units_[id] = Unit{static_cast<uint8_t>(id), s, role,
                          static_cast<int16_t>(x), static_cast<int16_t>(y),
                          static_cast<int16_t>(MaxHp(role)),
                          static_cast<int16_t>(InitialAmmo(role))};
        return id;
    }
    const Unit *unit(int id) const { return Valid(id) ? &units_[id] : nullptr; }
    int unit_count() const { return count_; }
    uint32_t ticks() const { return ticks_; }
    int pump_hp() const { return pump_hp_; }
    static int MaxHp(Class role) {
        switch(role){case Class::Heavy:return 13;case Class::Salvager:return 8;
          case Class::Sentinel:return 12;case Class::Legionary:return 14;
          default:return 9;}
    }
    static int MaxAmmo(Class role) {
        switch(role){case Class::Heavy:return 5;case Class::Salvager:return 6;
          case Class::Sentinel:return 6;case Class::Legionary:return 0;
          default:return 8;}
    }
    static int InitialAmmo(Class role) {return role==Class::Salvager?2:MaxAmmo(role);}
    Doctrine doctrine() const {return doctrine_;}
    int stolen_rounds() const {return stolen_rounds_;}
    bool ConfigureDoctrine(Doctrine doctrine){
        if(ticks_!=0 || result_!=Result::Active ||
           !ValidDoctrine(static_cast<uint8_t>(doctrine)))return false;
        doctrine_=doctrine;stolen_rounds_=0;return true;
    }
    Mission mission() const { return mission_; }
    int objective_progress() const { return objective_progress_; }
    int objective_goal() const { return mission_==Mission::CaptureOrchard?3:
        mission_==Mission::DisableRelay?2:0; }
    int objective_x() const { return mission_==Mission::CaptureOrchard?17:
        mission_==Mission::DisableRelay?18:2; }
    int objective_y() const { return 12; }
    // Set by the strategic mission builder only before any action. This
    // cannot transform a battle after its outcome is known.
    bool ConfigureMission(Mission mission) {
        if (ticks_ || result_!=Result::Active ||
            static_cast<unsigned>(mission)>static_cast<unsigned>(Mission::DisableRelay)) return false;
        mission_=mission;
        objective_progress_=0;
        return true;
    }
    // Scenario loading only. Does not repair a live or terminal encounter.
    bool ConfigurePump(int hp) {
        if (ticks_ || result_ != Result::Active || hp<1 || hp>18) return false;
        pump_hp_=hp;return true;
    }
    int components() const { return components_; }
    int ammo_stock() const { return ammo_stock_; }
    Result result() const { return result_; }
    Event last_event() const { return last_; }
    int queued(int id) const { return Valid(id) ? queues_[id].size : 0; }

    bool QueueMove(int id, int x, int y) {
        if (!PlayerCanOrder(id) || !Walkable(x,y) ||
            (x == pump_x_ && y == pump_y_) ||
            !PathExists(units_[id].x, units_[id].y, x, y)) return false;
        return Enqueue(id, Order{OrderType::Move, static_cast<int16_t>(x), static_cast<int16_t>(y)});
    }
    bool QueueAttack(int id, int target) {
        if (!PlayerCanOrder(id) || !Valid(target) || !units_[target].alive() ||
            units_[target].side == units_[id].side ||
            Distance(units_[id].x,units_[id].y,units_[target].x,units_[target].y) > 9 ||
            !Visible(units_[id].x,units_[id].y,units_[target].x,units_[target].y)) return false;
        return Enqueue(id, Order{OrderType::Attack, static_cast<int16_t>(target), 0});
    }
    bool QueueRepair(int id) {
        return PlayerCanOrder(id) && units_[id].role == Class::Engineer &&
            Enqueue(id, Order{OrderType::RepairPump, 0, 0});
    }
    bool QueueResupply(int id) {
        return PlayerCanOrder(id) && units_[id].role!=Class::Legionary &&
            Enqueue(id, Order{OrderType::Resupply, 0, 0});
    }
    bool CancelOrders(int id) {
        if (!PlayerCanOrder(id)) return false;
        queues_[id] = {};
        return true;
    }

    // One 250-ms logical command pulse. The shell may pause without stepping;
    // neither rendering an eye nor requesting a checksum changes battle state.
    void Step() {
        if (result_ != Result::Active) return;
        ++ticks_;
        last_ = {};
        for (int id = 0; id < count_; ++id) {
            if (units_[id].alive() && units_[id].side == Side::Hearthwell) ExecutePlayer(id);
        }
        for (int id = 0; id < count_; ++id) {
            if (units_[id].alive() && units_[id].side == Side::Raiders) ExecuteEnemy(id);
        }
        int friends = 0, foes = 0;
        for (int id = 0; id < count_; ++id) {
            if (!units_[id].alive()) continue;
            if (units_[id].side == Side::Hearthwell) ++friends;
            else ++foes;
        }
        // Mission objectives are simulated after both sides move/fire.
        // A defeated army does not automatically capture an orchard or
        // disable a relay: surviving troops must complete that actual task.
        if (mission_==Mission::CaptureOrchard) {
            bool holding=false,contested=false;
            for(int id=0;id<count_;++id) {
                const Unit &u=units_[id];
                if(!u.alive())continue;
                const int d=Distance(u.x,u.y,objective_x(),objective_y());
                if(u.side==Side::Hearthwell && d==0)holding=true;
                if(u.side==Side::Raiders && d<=1)contested=true;
            }
            if (holding && !contested) {
                if(objective_progress_<3)++objective_progress_;
            } else objective_progress_=0;
        } else if (mission_==Mission::DisableRelay) {
            bool engineer=false,contested=false;
            for(int id=0;id<count_;++id) {
                const Unit &u=units_[id];
                if(!u.alive())continue;
                const int d=Distance(u.x,u.y,objective_x(),objective_y());
                if(u.side==Side::Hearthwell && u.role==Class::Engineer && d<=1)engineer=true;
                if(u.side==Side::Raiders && d<=1)contested=true;
            }
            // Engineers physically spend one charge per successful pulse.
            if(engineer && !contested && components_>0 && objective_progress_<2){
                --components_;
                ++objective_progress_;
            }
        }
        if (!friends || ticks_>=160 ||
            (mission_!=Mission::CaptureOrchard && pump_hp_==0)) result_=Result::Overrun;
        else if ((mission_==Mission::HoldPump && !foes) ||
            (mission_==Mission::CaptureOrchard && objective_progress_==3) ||
            (mission_==Mission::DisableRelay && objective_progress_==2)) result_=Result::Defended;
    }

    // FNV-1a over *explicit logical fields*, never struct padding/pointers.
    uint64_t Checksum() const {
        uint64_t hash = UINT64_C(14695981039346656037);
        auto add = [&hash](uint64_t v) {
            for (int i = 0; i < 8; ++i) {
                hash ^= (v >> (i * 8)) & 255u;
                hash *= UINT64_C(1099511628211);
            }
        };
        add(rng_); add(ticks_); add(static_cast<unsigned>(result_));
        add(pump_hp_); add(components_); add(ammo_stock_); add(count_);
        add(static_cast<unsigned>(mission_)); add(objective_progress_);
        add(static_cast<unsigned>(doctrine_)); add(stolen_rounds_);
        for (Ground g : cells_) add(static_cast<unsigned>(g));
        for (int id = 0; id < count_; ++id) {
            const Unit &u = units_[id];
            add(u.id); add(static_cast<unsigned>(u.side)); add(static_cast<unsigned>(u.role));
            add(static_cast<uint16_t>(u.x)); add(static_cast<uint16_t>(u.y));
            add(static_cast<uint16_t>(u.hp)); add(static_cast<uint16_t>(u.ammo));
            const Queue &q = queues_[id];
            add(q.size);
            for (int j = 0; j < q.size; ++j) {
                add(static_cast<unsigned>(q.entries[j].type));
                add(static_cast<uint16_t>(q.entries[j].a));
                add(static_cast<uint16_t>(q.entries[j].b));
            }
        }
        add(last_.tick); add(last_.actor); add(last_.target);
        add(static_cast<uint16_t>(last_.damage)); add(last_.pump);
        return hash;
    }

private:
    friend class Checkpoint;
    struct Queue { std::array<Order,kOrdersPerUnit> entries{}; int size = 0; };
    std::array<Ground,kCells> cells_{};
    std::array<Unit,kUnitLimit> units_{};
    std::array<Queue,kUnitLimit> queues_{};
    uint32_t rng_ = 1;
    uint32_t ticks_ = 0;
    Result result_ = Result::Active;
    Event last_{};
    int count_ = 0;
    int pump_hp_ = 18;
    int components_ = 4;
    int ammo_stock_ = 18;
    Mission mission_ = Mission::HoldPump;
    uint8_t objective_progress_ = 0;
    Doctrine doctrine_ = Doctrine::Unaligned;
    uint8_t stolen_rounds_ = 0;
    static constexpr int pump_x_ = 2, pump_y_ = 12;
    static constexpr int crate_x_ = 5, crate_y_ = 12;

    bool Valid(int id) const { return id >= 0 && id < count_; }
    bool PlayerCanOrder(int id) const {
        return result_ == Result::Active && Valid(id) && units_[id].alive() &&
               units_[id].side == Side::Hearthwell;
    }
    bool Walkable(int x, int y) const {
        if (!Inside(x,y)) return false;
        Ground g = cells_[Index(x,y)];
        return g != Ground::Wall && g != Ground::DeepWater;
    }
    int Occupant(int x, int y, int except = -1) const {
        for (int i = 0; i < count_; ++i)
            if (i != except && units_[i].alive() && units_[i].x == x && units_[i].y == y) return i;
        return -1;
    }
    bool Enqueue(int id, Order o) {
        Queue &q = queues_[id];
        if (q.size >= kOrdersPerUnit) return false;
        q.entries[q.size++] = o;
        return true;
    }
    void Pop(int id) {
        Queue &q = queues_[id];
        if (!q.size) return;
        for (int i = 1; i < q.size; ++i) q.entries[i-1] = q.entries[i];
        q.entries[--q.size] = {};
    }
    uint32_t Random() { rng_ = rng_ * 1664525u + 1013904223u; return rng_; }
    static int Distance(int x, int y, int tx, int ty) { return abs(x-tx) + abs(y-ty); }
    static int Range(Class role) {
        if(role==Class::Sentinel)return 7;
        if(role==Class::Legionary)return 1;
        if(role==Class::Heavy)return 5;
        if(role==Class::Rifle)return 6;
        return 3;
    }
    static int BaseDamage(Class role) {
        if(role==Class::Heavy)return 6;
        if(role==Class::Legionary)return 5;
        if(role==Class::Sentinel || role==Class::Rifle)return 4;
        if(role==Class::Salvager)return 3;
        return 2;
    }
    static int Armor(Class role) {
        return role==Class::Legionary || role==Class::Sentinel || role==Class::Heavy?2:0;
    }
    static int AmmoCap(Class role){return MaxAmmo(role);}
    int FormationArmor(int target) const {
        const Unit &u=units_[target];
        if(u.role!=Class::Legionary)return 0;
        for(int j=0;j<count_;++j){
            if(j==target)continue;
            const Unit &other=units_[j];
            if(other.alive() && other.side==u.side && other.role==Class::Legionary &&
               Distance(u.x,u.y,other.x,other.y)==1)return 2;
        }
        return 0;
    }

    // Bresenham test: terrain blocks both player and AI fire identically.
    bool Visible(int x0, int y0, int x1, int y1) const {
        int dx = abs(x1-x0), dy = abs(y1-y0);
        int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
        int err = dx-dy;
        while (x0 != x1 || y0 != y1) {
            int twice = 2*err;
            if (twice > -dy) { err -= dy; x0 += sx; }
            if (twice < dx) { err += dx; y0 += sy; }
            if (!Inside(x0,y0)) return false;
            if ((x0 != x1 || y0 != y1) && ground(x0,y0) == Ground::Wall) return false;
        }
        return true;
    }
    bool CanShoot(int shooter, int target) const {
        const Unit &a = units_[shooter], &b = units_[target];
        return a.alive() && b.alive() && (a.role==Class::Legionary || a.ammo>0) &&
               Distance(a.x,a.y,b.x,b.y) <= Range(a.role) &&
               Visible(a.x,a.y,b.x,b.y);
    }
    bool Shoot(int shooter, int target) {
        if (!CanShoot(shooter,target)) return false;
        Unit &a = units_[shooter], &b = units_[target];
        if(a.role!=Class::Legionary)--a.ammo; // Melee does not invent bullets.
        int cover = ground(b.x,b.y) == Ground::Rubble ? 2 : 0;
        int damage = BaseDamage(a.role) + static_cast<int>(Random()%3u) -
                     Armor(b.role) - cover - FormationArmor(target);
        if (damage < 0) damage = 0;
        b.hp = static_cast<int16_t>(b.hp > damage ? b.hp - damage : 0);
        last_ = Event{ticks_,a.id,b.id,static_cast<int16_t>(damage),false};
        return true;
    }
    bool ShootPump(int id) {
        Unit &u = units_[id];
        if (!u.alive() || (u.role!=Class::Legionary && u.ammo==0) ||
            Distance(u.x,u.y,pump_x_,pump_y_) > Range(u.role) ||
            !Visible(u.x,u.y,pump_x_,pump_y_)) return false;
        if(u.role!=Class::Legionary)--u.ammo;
        int damage = BaseDamage(u.role) + static_cast<int>(Random()%3u);
        pump_hp_ = pump_hp_ > damage ? pump_hp_ - damage : 0;
        last_ = Event{ticks_,u.id,255,static_cast<int16_t>(damage),true};
        return true;
    }
    // BFS cardinal movement and occupancy: bounded to 576 cells, deterministic
    // neighbor priority (right, down, left, up), with no heap allocation.
    bool StepToward(int id, int tx, int ty, bool exact) {
        Unit &u = units_[id];
        if (u.x == tx && u.y == ty) return true;
        std::array<int16_t,kCells> parent{};
        parent.fill(-1);
        std::array<int16_t,kCells> fifo{};
        int head = 0, tail = 0;
        int start = Index(u.x,u.y), best = start;
        int dist = Distance(u.x,u.y,tx,ty);
        parent[start] = static_cast<int16_t>(start);
        fifo[tail++] = static_cast<int16_t>(start);
        const int step_x[4] = {1,0,-1,0}, step_y[4] = {0,1,0,-1};
        while (head < tail) {
            int cell = fifo[head++], x = cell % kWidth, y = cell / kWidth;
            for (int j = 0; j < 4; ++j) {
                int nx = x+step_x[j], ny = y+step_y[j];
                if (!Walkable(nx,ny) || (nx == pump_x_ && ny == pump_y_) ||
                    Occupant(nx,ny,id) >= 0) continue;
                int ni = Index(nx,ny);
                if (parent[ni] != -1) continue;
                parent[ni] = static_cast<int16_t>(cell);
                fifo[tail++] = static_cast<int16_t>(ni);
                int d = Distance(nx,ny,tx,ty);
                if (d < dist) { best = ni; dist = d; }
            }
        }
        if (exact) {
            if (parent[Index(tx,ty)] == -1) return false;
            best = Index(tx,ty);
        }
        if (best == start) return false;
        while (parent[best] != start) best = parent[best];
        u.x = static_cast<int16_t>(best % kWidth);
        u.y = static_cast<int16_t>(best / kWidth);
        return true;
    }
    bool PathExists(int x0, int y0, int tx, int ty) const {
        if (x0 == tx && y0 == ty) return true;
        std::array<uint8_t,kCells> seen{};
        std::array<int16_t,kCells> fifo{};
        int head=0,tail=0;
        fifo[tail++] = static_cast<int16_t>(Index(x0,y0));
        seen[Index(x0,y0)] = 1;
        const int dx[4]={1,0,-1,0},dy[4]={0,1,0,-1};
        while (head<tail) {
            int v=fifo[head++],x=v%kWidth,y=v/kWidth;
            for(int i=0;i<4;++i) {
                int nx=x+dx[i],ny=y+dy[i];
                if(!Walkable(nx,ny)||(nx==pump_x_&&ny==pump_y_)||seen[Index(nx,ny)])continue;
                if(nx==tx&&ny==ty)return true;
                seen[Index(nx,ny)]=1;
                fifo[tail++]=static_cast<int16_t>(Index(nx,ny));
            }
        }
        return false;
    }
    void ExecutePlayer(int id) {
        Queue &q = queues_[id];
        if (!q.size) return;
        const Order o = q.entries[0];
        Unit &u = units_[id];
        if (o.type == OrderType::Move) {
            if ((u.x == o.a && u.y == o.b) || !StepToward(id,o.a,o.b,true)) Pop(id);
            else if (u.x == o.a && u.y == o.b) Pop(id);
        } else if (o.type == OrderType::Attack) {
            if (!Valid(o.a) || !units_[o.a].alive() || (!u.ammo && u.role!=Class::Legionary)) Pop(id);
            else if (!CanShoot(id,o.a)) {
                // Attack orders pursue only *currently observed* enemy locations.
                // No invisible automatic tracking through hard cover.
                if (Distance(u.x,u.y,units_[o.a].x,units_[o.a].y) > 9 ||
                    !Visible(u.x,u.y,units_[o.a].x,units_[o.a].y)) Pop(id);
                else if (!StepToward(id,units_[o.a].x,units_[o.a].y,false)) Pop(id);
            } else {
                Shoot(id,o.a);
                if (!units_[o.a].alive() || (!u.ammo && u.role!=Class::Legionary)) Pop(id);
            }
        } else if (o.type == OrderType::RepairPump) {
            if (!components_ || pump_hp_ == 18) Pop(id);
            else if (Distance(u.x,u.y,pump_x_,pump_y_) == 1) {
                --components_;
                pump_hp_ = pump_hp_+3 < 18 ? pump_hp_+3 : 18;
                if (!components_ || pump_hp_ == 18) Pop(id);
            } else if (!StepToward(id,pump_x_,pump_y_,false)) Pop(id);
        } else if (o.type == OrderType::Resupply) {
            if (!ammo_stock_ || u.ammo == AmmoCap(u.role)) Pop(id);
            else if (Distance(u.x,u.y,crate_x_,crate_y_) <= 1) {
                --ammo_stock_;
                ++u.ammo;
                if (!ammo_stock_ || u.ammo == AmmoCap(u.role)) Pop(id);
            } else if (!StepToward(id,crate_x_,crate_y_,false)) Pop(id);
        } else Pop(id);
    }
    void ExecuteEnemy(int id) {
        Unit &u = units_[id];
        // A Scrapline salvager physically steals one existing depot round at
        // a time. The ledger shrinks and the unit's carried ammo increases;
        // no remote or infinite replenishment is permitted.
        if(doctrine_==Doctrine::Scrapline && u.role==Class::Salvager &&
           ammo_stock_>0 && u.ammo<AmmoCap(u.role) &&
           Distance(u.x,u.y,crate_x_,crate_y_)<=1) {
            --ammo_stock_;++u.ammo;++stolen_rounds_;return;
        }
        int choice = -1, priority = 999;
        for (int j=0; j<count_; ++j) {
            if (units_[j].side != Side::Hearthwell || !CanShoot(id,j)) continue;
            const Unit &target=units_[j];
            const int distance=Distance(u.x,u.y,target.x,target.y);
            // Every shot obeys the SAME LOS, range, cover, armor, ammo
            // consumption and damage rules as a player-issued shot.
            // Organized doctrine troops know not to waste ammunition into
            // entrenched armor while an exposed objective specialist stands
            // nearby. Advanced armies also finish actually wounded targets;
            // no omniscient pursuit, bonus health or fabricated damage.
            int score=distance*4;
            if(doctrine_!=Doctrine::Unaligned){
                const int cover=ground(target.x,target.y)==Ground::Rubble?2:0;
                score+=cover*5+Armor(target.role)*2+FormationArmor(j)*2;
                const int minimum=BaseDamage(u.role)-Armor(target.role)-
                                  cover-FormationArmor(j);
                if(target.hp<=minimum)score-=16; // guaranteed finishing shot
                else if(target.hp<=minimum+2)score-=6; // plausible casualty
                if(mission_==Mission::DisableRelay &&
                   target.role==Class::Engineer)score-=8;
                if(doctrine_==Doctrine::Meridian &&
                   target.role==Class::Engineer)score-=28;
            }
            if(score<priority){priority=score;choice=j;}
        }
        if (choice >= 0) { Shoot(id,choice); return; }
        if(doctrine_==Doctrine::Scrapline && u.role==Class::Salvager &&
           ammo_stock_>0 && u.ammo<AmmoCap(u.role)) {
            StepToward(id,crate_x_,crate_y_,false);return;
        }
        // Roman infantry is strictly melee, can strike without fabricated
        // ammunition, and pursues only defenders actually seen nearby.
        if(doctrine_==Doctrine::LostNinth && u.role==Class::Legionary) {
            int seen=-1,nearest=999;
            for(int j=0;j<count_;++j){
                const Unit &v=units_[j];
                const int distance=Distance(u.x,u.y,v.x,v.y);
                if(v.side==Side::Hearthwell && v.alive() && distance<=8 &&
                   distance<nearest && Visible(u.x,u.y,v.x,v.y)){
                    nearest=distance;seen=j;
                }
            }
            if(seen>=0){StepToward(id,units_[seen].x,units_[seen].y,false);return;}
        }
        if(!u.ammo && u.role!=Class::Legionary)return;

        if(mission_==Mission::CaptureOrchard){
            // Hearthward watch defends its orchard; it does not arbitrarily
            // charge the remote waterworks from a different map.
            if(Distance(u.x,u.y,objective_x(),objective_y())>2)
                StepToward(id,objective_x(),objective_y(),false);
            return;
        }
        if(mission_==Mission::DisableRelay && u.role==Class::Engineer){
            // The receivership radio engineer guards the physical field
            // relay instead of becoming another pump-seeking raider.
            if(Distance(u.x,u.y,objective_x(),objective_y())>1)
                StepToward(id,objective_x(),objective_y(),false);
            return;
        }
        if (ShootPump(id)) return;
        StepToward(id,pump_x_,pump_y_,false);
    }
};

}}  // namespace reverie::omega
#endif
