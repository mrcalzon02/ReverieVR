#ifndef REVERIE_BREAKWATER_CAMPAIGN_H
#define REVERIE_BREAKWATER_CAMPAIGN_H
#include <array>
#include <cstddef>
#include <cstdint>

namespace reverie { namespace breakwater {
enum class Difficulty : uint8_t { Casual, Regular, Veteran, Siege };
enum class Phase : uint8_t { Briefing, Shop, Combat, Victory, Defeat };
enum class Period : uint8_t { Day, Night };
enum class UnitType : uint8_t { Rowboat, LandingCraft, Gunboat, Aircraft };
enum class AircraftPattern : uint8_t { None, Strafer, Bomber };
enum class Upgrade : uint8_t {
    FireRate, Damage, Magazine, AntiAir, Artillery, Airstrike, SupportTurret, Repair
};
struct Location {
    const char *name;
    uint8_t days;
};
struct Enemy {
    UnitType type = UnitType::Rowboat;
    float x = 0;
    float z = 0;
    int16_t hp = 0;
    uint8_t passengers = 0;
    bool active = false;
    bool landed = false;
    AircraftPattern aircraft_pattern = AircraftPattern::None;
    uint8_t attacks_remaining = 0;
    float attack_timer = 0;
    // Spawn-seeded lateral approach, recomputed from forward progress.
    // No accumulation or RNG per eye/frame and no new save schema fields.
    float flight_center_x = 0;
    float flight_sign = 1;
    float bank = 0;
};
struct Infantry {
    float x = 0;
    float z = 0;
    bool active = false;
};
struct AircraftFlightPose {
    float x = 0;
    float bank = 0; // radians; wing tips rotate about the fuselage
};
// Pure, deterministic flight path shared by simulation and native tests.
// z is authoritative forward progress from the seeded -11 spawn.
AircraftFlightPose AircraftFlightAt(AircraftPattern pattern,
                                    float center_x, float sign, float z);
// Shared by the pure-C++ tests and the headset's combat ray.
// Returns the first positive surface intersection, or the exit if inside.
// Supports finite non-unit ray directions and rejects malformed input.
bool RaySphereSurfaceDistance(const float *origin, const float *direction,
                              const float *center, float radius, float *distance);
bool RayHitsInfantry(const Infantry &soldier, const float *origin,
                     const float *direction, float *distance);
struct Stats {
    uint16_t fire_delay_ms = 300;
    uint16_t gun_damage = 20;
    // Additional damage applies to aircraft only after purchasing AA tier 2/3.
    uint16_t anti_air_damage_bonus = 0;
    uint16_t magazine = 12;
    bool anti_air = false;
    bool turret = false;
};
struct IncomingThreats {
    // -1 means no eligible attacker can fire before leaving its attack window.
    float aircraft_seconds = -1.0f;
    float gunboat_seconds = -1.0f;
};
struct TurretShotEvent {
    uint32_t sequence = 0;
    float x = 0;
    float z = 0;
    bool destroyed = false;
};
// Emitted only when front-line infantry actually damage the battery.
// The location belongs to one of the living shooters, not an inferred flash.
struct InfantryVolleyEvent {
    uint32_t sequence = 0;
    float x = 0;
    float z = 0;
    uint8_t shooters = 0;
};
// The simulation emits this only when an aircraft deals real hull damage.
struct AircraftAttackEvent {
    uint32_t sequence = 0;
    float x = 0, z = 0;
    AircraftPattern pattern = AircraftPattern::None;
    uint8_t damage = 0;
};
struct GunboatSalvoEvent {
    uint32_t sequence = 0;
    float x = 0, z = 0; // firing vessel position at the actual salvo
    uint8_t damage = 0; // actual hull loss, clamped at zero integrity
};
struct Status {
    uint8_t boats_sunk = 0;
    uint8_t troops_landed = 0;
    uint8_t troops_stopped = 0;
    uint8_t aircraft_destroyed = 0;
    uint8_t aircraft_attacks = 0;
    uint8_t gunboat_attacks = 0;
};
class Campaign {
public:
    static constexpr size_t kSerializedSize = 80;
    static constexpr size_t kMaxEnemies = 12;
    static constexpr size_t kMaxInfantry = 60; // ten transports x six passengers
    static constexpr uint8_t kLocations = 3;
    static const std::array<Location, kLocations> &Locations();
    explicit Campaign(Difficulty difficulty = Difficulty::Regular,
                      uint32_t seed = 1);
    bool Start();
    bool SerializeShop(uint8_t *buffer, size_t capacity) const;
    bool DeserializeShop(const uint8_t *buffer, size_t size);
    bool BeginWave();
    bool Purchase(Upgrade upgrade);
    bool FireAt(uint8_t index);
    bool FireAtInfantry(uint8_t index);
    bool FireMiss();
    bool RequestReload();
    bool CallArtillery();
    bool CallAirstrike();
    void Tick(float seconds);
    Phase phase() const { return phase_; }
    Difficulty difficulty() const { return difficulty_; }
    Period period() const { return period_; }
    uint8_t location() const { return location_; }
    uint8_t day() const { return day_; }
    uint8_t wave_index() const { return wave_index_; }
    uint8_t waves_this_period() const;
    int32_t credits() const { return credits_; }
    int32_t integrity() const { return integrity_; }
    uint8_t infantry() const { return infantry_; }
    uint8_t magazine_left() const { return magazine_left_; }
    bool reloading() const { return phase_ == Phase::Combat && reload_timer_ > 0.0f; }
    float reload_fraction() const;
    uint8_t artillery_charges() const { return artillery_charges_; }
    uint8_t airstrike_charges() const { return airstrike_charges_; }
    Stats stats() const;
    Status wave_status() const { return wave_status_; }
    TurretShotEvent turret_shot() const { return turret_shot_; }
    InfantryVolleyEvent infantry_volley() const { return infantry_volley_; }
    AircraftAttackEvent aircraft_attack() const { return aircraft_attack_; }
    GunboatSalvoEvent gunboat_salvo() const { return gunboat_salvo_; }
    IncomingThreats incoming_threats() const;
    const std::array<Enemy,kMaxEnemies> &enemies() const { return enemies_; }
    const std::array<Infantry,kMaxInfantry> &infantry_units() const { return infantry_units_; }
    int32_t price(Upgrade upgrade) const;
    uint8_t level(Upgrade upgrade) const;
private:
    uint32_t Hash(uint32_t tag) const;
    bool BeginGunShot();
    void TickStep(float dt);
    bool FinishedWave() const;
    void CompleteWave();
    void Land(Enemy &enemy);
    uint8_t NextCount() const;
    Difficulty difficulty_;
    uint32_t seed_;
    Phase phase_ = Phase::Briefing;
    Period period_ = Period::Day;
    uint8_t location_ = 0;
    uint8_t day_ = 1;
    uint8_t wave_index_ = 1;
    int32_t credits_ = 1300;
    int32_t integrity_ = 100;
    uint8_t infantry_ = 0;
    uint8_t magazine_left_ = 12;
    uint8_t artillery_charges_ = 0;
    uint8_t airstrike_charges_ = 0;
    std::array<uint8_t,8> upgrades_{};
    std::array<Enemy,kMaxEnemies> enemies_{};
    std::array<Infantry,kMaxInfantry> infantry_units_{};
    uint8_t infantry_spawn_cursor_ = 0;
    Status wave_status_{};
    TurretShotEvent turret_shot_{};
    InfantryVolleyEvent infantry_volley_{};
    AircraftAttackEvent aircraft_attack_{};
    GunboatSalvoEvent gunboat_salvo_{};
    uint8_t remaining_to_spawn_ = 0;
    uint8_t spawned_ = 0;
    float spawn_timer_ = 0;
    float fire_timer_ = 0;
    float reload_timer_ = 0;
    float infantry_timer_ = 0;
    float turret_timer_ = 0;
};
}}  // namespace reverie::breakwater
#endif
