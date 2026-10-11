#include "breakwater_campaign.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace reverie { namespace breakwater {
namespace {
constexpr std::array<Location, Campaign::kLocations> kPlaces = {{
    {"Shingle Bay",2}, {"Iron Quay",3}, {"Blackcap Island",4}
}};
constexpr int kCost[] = {180,200,160,750,260,420,470,140};
constexpr int kExtra[] = {120,140,100,0,0,0,0,0};
bool Valid(Upgrade u) { return static_cast<uint8_t>(u) <= 7; }
uint8_t Tier(Difficulty d) { return static_cast<uint8_t>(d); }
static_assert(Campaign::kMaxInfantry >= 10u * 6u,
              "All landing passengers must receive unique soldier slots");
} // namespace
const std::array<Location,Campaign::kLocations> &Campaign::Locations() {
    return kPlaces;
}
Campaign::Campaign(Difficulty difficulty, uint32_t seed)
    : difficulty_(Tier(difficulty) <= 3 ? difficulty : Difficulty::Regular),
      seed_(seed) {}
bool RaySphereSurfaceDistance(const float *origin, const float *direction,
                              const float *center, float radius, float *distance) {
    if (!origin || !direction || !center || !distance ||
        !std::isfinite(radius) || radius <= 0.0f) return false;
    // Double intermediates keep the discriminant stable for finite float rays.
    // Do not rely on unit length: the input sanitizer normally normalizes,
    // but host-side tests and future callers may not.
    double a = 0.0, b = 0.0, c = -static_cast<double>(radius) * radius;
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(origin[i]) || !std::isfinite(direction[i]) ||
            !std::isfinite(center[i])) return false;
        const double offset = static_cast<double>(origin[i]) - center[i];
        const double ray = direction[i];
        a += ray * ray;
        b += offset * ray;
        c += offset * offset;
    }
    if (a <= 0.0 || !std::isfinite(a) || !std::isfinite(b) ||
        !std::isfinite(c)) return false;
    const double discriminant = b * b - a * c;
    if (!std::isfinite(discriminant) || discriminant < 0.0) return false;
    const double root = std::sqrt(discriminant);
    double t = (-b - root) / a;
    if (t <= 0.0) t = (-b + root) / a; // origin inside the hitbox
    if (!std::isfinite(t) || t <= 0.0 ||
        t > std::numeric_limits<float>::max()) return false;
    *distance = static_cast<float>(t);
    return true;
}
bool RayHitsInfantry(const Infantry &soldier, const float *origin,
                     const float *direction, float *distance) {
    if (!soldier.active) return false;
    const float center[] = {soldier.x, -0.66f, soldier.z};
    return RaySphereSurfaceDistance(origin, direction, center, 0.30f, distance);
}
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
bool Campaign::SerializeShop(uint8_t *out, size_t capacity) const {
    if (phase_ != Phase::Shop || out == nullptr || capacity < kSerializedSize)
        return false;
    const uint32_t words[20] = {
        0x31574252u, 1u, seed_, static_cast<uint32_t>(difficulty_),
        location_, day_, static_cast<uint32_t>(period_), wave_index_,
        static_cast<uint32_t>(credits_), static_cast<uint32_t>(integrity_),
        artillery_charges_, airstrike_charges_,
        upgrades_[0], upgrades_[1], upgrades_[2], upgrades_[3],
        upgrades_[4], upgrades_[5], upgrades_[6], upgrades_[7]
    };
    for (size_t i = 0; i < 20; ++i) {
        const uint32_t value = words[i];
        for (size_t byte = 0; byte < 4; ++byte)
            out[i * 4u + byte] = static_cast<uint8_t>(value >> (byte * 8u));
    }
    return true;
}

bool Campaign::DeserializeShop(const uint8_t *source, size_t length) {
    if (source == nullptr || length != kSerializedSize) return false;
    uint32_t data[20] = {};
    for (size_t i = 0; i < 20; ++i) {
        for (size_t byte = 0; byte < 4; ++byte)
            data[i] |= static_cast<uint32_t>(source[i * 4u + byte]) << (byte * 8u);
    }
    if (data[0] != 0x31574252u || data[1] != 1u ||
        data[3] > 3u || data[4] >= kLocations || data[5] == 0 ||
        data[5] > kPlaces[data[4]].days || data[6] > 1u ||
        data[7] == 0 || data[8] > 10000000u ||
        data[9] == 0 || data[9] > 100u ||
        data[10] > 3u || data[11] > 3u ||
        data[12] > 4u || data[13] > 4u || data[14] > 4u ||
        data[15] > 1u || data[16] != 0u || data[17] != 0u ||
        data[18] > 1u || data[19] != 0u) return false;
    Campaign candidate(static_cast<Difficulty>(data[3]), data[2]);
    candidate.phase_ = Phase::Shop;
    candidate.location_ = static_cast<uint8_t>(data[4]);
    candidate.day_ = static_cast<uint8_t>(data[5]);
    candidate.period_ = static_cast<Period>(data[6]);
    candidate.wave_index_ = static_cast<uint8_t>(data[7]);
    if (candidate.wave_index_ > candidate.waves_this_period()) return false;
    candidate.credits_ = static_cast<int32_t>(data[8]);
    candidate.integrity_ = static_cast<int32_t>(data[9]);
    candidate.artillery_charges_ = static_cast<uint8_t>(data[10]);
    candidate.airstrike_charges_ = static_cast<uint8_t>(data[11]);
    for (size_t i = 0; i < candidate.upgrades_.size(); ++i)
        candidate.upgrades_[i] = static_cast<uint8_t>(data[12u + i]);
    candidate.magazine_left_ = static_cast<uint8_t>(candidate.stats().magazine);
    *this = candidate; // commit only after every validation succeeds
    return true;
}

uint8_t Campaign::NextCount() const {
    return static_cast<uint8_t>(std::min<unsigned>(10u, 2u + Tier(difficulty_) +
         location_ + (day_ - 1u) + (period_ == Period::Night ? 1u : 0u)));
}
bool Campaign::BeginWave() {
    if (phase_ != Phase::Shop) return false;
    enemies_.fill({});
    infantry_units_.fill({});
    infantry_spawn_cursor_ = 0;
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
    // NextCount caps a wave at ten craft, each carrying no more than six.
    // Append-only slots preserve identity when other soldiers are killed.
    for (uint8_t passenger = 0; passenger < e.passengers; ++passenger) {
        Infantry &trooper = infantry_units_[infantry_spawn_cursor_++];
        const float spread = (static_cast<float>(passenger) -
            static_cast<float>(e.passengers - 1u) * 0.5f) * 0.27f;
        trooper.x = std::max(-3.1f, std::min(3.1f, e.x + spread));
        trooper.z = -2.28f - 0.10f * static_cast<float>(passenger % 2u);
        trooper.active = true;
    }
    e.active = false;
    e.landed = true;
    infantry_ = static_cast<uint8_t>(infantry_ + e.passengers);
    wave_status_.troops_landed = static_cast<uint8_t>(
        wave_status_.troops_landed + e.passengers);
}
float Campaign::reload_fraction() const {
    if (!reloading()) return 0.0f;
    // This is display-only progress. The simulation remains the sole owner
    // of magazine refills and reload timing.
    constexpr float kReloadSeconds = 1.15f;
    return std::max(0.0f, std::min(1.0f,
        1.0f - reload_timer_ / kReloadSeconds));
}
bool Campaign::BeginGunShot() {
    if (phase_ != Phase::Combat || fire_timer_ > 0 ||
        reload_timer_ > 0 || magazine_left_ == 0) return false;
    --magazine_left_;
    fire_timer_ = stats().fire_delay_ms / 1000.0f;
    if (magazine_left_ == 0) reload_timer_ = 1.15f;
    return true;
}
bool Campaign::FireMiss() {
    return BeginGunShot();
}
bool Campaign::FireAt(uint8_t index) {
    if (phase_ != Phase::Combat || index >= enemies_.size() ||
        !enemies_[index].active) return false;
    Enemy &e = enemies_[index];
    if (!BeginGunShot()) return false;
    // The basic mount may still throw rounds at an aircraft, but without the
    // AA package those rounds cannot damage the fast overhead target.
    if (e.type == UnitType::Aircraft && !stats().anti_air) return true;
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
bool Campaign::FireAtInfantry(uint8_t index) {
    if (phase_ != Phase::Combat || index >= infantry_spawn_cursor_ ||
        !infantry_units_[index].active) return false;
    if (!BeginGunShot()) return false;
    infantry_units_[index].active = false;
    --infantry_;
    ++wave_status_.troops_stopped;
    credits_ += 20;
    return true;
}
bool Campaign::CallArtillery() {
    if (phase_ != Phase::Combat || artillery_charges_ == 0) return false;
    // A strike is only spent if at least one eligible vessel is present.
    // Troops already ashore and aircraft are not artillery targets.
    const bool target_available = std::any_of(enemies_.begin(), enemies_.end(),
        [](const Enemy &e) { return e.active && e.type != UnitType::Aircraft; });
    if (!target_available) return false;
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
    // Empty-airspace calls must not silently consume purchased support.
    const bool target_available = std::any_of(enemies_.begin(), enemies_.end(),
        [](const Enemy &e) { return e.active; });
    if (!target_available) return false;
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
                (period_ == Period::Night ? 1u : 0u) +
                (Tier(difficulty_) > 0u ? 1u : 0u);
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
            if (kind == UnitType::Aircraft) {
                e.aircraft_pattern = (choice & 0x100u) ?
                    AircraftPattern::Bomber : AircraftPattern::Strafer;
                e.attacks_remaining =
                    e.aircraft_pattern == AircraftPattern::Bomber ? 1u : 4u;
                e.attack_timer = 0.15f;
            } else if (kind == UnitType::Gunboat) {
                // Surface gunboats need a warm-up before their first salvo.
                e.attack_timer = 2.6f;
            }
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
        if (e.type == UnitType::Aircraft) {
            const bool bomber = e.aircraft_pattern == AircraftPattern::Bomber;
            e.z += (bomber ? 1.05f : 1.55f) * dt;
            e.attack_timer = std::max(0.0f, e.attack_timer - dt);
            const float attack_start = bomber ? -5.6f : -8.0f;
            const float attack_end = bomber ? -2.0f : 1.2f;
            if (e.attacks_remaining > 0 && e.z >= attack_start &&
                e.z <= attack_end && e.attack_timer <= 0.0f) {
                integrity_ = std::max(0, integrity_ - (bomber ? 12 : 3));
                --e.attacks_remaining;
                ++wave_status_.aircraft_attacks;
                e.attack_timer = bomber ? 20.0f : 0.72f;
            }
            // Aircraft make a complete overhead pass rather than colliding
            // with the beach. Surviving attackers leave the combat volume.
            if (e.z >= 2.4f) e.active = false;
            continue;
        }
        const float speed = e.type == UnitType::Rowboat ? 0.9f :
            e.type == UnitType::LandingCraft ? 0.62f : 0.76f;
        e.z += speed * dt;
        if (e.type == UnitType::Gunboat) {
            e.attack_timer = std::max(0.0f, e.attack_timer - dt);
            // Unlike troop transports, gunboats threaten the battery from
            // offshore. Sinking one before the firing line prevents its salvos.
            if (e.z >= -8.0f && e.z < -2.2f && e.attack_timer <= 0.0f) {
                integrity_ = std::max(0, integrity_ - 3);
                ++wave_status_.gunboat_attacks;
                e.attack_timer = 2.8f;
            }
        }
        if (e.z >= -2.2f) {
            if (e.type == UnitType::Gunboat) {
                integrity_ = std::max(0, integrity_ - 8);
                e.active = false;
            } else Land(e);
        }
    }
    // Individual soldiers advance toward the parapet between rifle salvos.
    for (Infantry &trooper : infantry_units_)
        if (trooper.active) trooper.z = std::min(-1.34f, trooper.z + 0.13f * dt);
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
