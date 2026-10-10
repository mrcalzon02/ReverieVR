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
};
struct Stats {
    uint16_t fire_delay_ms = 300;
    uint16_t gun_damage = 20;
    uint16_t magazine = 12;
    bool anti_air = false;
    bool turret = false;
};
struct Status {
    uint8_t boats_sunk = 0;
    uint8_t troops_landed = 0;
    uint8_t troops_stopped = 0;
    uint8_t aircraft_destroyed = 0;
};
class Campaign {
public:
    static constexpr size_t kSerializedSize = 80;
    static constexpr size_t kMaxEnemies = 12;
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
    bool FireAtInfantry();
    bool CallArtillery();
    bool CallAirstrike();
    void Tick(float seconds);
    Phase phase() const { return phase_; }
    Period period() const { return period_; }
    uint8_t location() const { return location_; }
    uint8_t day() const { return day_; }
    uint8_t wave_index() const { return wave_index_; }
    uint8_t waves_this_period() const;
    int32_t credits() const { return credits_; }
    int32_t integrity() const { return integrity_; }
    uint8_t infantry() const { return infantry_; }
    uint8_t magazine_left() const { return magazine_left_; }
    uint8_t artillery_charges() const { return artillery_charges_; }
    uint8_t airstrike_charges() const { return airstrike_charges_; }
    Stats stats() const;
    Status wave_status() const { return wave_status_; }
    const std::array<Enemy,kMaxEnemies> &enemies() const { return enemies_; }
    int32_t price(Upgrade upgrade) const;
    uint8_t level(Upgrade upgrade) const;
private:
    uint32_t Hash(uint32_t tag) const;
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
    Status wave_status_{};
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
