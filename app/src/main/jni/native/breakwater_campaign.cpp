#include "breakwater_campaign.h"
#include "breakwater_wave_schedule.h"
#include "breakwater_wave_threat.h"
#include "breakwater_infantry_tactics.h"
#include "breakwater_cover.h"
#include "breakwater_armor.h"
#include "breakwater_invasion_units.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace reverie { namespace breakwater {
namespace {
constexpr std::array<Location, Campaign::kLocations> kPlaces = {{
    {"Shingle Bay",2}, {"Iron Quay",3}, {"Blackcap Island",4}
}};
constexpr int kCost[] = {180,200,160,750,260,420,470,140};
constexpr int kExtra[] = {120,140,100,300,0,0,0,0};
constexpr float kReloadSeconds = 1.15f;
constexpr float kGunboatSpeed = .76f;
constexpr float kBomberSpeed = 1.05f, kStraferSpeed = 1.55f;
constexpr float kBomberAttackStart = -5.6f, kStraferAttackStart = -8.0f;
constexpr float kBomberAttackEnd = -2.0f, kStraferAttackEnd = 1.2f;
constexpr float kGunboatAttackStart = -8.0f, kGunboatAttackEnd = -2.2f;
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
AircraftFlightPose AircraftFlightAt(AircraftPattern pattern,
                                    float center_x, float sign, float z) {
    AircraftFlightPose pose{};
    if (!std::isfinite(center_x) || !std::isfinite(sign) ||
        !std::isfinite(z) || (sign != 1.0f && sign != -1.0f)) return pose;
    pose.x = center_x;
    if (pattern != AircraftPattern::Bomber &&
        pattern != AircraftPattern::Strafer) return pose;
    // A bomber glides with gentle course correction; a fast strafer
    // snakes across the gun's sightline. Travel, not frame count, drives
    // lateral motion so a slow frame and split frames agree exactly.
    const float amplitude = pattern == AircraftPattern::Bomber ? .24f : .76f;
    const float frequency = pattern == AircraftPattern::Bomber ? .29f : .52f;
    const float travel = std::max(0.0f,z + 11.0f);
    const float phase = travel * frequency;
    pose.x = center_x + sign * amplitude * std::sin(phase);
    pose.bank = std::max(-.28f,std::min(.28f,
        -sign * amplitude * frequency * std::cos(phase) * .62f));
    return pose;
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
    // One schedule owns both the campaign's progress and its native tests.
    return waves::ForPeriod(location_,day_,period_==Period::Night,
                            Tier(difficulty_));
}
Stats Campaign::stats() const {
    Stats s;
    const WeaponSpec spec=WeaponFor(selected_weapon());
    s.fire_delay_ms = static_cast<uint16_t>(std::max<int>(
        90, static_cast<int>(spec.fire_delay_ms)-35*upgrades_[0]));
    s.gun_damage = static_cast<uint16_t>(spec.damage + 10 * upgrades_[1]);
    // AA tier 1 unlocks engagement; tiers 2/3 increase aircraft damage.
    // Surface craft continue to use the normal gun damage unchanged.
    s.anti_air_damage_bonus = static_cast<uint16_t>(
        upgrades_[3] > 1 ? 15 * (upgrades_[3] - 1) : 0);
    const uint16_t rounds_per_tier =
        selected_weapon()==WeaponKind::MissileBattery ||
        selected_weapon()==WeaponKind::DirectCannon ? 1u :
        selected_weapon()==WeaponKind::Flak ? 2u :
        selected_weapon()==WeaponKind::HeavyGun ? 3u : 4u;
    s.magazine = static_cast<uint16_t>(spec.magazine + rounds_per_tier*upgrades_[2]);
    s.anti_air = upgrades_[3] > 0 || spec.air_capable;
    s.turret = upgrades_[6] > 0;
    s.projectile_gravity = spec.gravity;
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
    if (u == Upgrade::AntiAir && upgrades_[i] >= 3) return -1;
    if (u == Upgrade::SupportTurret && upgrades_[i] != 0) return -1;
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
bool Campaign::PurchaseWeapon(WeaponKind weapon) {
    if (phase_ != Phase::Shop) return false;
    const uint8_t id=static_cast<uint8_t>(weapon);
    if (!WeaponValid(id)) return false;
    if (owns_weapon(weapon)) return SelectWeapon(weapon);
    const int32_t cost=WeaponFor(weapon).purchase_cost;
    if (credits_<cost) return false;
    credits_-=cost;
    owned_weapons_=static_cast<uint8_t>(owned_weapons_ | (1u<<id));
    return SelectWeapon(weapon);
}
bool Campaign::SelectWeapon(WeaponKind weapon) {
    const uint8_t id=static_cast<uint8_t>(weapon);
    if ((phase_!=Phase::Shop && phase_!=Phase::Combat) ||
        !WeaponValid(id) || !owns_weapon(weapon) || reloading())return false;
    if (id==selected_weapon_)return true;
    weapon_magazines_[selected_weapon_]=magazine_left_;
    selected_weapon_=id;
    if (phase_==Phase::Shop) {
        magazine_left_=static_cast<uint8_t>(stats().magazine);
        weapon_magazines_[id]=magazine_left_;
    } else magazine_left_=weapon_magazines_[id];
    return true;
}
bool Campaign::PurchaseSam() {
    constexpr int32_t kSamCost=540;
    if (phase_!=Phase::Shop || sam_charges_>=3 || credits_<kSamCost)
        return false;
    credits_-=kSamCost;
    ++sam_charges_;
    return true;
}
bool Campaign::CallSam(uint8_t index) {
    if (phase_!=Phase::Combat || !sam_charges_ ||
        index>=enemies_.size() || !enemies_[index].active ||
        enemies_[index].type!=UnitType::Aircraft) return false;
    --sam_charges_;
    Enemy &e=enemies_[index];
    e.hp=static_cast<int16_t>(e.hp-150);
    if(e.hp<=0) {
        e.active=false;
        ++wave_status_.aircraft_destroyed;
        credits_+=115;
    }
    return true;
}
bool Campaign::SerializeShop(uint8_t *out, size_t capacity) const {
    if (phase_ != Phase::Shop || out == nullptr || capacity < kSerializedSize)
        return false;
    const uint32_t words[20] = {
        0x31574252u, 2u, seed_, static_cast<uint32_t>(difficulty_),
        location_, day_, static_cast<uint32_t>(period_), wave_index_,
        static_cast<uint32_t>(credits_), static_cast<uint32_t>(integrity_),
        artillery_charges_, airstrike_charges_,
        upgrades_[0], upgrades_[1], upgrades_[2], upgrades_[3],
        owned_weapons_, selected_weapon_, upgrades_[6], sam_charges_
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
    if (data[0] != 0x31574252u || (data[1] != 1u && data[1] != 2u) ||
        data[3] > 3u || data[4] >= kLocations || data[5] == 0 ||
        data[5] > kPlaces[data[4]].days || data[6] > 1u ||
        data[7] == 0 || data[7] > 32u || data[8] > 10000000u ||
        data[9] == 0 || data[9] > 100u ||
        data[10] > 3u || data[11] > 3u ||
        data[12] > 4u || data[13] > 4u || data[14] > 4u ||
        data[15] > 3u || data[18] > 1u) return false;
    if (data[1]==1u && (data[16]!=0u || data[17]!=0u || data[19]!=0u))
        return false;
    if (data[1]==2u &&
        (data[16]==0u || data[16]>31u || (data[16]&1u)==0u ||
         data[17]>=kWeaponCount || (data[16]&(1u<<data[17]))==0u ||
         data[19]>3u)) return false;
    Campaign candidate(static_cast<Difficulty>(data[3]), data[2]);
    candidate.phase_ = Phase::Shop;
    candidate.location_ = static_cast<uint8_t>(data[4]);
    candidate.day_ = static_cast<uint8_t>(data[5]);
    candidate.period_ = static_cast<Period>(data[6]);
    // Validate the untruncated 32-bit checkpoint word against this exact
    // location/day/night/tier; reject overflow and late out-of-period waves
    // before a cast could silently wrap a damaged save index.
    if(!waves::CheckpointAllowed(data[7],candidate.location_,candidate.day_,
                                 candidate.period_==Period::Night,
                                 Tier(candidate.difficulty_)))return false;
    candidate.wave_index_ = static_cast<uint8_t>(data[7]);
    candidate.credits_ = static_cast<int32_t>(data[8]);
    candidate.integrity_ = static_cast<int32_t>(data[9]);
    candidate.artillery_charges_ = static_cast<uint8_t>(data[10]);
    candidate.airstrike_charges_ = static_cast<uint8_t>(data[11]);
    for (size_t i : {0u,1u,2u,3u,6u})
        candidate.upgrades_[i] = static_cast<uint8_t>(data[12u + i]);
    if(data[1]==2u) {
        candidate.owned_weapons_=static_cast<uint8_t>(data[16]);
        candidate.selected_weapon_=static_cast<uint8_t>(data[17]);
        candidate.sam_charges_=static_cast<uint8_t>(data[19]);
    }
    for(uint8_t i=0;i<kWeaponCount;i++) {
        const WeaponSpec spec=WeaponFor(static_cast<WeaponKind>(i));
        const uint8_t rounds= i==3 || i==4 ? 1 : i==2 ? 2 : i==1 ? 3 : 4;
        candidate.weapon_magazines_[i]=static_cast<uint8_t>(
            spec.magazine+rounds*candidate.upgrades_[2]);
    }
    candidate.magazine_left_=candidate.weapon_magazines_[candidate.selected_weapon_];
    *this = candidate; // commit only after every validation succeeds
    return true;
}

uint8_t Campaign::NextCount() const {
    const unsigned base=2u+Tier(difficulty_)+location_+
        (day_-1u)+(period_==Period::Night?1u:0u);
    return static_cast<uint8_t>(std::min<unsigned>(30u,base*3u));
}
bool Campaign::BeginWave() {
    if (phase_ != Phase::Shop) return false;
    enemies_.fill({});
    infantry_units_.fill({});
    armor_.fill({});
    shore_cover_ = MakeShoreCover(location_);
    infantry_spawn_cursor_ = 0;
    armor_spawn_cursor_ = 0;
    wave_status_ = {};
    infantry_ = 0;
    spawned_ = 0;
    remaining_to_spawn_ = NextCount();
    spawn_timer_ = 0;
    fire_timer_ = 0;
    reload_timer_ = 0;
    infantry_timer_ = 0;
    turret_timer_ = 0;
    turret_shot_ = {};
    infantry_volley_ = {};
    aircraft_attack_ = {};
    gunboat_salvo_ = {};
    gunboat_collision_ = {};
    armor_shot_ = {};
    // Every installed mount starts a wave with a full magazine. Switching
    // during combat preserves spent rounds per mount and never grants ammo.
    for(uint8_t id=0;id<kWeaponCount;id++) {
        const WeaponSpec spec=WeaponFor(static_cast<WeaponKind>(id));
        const uint8_t rounds=id==3 || id==4?1:id==2?2:id==1?3:4;
        weapon_magazines_[id]=static_cast<uint8_t>(
            spec.magazine+rounds*upgrades_[2]);
    }
    magazine_left_=weapon_magazines_[selected_weapon_];
    phase_ = Phase::Combat;
    return true;
}
void Campaign::Land(Enemy &e) {
    // Armor is created ONLY on a successful, live landing; sinking the
    // landing craft beforehand destroys both its infantry and vehicle.
    const uint8_t vehicles=invasion::VehicleCargo(e.type,e.carries_armor);
    for(uint8_t n=0;n<vehicles;++n) {
        // An armored landing may reuse a destroyed vehicle's retired slot.
        // Living vehicles are never displaced to make room for later waves.
        size_t slot=armor_.size();
        for(size_t i=0;i<armor_spawn_cursor_;++i)
            if(!armor_[i].active) {slot=i;break;}
        if(slot==armor_.size()) {
            if(armor_spawn_cursor_>=armor_.size())break;
            slot=armor_spawn_cursor_++;
        }
        ArmoredVehicle &vehicle=armor_[slot];
        const float offset=vehicles==2u?(n==0u?-.43f:.43f):0.0f;
        vehicle={std::max(-3.1f,std::min(3.1f,e.x+offset)),
                 -2.23f,armor::kVehicleHp,true,1.8f+.45f*n};
        ++wave_status_.armor_landed;
    }
    // Reuse only defeated troops' slots for larger waves.
    uint8_t deployed=0;
    for (uint8_t passenger=0;passenger<e.passengers;++passenger) {
        size_t slot=infantry_units_.size();
        for(size_t i=0;i<infantry_spawn_cursor_;++i)
            if(!infantry_units_[i].active) { slot=i; break; }
        if(slot==infantry_units_.size()) {
            if(infantry_spawn_cursor_>=infantry_units_.size())break;
            slot=infantry_spawn_cursor_++;
        }
        Infantry &trooper=infantry_units_[slot];
        trooper=Infantry{};
        ++deployed;
        const float spread = (static_cast<float>(passenger) -
            static_cast<float>(e.passengers - 1u) * 0.5f) * 0.27f;
        trooper.x = std::max(-3.1f, std::min(3.1f, e.x + spread));
        trooper.z = -2.28f - 0.10f * static_cast<float>(passenger % 2u);
        trooper.active = true;
        trooper.tactic = AssignInfantryTactic(e.type,passenger);
        trooper.lane_x = InfantryLaneX(trooper.x,trooper.tactic);
        trooper.open_lane_x=trooper.lane_x;
        if(e.type==UnitType::LandingCraft ||
           e.type==UnitType::ArmoredTransport)
            AssignInfantryCover(trooper,shore_cover_,passenger);
    }
    e.active = false;
    e.landed = true;
    infantry_ = static_cast<uint8_t>(infantry_ + deployed);
    wave_status_.troops_landed = static_cast<uint8_t>(
        wave_status_.troops_landed + deployed);
}
IncomingThreats Campaign::incoming_threats() const {
    IncomingThreats warning;
    if (phase_ != Phase::Combat) return warning;
    for(const ArmoredVehicle &vehicle:armor_) {
        const float seconds=armor::SecondsToFire(vehicle);
        if(seconds>=0.0f && (warning.armor_seconds<0.0f ||
                             seconds<warning.armor_seconds))
            warning.armor_seconds=seconds;
    }
    for (const Enemy &enemy : enemies_) {
        if (!enemy.active) continue;
        float speed = 0.0f, start = 0.0f, end = 0.0f;
        float *earliest = nullptr;
        if (enemy.type == UnitType::Aircraft && enemy.attacks_remaining > 0) {
            const bool bomber = enemy.aircraft_pattern == AircraftPattern::Bomber;
            speed = bomber ? kBomberSpeed : kStraferSpeed;
            start = bomber ? kBomberAttackStart : kStraferAttackStart;
            end = bomber ? kBomberAttackEnd : kStraferAttackEnd;
            earliest = &warning.aircraft_seconds;
        } else if (enemy.type == UnitType::Gunboat ||
                   (enemy.type == UnitType::ArmoredTransport &&
                    !enemy.landed)) {
            speed = enemy.type==UnitType::ArmoredTransport?.44f:kGunboatSpeed;
            start = kGunboatAttackStart;
            end = enemy.type==UnitType::ArmoredTransport?
                invasion::BeachingZ(enemy.type):kGunboatAttackEnd;
            earliest = &warning.gunboat_seconds;
        } else continue;
        // A cooldown that expires after the attacker exits the firing lane
        // is not a real incoming salvo. Use the same speed/zone as TickStep.
        const float until_zone = std::max(0.0f, (start - enemy.z) / speed);
        const float until_fire = std::max(until_zone,
                                          std::max(0.0f, enemy.attack_timer));
        if (enemy.z + speed * until_fire > end) continue;
        if (*earliest < 0.0f || until_fire < *earliest)
            *earliest = until_fire;
    }
    // Shared firing cadence remains four seconds, but each deployed
    // soldier owns an actual tactic-dependent lane, forward speed and
    // firing line. The HUD uses precisely the same arrival calculation
    // as AdvanceInfantry, rather than guessing from the first rifleman.
    bool shooter_ready = false;
    float nearest_arrival = -1.0f;
    for (const Infantry &original : infantry_units_) {
        if (!original.active) continue;
        // Destruction of a wall takes effect on the threat predictor
        // immediately, even before the next physics update can mutate
        // the soldier's original station/order.
        Infantry soldier=original;
        RefreshInfantryCover(soldier,shore_cover_);
        if (InfantryReady(soldier)) {
            shooter_ready = true;
            break;
        }
        const float until_ready = InfantrySecondsToReady(soldier);
        if(until_ready<0.0f)continue;
        if (nearest_arrival < 0.0f || until_ready < nearest_arrival)
            nearest_arrival = until_ready;
    }
    if (shooter_ready)
        warning.infantry_seconds = std::max(0.0f,
            kInfantrySalvoPeriod - infantry_timer_);
    else if (nearest_arrival >= 0.0f)
        warning.infantry_seconds = nearest_arrival + kInfantrySalvoPeriod;
    return warning;
}
float Campaign::reload_fraction() const {
    if (!reloading()) return 0.0f;
    // This is display-only progress. The simulation remains the sole owner
    // of magazine refills and reload timing.
    return std::max(0.0f, std::min(1.0f,
        1.0f - reload_timer_ / kReloadSeconds));
}
bool Campaign::RequestReload() {
    // Tactical reload is an explicit secondary action. It costs the same
    // time as an empty-magazine reload and cannot interrupt one in progress.
    if (phase_ != Phase::Combat || reload_timer_ > 0.0f ||
        magazine_left_ == 0 || magazine_left_ >= stats().magazine) return false;
    reload_timer_ = kReloadSeconds;
    return true;
}
bool Campaign::BeginGunShot() {
    if (phase_ != Phase::Combat || fire_timer_ > 0 ||
        reload_timer_ > 0 || magazine_left_ == 0) return false;
    --magazine_left_;
    fire_timer_ = stats().fire_delay_ms / 1000.0f;
    if (magazine_left_ == 0) reload_timer_ = kReloadSeconds;
    return true;
}
bool Campaign::FireMiss() {
    if(selected_weapon()==WeaponKind::MissileBattery) return false;
    return BeginGunShot();
}
bool Campaign::FireAt(uint8_t index) {
    if (phase_ != Phase::Combat || index >= enemies_.size() ||
        !enemies_[index].active) return false;
    Enemy &e = enemies_[index];
    if (!BeginGunShot()) return false;
    // The basic mount may still throw rounds at an aircraft, but without the
    // AA package those rounds cannot damage the fast overhead target.
    const Stats weapon = stats();
    if (e.type == UnitType::Aircraft &&
        (selected_weapon()==WeaponKind::DirectCannon || !weapon.anti_air))
        return true;
    const int damage = weapon.gun_damage +
        (e.type == UnitType::Aircraft ? weapon.anti_air_damage_bonus : 0);
    e.hp = static_cast<int16_t>(e.hp-
        invasion::HullDamage(e.type,selected_weapon(),damage));
    if (e.hp <= 0) {
        e.active = false;
        credits_ += e.type == UnitType::Aircraft ? 115 :
            e.type == UnitType::ArmoredTransport ? 210 : 55;
        if (e.type == UnitType::Aircraft) ++wave_status_.aircraft_destroyed;
        else ++wave_status_.boats_sunk;
        // Critically: a sunk boat does not create disembarked infantry.
    }
    return true;
}
bool Campaign::FireAtInfantry(uint8_t index) {
    if (phase_ != Phase::Combat || index >= infantry_spawn_cursor_ ||
        !infantry_units_[index].active) return false;
    if (!WeaponFor(selected_weapon()).anti_infantry) return false;
    if (!BeginGunShot()) return false;
    infantry_units_[index].active = false;
    --infantry_;
    ++wave_status_.troops_stopped;
    credits_ += 20;
    return true;
}
bool Campaign::FireAtArmor(uint8_t index) {
    if(phase_!=Phase::Combat || index>=armor_.size() ||
       !armor_[index].active || !BeginGunShot())return false;
    ArmoredVehicle &vehicle=armor_[index];
    vehicle.hp=static_cast<int16_t>(vehicle.hp-
        armor::ArmorDamage(selected_weapon(),stats().gun_damage));
    if(vehicle.hp<=0) {
        vehicle.active=false;
        ++wave_status_.armor_destroyed;
        credits_+=armor::kArmorKillCredits;
    }
    return true;
}
bool Campaign::FireAtCover(uint8_t index) {
    if(phase_!=Phase::Combat || index>=shore_cover_.size() ||
       shore_cover_[index].hp<=0 || !BeginGunShot())return false;
    ShoreCover &wall=shore_cover_[index];
    const ShoreCover wall_before=wall;
    // Cover absorbs the actual selected mount damage and persists across
    // individual shots. A destroyed wall cannot block future targeting.
    wall.hp=static_cast<int16_t>(std::max(0,
        static_cast<int>(wall.hp)-static_cast<int>(stats().gun_damage)));
    // The hit suppresses only soldiers physically occupying this wall.
    // Use its pre-hit geometry even if the same shot finishes destruction.
    // Destroyed-wall rerouting remains immediate and authoritative.
    for(Infantry &trooper:infantry_units_) {
        if(!trooper.active || trooper.cover_slot!=static_cast<int8_t>(index))
            continue;
        if(CoverSheltersInfantry(trooper,wall_before,index))
            trooper.pinned_seconds=kInfantryPinSeconds;
        if(wall.hp==0)RefreshInfantryCover(trooper,shore_cover_);
    }
    return true;
}
bool Campaign::CallArtillery() {
    if (phase_ != Phase::Combat || artillery_charges_ == 0) return false;
    // A strike is only spent if at least one eligible vessel is present.
    // Troops already ashore and aircraft are not artillery targets.
    const bool target_available = std::any_of(enemies_.begin(), enemies_.end(),
        [](const Enemy &e) { return e.active && e.type != UnitType::Aircraft; }) ||
        std::any_of(armor_.begin(),armor_.end(),
            [](const ArmoredVehicle &v){return v.active;});
    if (!target_available) return false;
    --artillery_charges_;
    for (Enemy &e : enemies_) {
        if (e.active && e.type != UnitType::Aircraft) {
            e.hp = static_cast<int16_t>(e.hp -
                invasion::SupportDamage(e.type,120));
            if (e.hp <= 0) { e.active = false; ++wave_status_.boats_sunk;
                credits_ += e.type==UnitType::ArmoredTransport?210:55; }
        }
    }
    for(ArmoredVehicle &v:armor_)if(v.active) {
        v.hp=static_cast<int16_t>(v.hp-120);
        if(v.hp<=0) {
            v.active=false;
            ++wave_status_.armor_destroyed;
            credits_+=armor::kArmorKillCredits;
        }
    }
    return true;
}
bool Campaign::CallAirstrike() {
    if (phase_ != Phase::Combat || airstrike_charges_ == 0) return false;
    // Empty-airspace calls must not silently consume purchased support.
    const bool target_available = std::any_of(enemies_.begin(), enemies_.end(),
        [](const Enemy &e) { return e.active; }) ||
        std::any_of(armor_.begin(),armor_.end(),
            [](const ArmoredVehicle &v){return v.active;});
    if (!target_available) return false;
    --airstrike_charges_;
    for (Enemy &e : enemies_) {
        if (e.active) {
            e.hp = static_cast<int16_t>(e.hp -
                invasion::SupportDamage(e.type,150));
            if (e.hp <= 0) {
                e.active = false; credits_ +=
                    e.type==UnitType::ArmoredTransport?210:75;
                if (e.type == UnitType::Aircraft) ++wave_status_.aircraft_destroyed;
                else ++wave_status_.boats_sunk;
            }
        }
    }
    for(ArmoredVehicle &v:armor_)if(v.active) {
        v.hp=static_cast<int16_t>(v.hp-150);
        if(v.hp<=0) {
            v.active=false;
            ++wave_status_.armor_destroyed;
            credits_+=armor::kArmorKillCredits;
        }
    }
    return true;
}
void Campaign::Tick(float seconds) {
    if (phase_ != Phase::Combat || !std::isfinite(seconds) || seconds <= 0) return;
    // The native input ABI accepts up to 0.10s per frame. Previously
    // discarding everything beyond 0.05s slowed the entire campaign
    // (spawns, reloads and incoming fire) whenever the headset ran below 20 FPS.
    // Catch up in bounded substeps, but never simulate a long resume pause.
    float remaining = std::min(seconds, 0.10f);
    while (remaining > 0.0f && phase_ == Phase::Combat) {
        const float dt = std::min(remaining, 0.05f);
        TickStep(dt);
        remaining -= dt;
    }
}
void Campaign::TickStep(float dt) {
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
            const UnitType kind = invasion::SelectUnit(
                progression,wave_index_,waves_this_period(),choice,spawned_);
            const int hp = EnemyStartingHp(kind);
            e = {kind, (static_cast<int>(choice % 9u) - 4) * 0.65f,
                 -11.0f, static_cast<int16_t>(hp),
                 invasion::InfantryCargo(kind,choice),
                 true, false};
            // Guaranteed final-wave escort forms alongside the flagship.
            // Its faster travel and real deck gun screen the payload.
            if(kind==UnitType::Gunboat && progression>=10u &&
               wave_index_==waves_this_period() && spawned_==1u) {
                const uint32_t flagship_roll=Hash(0x1234u);
                const float flagship_lane=
                    (static_cast<int>(flagship_roll%9u)-4)*.65f;
                e.x=invasion::FlagshipEscortLane(flagship_lane);
            }
            e.carries_armor=kind==UnitType::ArmoredTransport ||
                armor::CarriesArmor(kind,progression,choice,spawned_);
            if(e.carries_armor && kind==UnitType::LandingCraft)
                e.passengers=armor::TroopPassengers(e.passengers,true);
            if (kind == UnitType::Aircraft) {
                e.aircraft_pattern = (choice & 0x100u) ?
                    AircraftPattern::Bomber : AircraftPattern::Strafer;
                e.attacks_remaining =
                    e.aircraft_pattern == AircraftPattern::Bomber ? 1u : 4u;
                e.attack_timer = 0.15f;
                e.flight_center_x = e.x;
                e.flight_sign = (choice & 0x200u) ? 1.0f : -1.0f;
                const AircraftFlightPose start = AircraftFlightAt(
                    e.aircraft_pattern,e.flight_center_x,e.flight_sign,e.z);
                e.x = start.x;
                e.bank = start.bank;
            } else if (kind == UnitType::Gunboat ||
                       kind == UnitType::ArmoredTransport) {
                // Heavy transports have their own slower deck-gun cycle.
                e.attack_timer = kind==UnitType::ArmoredTransport?3.4f:2.6f;
            }
            --remaining_to_spawn_;
            ++spawned_;
            spawn_timer_ = 0.65f;
            break;
        }
    }
    // Purchased support turrets defend against the closest surface threat,
    // including armored vehicles already deployed ashore. Aircraft remain
    // immune. Target selection is based on the real live z positions; no
    // imaginary shot is emitted when there is nothing eligible to hit.
    if (stats().turret) {
        turret_timer_ -= dt;
        if (turret_timer_ <= 0.0f) {
            Enemy *target = nullptr;
            for (Enemy &candidate : enemies_) {
                if (candidate.active && candidate.type != UnitType::Aircraft &&
                    (target == nullptr || candidate.z > target->z))
                    target = &candidate;
            }
            ArmoredVehicle *vehicle = nullptr;
            for (ArmoredVehicle &candidate : armor_)
                if(candidate.active &&
                   (vehicle == nullptr || candidate.z > vehicle->z))
                    vehicle=&candidate;
            if(target && vehicle && target->z>=vehicle->z)
                vehicle=nullptr;
            else if(vehicle)
                target=nullptr;
            if (target || vehicle) {
                const float hit_x = vehicle?vehicle->x:target->x;
                const float hit_z = vehicle?vehicle->z:target->z;
                bool destroyed = false;
                if(vehicle) {
                    // The autonomous gun fires heavy projectiles; armor is
                    // not magically vulnerable to its full hull damage.
                    vehicle->hp=static_cast<int16_t>(vehicle->hp-
                        armor::ArmorDamage(WeaponKind::HeavyGun,15u));
                    destroyed=vehicle->hp<=0;
                    if(destroyed) {
                        vehicle->active=false;
                        ++wave_status_.armor_destroyed;
                        credits_+=armor::kArmorKillCredits;
                    }
                } else {
                    target->hp = static_cast<int16_t>(target->hp -
                        invasion::HullDamage(target->type,
                                              WeaponKind::HeavyGun,15));
                    destroyed=target->hp<=0;
                    if(destroyed) {
                        target->active=false;
                        ++wave_status_.boats_sunk;
                        credits_+=target->type==UnitType::ArmoredTransport?
                            210:55;
                    }
                }
                ++turret_shot_.sequence;
                turret_shot_.x=hit_x;
                turret_shot_.z=hit_z;
                turret_shot_.destroyed=destroyed;
                turret_timer_=1.35f;
            }
        }
    }
    for (Enemy &e : enemies_) {
        if (!e.active) continue;
        if (e.type == UnitType::ArmoredTransport && e.landed) {
            // Keep the 3D ramp open and vulnerable until cargo is released.
            // A kill at the beach denies both cars and four soldiers.
            e.attack_timer=std::max(0.0f,e.attack_timer-dt);
            if(e.attack_timer<=0.0f)Land(e);
            continue;
        }
        if (e.type == UnitType::Aircraft) {
            const bool bomber = e.aircraft_pattern == AircraftPattern::Bomber;
            e.z += (bomber ? kBomberSpeed : kStraferSpeed) * dt;
            // The same authoritative x drives wing rendering, targeting,
            // hit impacts and recorded attacking-aircraft coordinates.
            const AircraftFlightPose pose = AircraftFlightAt(
                e.aircraft_pattern,e.flight_center_x,e.flight_sign,e.z);
            e.x = pose.x;
            e.bank = pose.bank;
            e.attack_timer = std::max(0.0f, e.attack_timer - dt);
            const float attack_start = bomber ? kBomberAttackStart : kStraferAttackStart;
            const float attack_end = bomber ? kBomberAttackEnd : kStraferAttackEnd;
            if (e.attacks_remaining > 0 && e.z >= attack_start &&
                e.z <= attack_end && e.attack_timer <= 0.0f) {
                const uint8_t nominal_damage = bomber ? 12u : 3u;
                const int32_t hull_before = integrity_;
                integrity_ = std::max(0, integrity_ - nominal_damage);
                --e.attacks_remaining;
                ++wave_status_.aircraft_attacks;
                const int32_t actual_damage = hull_before - integrity_;
                if (actual_damage > 0) {
                    ++aircraft_attack_.sequence;
                    aircraft_attack_.x = e.x;
                    aircraft_attack_.z = e.z;
                    aircraft_attack_.pattern = e.aircraft_pattern;
                    aircraft_attack_.damage = static_cast<uint8_t>(actual_damage);
                }
                e.attack_timer = bomber ? 20.0f : 0.72f;
                // A fatal strike ends this substep; no later enemy acts.
                if (integrity_ == 0) { phase_ = Phase::Defeat; return; }
            }
            // Aircraft make a complete overhead pass rather than colliding
            // with the beach. Surviving attackers leave the combat volume.
            if (e.z >= 2.4f) e.active = false;
            continue;
        }
        const float speed = e.type == UnitType::Rowboat ? 0.9f :
            e.type == UnitType::ArmoredTransport ? .44f :
            e.type == UnitType::LandingCraft ? 0.62f : kGunboatSpeed;
        // Armed gunboats are not troop carriers or kamikaze landing craft.
        // Stop short of shore and sustain fire; only player/support damage
        // can destroy them. Troop transports still progress to their ramps.
        e.z = e.type==UnitType::Gunboat?
            std::min(invasion::kGunboatStandOffZ,e.z+speed*dt):
            e.z+speed*dt;
        if (e.type == UnitType::Gunboat ||
            e.type == UnitType::ArmoredTransport) {
            e.attack_timer = std::max(0.0f, e.attack_timer - dt);
            // Unlike troop transports, gunboats threaten the battery from
            // offshore. Sinking one before the firing line prevents its salvos.
            const float attack_end=e.type==UnitType::ArmoredTransport?
                invasion::BeachingZ(e.type):kGunboatAttackEnd;
            if (e.z >= kGunboatAttackStart && e.z < attack_end &&
                e.attack_timer <= 0.0f) {
                const int32_t before = integrity_;
                integrity_ = std::max(0, integrity_ -
                    (e.type==UnitType::ArmoredTransport?5:3));
                ++wave_status_.gunboat_attacks;
                const int32_t damage = before - integrity_;
                if (damage > 0) {
                    ++gunboat_salvo_.sequence;
                    gunboat_salvo_.x = e.x;
                    gunboat_salvo_.z = e.z;
                    gunboat_salvo_.damage = static_cast<uint8_t>(damage);
                    gunboat_salvo_.armored_transport=
                        e.type==UnitType::ArmoredTransport;
                }
                e.attack_timer = e.type==UnitType::ArmoredTransport?
                    3.8f:2.8f;
                if (integrity_ == 0) { phase_ = Phase::Defeat; return; }
            }
        }
        if (e.z >= invasion::BeachingZ(e.type)) {
            if(e.type==UnitType::ArmoredTransport) {
                e.z=invasion::BeachingZ(e.type);
                e.landed=true;
                e.attack_timer=invasion::UnloadingSeconds(e.type);
            } else Land(e);
        }
    }
    // Only armor that has been physically unloaded from a surviving
    // transport can enter this loop. The moving vehicle is silent until
    // its real shore firing position and cooldown are both satisfied.
    // The attack and its damage are authoritative,
    // and destroying the vehicle suppresses all future salvos.
    for(ArmoredVehicle &vehicle:armor_) {
        if(!vehicle.active)continue;
        armor::Advance(vehicle,dt);
        if(armor::Ready(vehicle)) {
            const int32_t before=integrity_;
            integrity_=std::max(0,integrity_-armor::kArmorShellDamage);
            const int32_t damage=before-integrity_;
            if(damage>0) {
                ++armor_shot_.sequence;
                armor_shot_.x=vehicle.x;
                armor_shot_.z=vehicle.z;
                armor_shot_.damage=static_cast<uint8_t>(damage);
                ++wave_status_.armor_attacks;
            }
            vehicle.attack_timer=armor::kArmorFireInterval;
            if(integrity_==0) {phase_=Phase::Defeat;return;}
        }
    }
    // Riflemen hold the old frontal line. Landing-craft flankers travel
    // toward side lanes while advancing, and support gunners stop short
    // at a rear firing line. All remain individually targetable soldiers.
    uint8_t ready_to_fire = 0;
    int32_t nominal_damage = 0;
    uint64_t firing_mask = 0;
    const Infantry *first_shooter = nullptr;
    static_assert(kMaxInfantry<=64,"volley roster must fit uint64_t");
    for (size_t slot=0;slot<infantry_units_.size();++slot) {
        Infantry &trooper=infantry_units_[slot];
        if (!trooper.active) continue;
        RefreshInfantryCover(trooper,shore_cover_);
        AdvanceInfantry(trooper,dt);
        if (InfantryReady(trooper)) {
            ++ready_to_fire;
            firing_mask|=(uint64_t{1}<<slot);
            nominal_damage += InfantryShotDamage(trooper.tactic);
            if (first_shooter == nullptr) first_shooter = &trooper;
        }
    }
    if (ready_to_fire == 0) {
        // Losing all ready soldiers cancels the partial salvo charge.
        infantry_timer_ = 0.0f;
    } else {
        infantry_timer_ += dt;
        if (infantry_timer_ >= kInfantrySalvoPeriod) {
            infantry_timer_ = 0.0f;
            const int32_t hull_before = integrity_;
            integrity_ = std::max(0, integrity_ - nominal_damage);
            const int32_t actual_damage = hull_before - integrity_;
            if (actual_damage > 0) {
                ++infantry_volley_.sequence;
                infantry_volley_.x = first_shooter->x;
                infantry_volley_.z = first_shooter->z;
                infantry_volley_.shooters = ready_to_fire;
                infantry_volley_.damage = static_cast<uint8_t>(actual_damage);
                infantry_volley_.firing_mask = firing_mask;
            }
        }
    }
    if (integrity_ == 0) { phase_ = Phase::Defeat; return; }
    if (FinishedWave()) CompleteWave();
}
bool Campaign::FinishedWave() const {
    if (remaining_to_spawn_ || infantry_) return false;
    for(const ArmoredVehicle &vehicle:armor_)
        if(vehicle.active)return false;
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
