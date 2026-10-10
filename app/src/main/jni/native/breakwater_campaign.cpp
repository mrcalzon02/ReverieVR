#include "breakwater_campaign.h"
#include <algorithm>
#include <cmath>

namespace reverie { namespace breakwater {
namespace {
constexpr std::array<Location, Campaign::kLocations> kPlaces = {{
    {"Shingle Bay",2}, {"Iron Quay",3}, {"Blackcap Island",4}
}};
constexpr int kCost[] = {180,200,160,750,260,420,470,140};
constexpr int kExtra[] = {120,140,100,0,0,0,0,0};
bool Valid(Upgrade u) { return static_cast<uint8_t>(u) <= 7; }
uint8_t Tier(Difficulty d) { return static_cast<uint8_t>(d); }
} // namespace
const std::array<Location,Campaign::kLocations> &Campaign::Locations() {
    return kPlaces;
}
Campaign::Campaign(Difficulty difficulty, uint32_t seed)
    : difficulty_(Tier(difficulty) <= 3 ? difficulty : Difficulty::Regular),
      seed_(seed) {}
uint32_t Campaign::Hash(uint32_t tag) const {
    uint32_t x = seed_ ^ tag ^ (static_cast<uint32_t>(location_) * 173u)
        ^ (static_cast<uint32_t>(day_) * 511u)
        ^ (static_cast<uint32_t>(period_) * 1511u)
        ^ (static_cast<uint32_t>(wave_index_) * 65537u);
    x ^= x >> 16; x *= 0x7feb352du;
    x ^= x >> 15; x *= 0x846ca68bu;
    return x ^ (x >> 16);
}
uint8_t Campaign::waves_this_period() const {
    const uint8_t difficulty = Tier(difficulty_);
    return static_cast<uint8_t>(std::min<unsigned>(
        8u, (period_ == Period::Day ? 2u : 1u) + difficulty +
        static_cast<unsigned>((day_ - 1u)/2u) + (location_ > 0 ? 1u : 0u)));
}
Stats Campaign::stats() const {
    Stats s;
    s.fire_delay_ms = static_cast<uint16_t>(300 - 35 * upgrades_[0]);
    s.gun_damage = static_cast<uint16_t>(20 + 10 * upgrades_[1]);
    s.magazine = static_cast<uint16_t>(12 + 4 * upgrades_[2]);
    s.anti_air = upgrades_[3] > 0;
    s.turret = upgrades_[6] > 0;
    return s;
}
uint8_t Campaign::level(Upgrade u) const {
    return Valid(u) ? upgrades_[static_cast<uint8_t>(u)] : 0;
}
int32_t Campaign::price(Upgrade u) const {
    if (!Valid(u)) return -1;
    const auto i = static_cast<uint8_t>(u);
    if (u == Upgrade::Repair && integrity_ == 100) return -1;
    if ((u == Upgrade::FireRate || u == Upgrade::Damage ||
         u == Upgrade::Magazine) && upgrades_[i] >= 4) return -1;
    if ((u == Upgrade::AntiAir || u == Upgrade::SupportTurret) &&
        upgrades_[i] != 0) return -1;
    if (u == Upgrade::Artillery && artillery_charges_ >= 3) return -1;
    if (u == Upgrade::Airstrike && airstrike_charges_ >= 3) return -1;
    return kCost[i] + kExtra[i] * upgrades_[i];
}
bool Campaign::Start() {
    if (phase_ != Phase::Briefing) return false;
    phase_ = Phase::Shop; return true;
}
bool Campaign::Purchase(Upgrade u) {
    if (phase_ != Phase::Shop) return false;
    const int32_t cost = price(u);
    if (cost < 0 || credits_ < cost) return false;
    credits_ -= cost;
    if (u == Upgrade::Artillery) ++artillery_charges_;
    else if (u == Upgrade::Airstrike) ++airstrike_charges_;
    else if (u == Upgrade::Repair) integrity_ = std::min(100, integrity_ + 25);
    else ++upgrades_[static_cast<uint8_t>(u)];
    return true;
}
uint8_t Campaign::NextCount() const {
    return static_cast<uint8_t>(std::min<unsigned>(10u, 2u + Tier(difficulty_) +
         location_ + (day_ - 1u) + (period_ == Period::Night ? 1u : 0u)));
}
bool Campaign::BeginWave() {
    if (phase_ != Phase::Shop) return false;
    enemies_.fill({});
    wave_status_ = {};
    infantry_ = 0;
    spawned_ = 0;
    remaining_to_spawn_ = NextCount();
    spawn_timer_ = 0;
    fire_timer_ = 0;
    reload_timer_ = 0;
    infantry_timer_ = 0;
    turret_timer_ = 0;
    magazine_left_ = static_cast<uint8_t>(stats().magazine);
    phase_ = Phase::Combat;
    return true;
}
void Campaign::Land(Enemy &e) {
    e.active = false;
    e.landed = true;
    infantry_ = static_cast<uint8_t>(infantry_ + e.passengers);
    wave_status_.troops_landed = static_cast<uint8_t>(
        wave_status_.troops_landed + e.passengers);
}
bool Campaign::FireAt(uint8_t index) {
    if (phase_ != Phase::Combat || index >= enemies_.size() ||
        !enemies_[index].active || fire_timer_ > 0 ||
        reload_timer_ > 0 || magazine_left_ == 0) return false;
    Enemy &e = enemies_[index];
    if (e.type == UnitType::Aircraft && !stats().anti_air) return false;
    --magazine_left_;
    fire_timer_ = stats().fire_delay_ms / 1000.0f;
    if (magazine_left_ == 0) reload_timer_ = 1.15f;
    e.hp = static_cast<int16_t>(e.hp - stats().gun_damage);
    if (e.hp <= 0) {
        e.active = false;
        credits_ += e.type == UnitType::Aircraft ? 115 : 55;
        if (e.type == UnitType::Aircraft) ++wave_status_.aircraft_destroyed;
        else ++wave_status_.boats_sunk;
        // Critically: a sunk boat does not create disembarked infantry.
    }
    return true;
}
bool Campaign::FireAtInfantry() {
    if (phase_ != Phase::Combat || infantry_ == 0 || fire_timer_ > 0 ||
        reload_timer_ > 0 || magazine_left_ == 0) return false;
    --magazine_left_;
    fire_timer_ = stats().fire_delay_ms / 1000.0f;
    if (magazine_left_ == 0) reload_timer_ = 1.15f;
    --infantry_;
    ++wave_status_.troops_stopped;
    credits_ += 20;
    return true;
}
bool Campaign::CallArtillery() {
    if (phase_ != Phase::Combat || artillery_charges_ == 0) return false;
    --artillery_charges_;
    for (Enemy &e : enemies_) {
        if (e.active && e.type != UnitType::Aircraft) {
            e.hp = static_cast<int16_t>(e.hp - 120);
            if (e.hp <= 0) { e.active = false; ++wave_status_.boats_sunk; credits_ += 55; }
        }
    }
    return true;
}
bool Campaign::CallAirstrike() {
    if (phase_ != Phase::Combat || airstrike_charges_ == 0) return false;
    --airstrike_charges_;
    for (Enemy &e : enemies_) {
        if (e.active) {
            e.hp = static_cast<int16_t>(e.hp - 150);
            if (e.hp <= 0) {
                e.active = false; credits_ += 75;
                if (e.type == UnitType::Aircraft) ++wave_status_.aircraft_destroyed;
                else ++wave_status_.boats_sunk;
            }
        }
    }
    return true;
}
void Campaign::Tick(float seconds) {
    if (phase_ != Phase::Combat || !std::isfinite(seconds) || seconds <= 0) return;
    const float dt = std::min(seconds, 0.05f);
    fire_timer_ = std::max(0.0f, fire_timer_ - dt);
    if (reload_timer_ > 0) {
        reload_timer_ -= dt;
        if (reload_timer_ <= 0) {
            reload_timer_ = 0;
            magazine_left_ = static_cast<uint8_t>(stats().magazine);
        }
    }
    spawn_timer_ -= dt;
    if (remaining_to_spawn_ && spawn_timer_ <= 0) {
        for (Enemy &e : enemies_) {
            if (e.active) continue;
            const uint32_t choice = Hash(0x1234u + spawned_ * 313u);
            const unsigned progression = location_ * 3u + (day_ - 1u) * 2u +
                (period_ == Period::Night ? 1u : 0u) + Tier(difficulty_);
            // Opening day: rowboats with exactly one passenger only.
            const UnitType kind = progression < 2u ? UnitType::Rowboat :
                (progression >= 8u && (choice % 6u == 0u)) ? UnitType::Aircraft :
                (progression >= 5u && (choice % 4u == 0u)) ? UnitType::Gunboat :
                (progression >= 3u && (choice % 3u == 0u)) ? UnitType::LandingCraft :
                UnitType::Rowboat;
            const int hp = kind == UnitType::Rowboat ? 20 :
                kind == UnitType::LandingCraft ? 85 :
                kind == UnitType::Gunboat ? 125 : 100;
            e = {kind, (static_cast<int>(choice % 9u) - 4) * 0.65f,
                 -11.0f, static_cast<int16_t>(hp),
                 static_cast<uint8_t>(kind == UnitType::Rowboat ? 1 :
                 kind == UnitType::LandingCraft ? 3 + (choice >> 8u) % 4u : 0),
                 true, false};
            --remaining_to_spawn_;
            ++spawned_;
            spawn_timer_ = 1.6f;
            break;
        }
    }
    // An actually purchased support turret automatically engages the nearest
    // surviving surface vessel; it never silently shoots down aircraft.
    if (stats().turret) {
        turret_timer_ -= dt;
        if (turret_timer_ <= 0.0f) {
            Enemy *target = nullptr;
            for (Enemy &candidate : enemies_) {
                if (candidate.active && candidate.type != UnitType::Aircraft &&
                    (target == nullptr || candidate.z > target->z)) {
                    target = &candidate;
                }
            }
            if (target != nullptr) {
                target->hp = static_cast<int16_t>(target->hp - 15);
                if (target->hp <= 0) {
                    target->active = false;
                    ++wave_status_.boats_sunk;
                    credits_ += 55;
                }
                turret_timer_ = 1.35f;
            }
        }
    }
    for (Enemy &e : enemies_) {
        if (!e.active) continue;
        const float speed = e.type == UnitType::Rowboat ? 0.9f :
            e.type == UnitType::LandingCraft ? 0.62f :
            e.type == UnitType::Gunboat ? 0.76f : 1.4f;
        e.z += speed * dt;
        if (e.z >= -2.2f) {
            if (e.type == UnitType::Aircraft || e.type == UnitType::Gunboat) {
                integrity_ = std::max(0, integrity_ -
                    (e.type == UnitType::Aircraft ? 12 : 8));
                e.active = false;
            } else Land(e);
        }
    }
    infantry_timer_ += dt;
    if (infantry_ && infantry_timer_ >= 4.0f) {
        infantry_timer_ = 0;
        integrity_ = std::max(0, integrity_ - infantry_ * 2);
    }
    if (integrity_ == 0) { phase_ = Phase::Defeat; return; }
    if (FinishedWave()) CompleteWave();
}
bool Campaign::FinishedWave() const {
    if (remaining_to_spawn_ || infantry_) return false;
    for (const Enemy &e : enemies_) if (e.active) return false;
    return true;
}
void Campaign::CompleteWave() {
    credits_ += 120 + (wave_status_.troops_landed == 0 ? 100 : 0);
    if (wave_index_ < waves_this_period()) ++wave_index_;
    else {
        wave_index_ = 1;
        if (period_ == Period::Day) period_ = Period::Night;
        else {
            period_ = Period::Day;
            if (day_ < kPlaces[location_].days) ++day_;
            else if (location_ + 1 < kLocations) { ++location_; day_ = 1; }
            else { phase_ = Phase::Victory; return; }
        }
    }
    phase_ = Phase::Shop;
}
}} // namespace reverie::breakwater
