#ifndef REVERIE_BREAKWATER_CAMPAIGN_H
#define REVERIE_BREAKWATER_CAMPAIGN_H
#include <array>
#include <cstddef>
#include <cstdint>
#include "breakwater_weapons.h"

namespace reverie { namespace breakwater {
enum class Difficulty : uint8_t { Casual, Regular, Veteran, Siege };
enum class Phase : uint8_t { Briefing, Shop, Combat, Victory, Defeat };
enum class Period : uint8_t { Day, Night };
enum class UnitType : uint8_t { Rowboat, LandingCraft, Gunboat, Aircraft };
enum class AircraftPattern : uint8_t { None, Strafer, Bomber };
// Shared spawn health also drives damage visualization. Do not duplicate
// balance values in renderer-only code, HUDs, or saved-state adapters.
constexpr int16_t EnemyStartingHp(UnitType type) {
    switch(type) {
        case UnitType::Rowboat: return 20;
        case UnitType::LandingCraft: return 85;
        case UnitType::Gunboat: return 125;
        case UnitType::Aircraft: return 100;
        default: return 0;
    }
}
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
    // A landing craft can carry one independently targetable armored car.
    // The cargo never becomes active unless its transport actually beaches.
    bool carries_armor = false;
    AircraftPattern aircraft_pattern = AircraftPattern::None;
    uint8_t attacks_remaining = 0;
    float attack_timer = 0;
    // Spawn-seeded lateral approach, recomputed from forward progress.
    // No accumulation or RNG per eye/frame and no new save schema fields.
    float flight_center_x = 0;
    float flight_sign = 1;
    float bank = 0;
};
enum class InfantryTactic : uint8_t {
    Rifle, FlankLeft, FlankRight, Support
};
struct Infantry {
    float x = 0;
    float z = 0;
    bool active = false;
    // A deterministic landing-craft assignment. Only transient combat
    // state: the campaign checkpoint can still save shop state in 80 bytes.
    InfantryTactic tactic = InfantryTactic::Rifle;
    float lane_x = 0;
    // Battle-only cover order. The original lane remains the fallback
    // destination if the associated barricade gets destroyed.
    int8_t cover_slot = -1;
    bool regrouping = false;
    float open_lane_x = 0;
    float cover_line_z = 0;
    // Transient battle-only firing suppression; never part of shop saves.
    float pinned_seconds = 0.0f;
};
struct ArmoredVehicle {
    float x = 0.0f;
    float z = 0.0f;
    int16_t hp = 0;
    bool active = false;
    float attack_timer = 0.0f;
};
struct ArmorShotEvent {
    uint32_t sequence = 0;
    float x = 0.0f, z = 0.0f;
    uint8_t damage = 0;
};
struct ShoreCover {
    float x = 0.0f;
    float z = 0.0f;
    int16_t hp = 0;
    int16_t max_hp = 0;
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
    float projectile_gravity = .014f;
};
struct IncomingThreats {
    // -1 means no eligible attacker can fire before leaving its attack window.
    float aircraft_seconds = -1.0f;
    float gunboat_seconds = -1.0f;
    // Earliest actual infantry salvo, including travel to the firing line.
    // -1 means no living disembarked soldier can fire yet.
    float infantry_seconds = -1.0f;
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
    uint8_t damage = 0; // actual hull loss, not nominal volley strength
    uint64_t firing_mask = 0; // exact ready slots at this one salvo substep
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
// A surviving gunboat reaching shore inflicts separate collision damage.
// This is not a gun salvo; its effect belongs at the vessel's last position.
struct GunboatCollisionEvent {
    uint32_t sequence = 0;
    float x = 0, z = 0;
    uint8_t damage = 0; // actual integrity loss, not nominal ram damage
};
struct Status {
    uint8_t boats_sunk = 0;
    uint8_t troops_landed = 0;
    uint8_t troops_stopped = 0;
    uint8_t aircraft_destroyed = 0;
    uint8_t armor_landed = 0;
    uint8_t armor_destroyed = 0;
    uint8_t armor_attacks = 0;
    uint8_t aircraft_attacks = 0;
    uint8_t gunboat_attacks = 0;
};
class Campaign {
public:
    static constexpr size_t kSerializedSize = 80;
    static constexpr size_t kMaxEnemies = 12;
    static constexpr size_t kMaxInfantry = 60; // ten transports x six passengers
    static constexpr size_t kMaxCover = 4;
    static constexpr size_t kMaxArmor = 10; // at most ten transports per wave // tiny location-specific barricades
    static constexpr uint8_t kLocations = 3;
    static const std::array<Location, kLocations> &Locations();
    explicit Campaign(Difficulty difficulty = Difficulty::Regular,
                      uint32_t seed = 1);
    bool Start();
    bool SerializeShop(uint8_t *buffer, size_t capacity) const;
    bool DeserializeShop(const uint8_t *buffer, size_t size);
    bool BeginWave();
    bool Purchase(Upgrade upgrade);
    bool PurchaseWeapon(WeaponKind weapon);
    bool SelectWeapon(WeaponKind weapon);
    bool PurchaseSam();
    bool CallSam(uint8_t enemy_index);
    bool owns_weapon(WeaponKind weapon) const {
        return (owned_weapons_ & (1u << static_cast<uint8_t>(weapon))) != 0;
    }
    WeaponKind selected_weapon() const {
        return static_cast<WeaponKind>(selected_weapon_);
    }
    uint8_t sam_charges() const { return sam_charges_; }
    bool FireAt(uint8_t index);
    bool FireAtInfantry(uint8_t index);
    bool FireAtCover(uint8_t index);
    bool FireAtArmor(uint8_t index);
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
    GunboatCollisionEvent gunboat_collision() const { return gunboat_collision_; }
    ArmorShotEvent armor_shot() const { return armor_shot_; }
    IncomingThreats incoming_threats() const;
    const std::array<Enemy,kMaxEnemies> &enemies() const { return enemies_; }
    const std::array<Infantry,kMaxInfantry> &infantry_units() const { return infantry_units_; }
    const std::array<ShoreCover,kMaxCover> &shore_cover() const { return shore_cover_; }
    const std::array<ArmoredVehicle,kMaxArmor> &armored_vehicles() const { return armor_; }
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
    // Save v2 reuses three unused fields in the original 80-byte envelope;
    // v1 saves are still accepted without requiring a migration file.
    uint8_t owned_weapons_ = 1;
    uint8_t selected_weapon_ = 0;
    uint8_t sam_charges_ = 0;
    std::array<uint8_t,kWeaponCount> weapon_magazines_{{12,8,6,2,4}};
    std::array<Enemy,kMaxEnemies> enemies_{};
    std::array<Infantry,kMaxInfantry> infantry_units_{};
    std::array<ShoreCover,kMaxCover> shore_cover_{};
    std::array<ArmoredVehicle,kMaxArmor> armor_{};
    uint8_t armor_spawn_cursor_ = 0;
    uint8_t infantry_spawn_cursor_ = 0;
    Status wave_status_{};
    TurretShotEvent turret_shot_{};
    InfantryVolleyEvent infantry_volley_{};
    AircraftAttackEvent aircraft_attack_{};
    GunboatSalvoEvent gunboat_salvo_{};
    GunboatCollisionEvent gunboat_collision_{};
    ArmorShotEvent armor_shot_{};
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
