#ifndef REVERIE_SITKA_FISHING_H
#define REVERIE_SITKA_FISHING_H
// Game-private, CPU-only Sitka fishing rules. No real-world harvest advice.
// Call Step once per simulation frame, never once per stereo eye.
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace reverie { namespace sitka {
enum class FishingPhase : uint8_t { Ready, Waiting, Bite, Reeling, Landed };
enum class Fish : uint8_t { DollyVarden, Coho, PinkSalmon, Flounder, Count };
inline constexpr std::array<const char*,4> kFishNames = {{"Dolly Varden", "Coho salmon", "Pink salmon", "Flounder"}};
struct Journal {
    uint32_t seed = 0x51A7CAu;
    uint32_t credits = 0;
    uint32_t gear = 0;
    std::array<uint32_t,4> caught = {{0,0,0,0}};
    uint32_t total = 0;
};
struct Fishing {
    Journal journal;
    FishingPhase phase = FishingPhase::Ready;
    Fish hooked = Fish::DollyVarden;
    float time = 0.f, reel = 0.f, tension = 0.f;
    float depth = 0.f;
    // Deterministic independent PRNG: no wall clock, global rand or render effects.
    uint32_t Next() {
        uint32_t v = journal.seed ? journal.seed : 0x51A7CAu;
        v ^= v << 13u; v ^= v >> 17u; v ^= v << 5u;
        journal.seed = v;
        return v;
    }
    bool Cast(bool confirmed_water, float depth_metres) {
        if (phase != FishingPhase::Ready || !confirmed_water ||
            !std::isfinite(depth_metres) || depth_metres < .3f || depth_metres > 200.f) return false;
        depth = depth_metres;
        time = 2.0f + float(Next() % 250u) * .012f;
        reel = tension = 0.f;
        phase = FishingPhase::Waiting;
        return true;
    }
    // Primary starts reeling after a bite; holding it reels. Secondary cancels.
    void Step(float delta, bool primary, bool secondary) {
        if (!std::isfinite(delta) || delta <= 0.f) return;
        const float dt = std::min(delta, .1f); // bounded after an app pause
        if (secondary && phase != FishingPhase::Ready) {
            phase = FishingPhase::Ready; time = reel = tension = 0.f; return;
        }
        switch (phase) {
            case FishingPhase::Waiting:
                time -= dt;
                if (time <= 0.f) {
                    // Shallow water favors flounder; deeper pockets favor salmon.
                    const uint32_t pick = Next() % 100u;
                    hooked = depth < 3.f ?
                        (pick < 40u ? Fish::Flounder : pick < 70u ? Fish::DollyVarden :
                         pick < 85u ? Fish::PinkSalmon : Fish::Coho) :
                        (pick < 38u ? Fish::Coho : pick < 67u ? Fish::PinkSalmon :
                         pick < 90u ? Fish::DollyVarden : Fish::Flounder);
                    phase = FishingPhase::Bite; time = 8.f;
                }
                break;
            case FishingPhase::Bite:
                if (primary) { phase = FishingPhase::Reeling; time = 0.f; }
                else if ((time -= dt) <= 0.f) { phase = FishingPhase::Ready; }
                break;
            case FishingPhase::Reeling:
                if (primary) {
                    tension = std::min(1.f, tension + dt * .52f);
                    // At high tension the line yields rather than snapping: calm fishing.
                    reel = std::min(1.f, reel + dt * (.18f + .045f*float(journal.gear)) *
                        (tension > .86f ? .25f : 1.f));
                } else {
                    tension = std::max(0.f, tension - dt * .85f);
                }
                if (reel >= 1.f) {
                    const unsigned id = static_cast<unsigned>(hooked);
                    if (id < journal.caught.size() && journal.total < 1000000u &&
                        journal.caught[id] < 1000000u) {
                        ++journal.caught[id]; ++journal.total;
                        journal.credits = std::min(1000000u, journal.credits + 35u);
                    }
                    phase = FishingPhase::Landed;
                }
                break;
            case FishingPhase::Ready: case FishingPhase::Landed: break;
        }
    }
    bool AcknowledgeCatch() {
        if (phase != FishingPhase::Landed) return false;
        phase = FishingPhase::Ready; time = reel = tension = 0.f;
        return true;
    }
    uint32_t UpgradeCost() const {
        return journal.gear >= 3u ? 0u : (journal.gear == 0u ? 70u : journal.gear == 1u ? 160u : 320u);
    }
    bool UpgradeTackle() {
        const uint32_t cost = UpgradeCost();
        if (phase != FishingPhase::Ready || cost == 0u || journal.credits < cost) return false;
        journal.credits -= cost; ++journal.gear; return true;
    }
};
// Deliberately saves durable journal only; active casts never resume mid-fight.
inline std::string SaveJournal(const Journal& j) {
    std::string out="SITKA-FISH-1\n";
    const uint32_t fields[] = {j.seed,j.credits,j.gear,j.total,
        j.caught[0],j.caught[1],j.caught[2],j.caught[3]};
    for (uint32_t field : fields) out += std::to_string(field) + "\n";
    return out;
}
inline bool LoadJournal(std::string_view bytes, Journal& out) {
    constexpr std::string_view magic="SITKA-FISH-1\n";
    if (bytes.size() <= magic.size() || bytes.size() > 128u ||
        bytes.substr(0,magic.size()) != magic) return false;
    bytes.remove_prefix(magic.size());
    std::array<uint32_t,8> n{};
    for (auto& field : n) {
        const size_t end = bytes.find('\n');
        if (end == std::string_view::npos || end == 0u || end > 10u) return false;
        const char* first = bytes.data();
        const auto parsed = std::from_chars(first, first+end, field);
        if (parsed.ec != std::errc{} || parsed.ptr != first+end) return false;
        bytes.remove_prefix(end+1u);
    }
    if (!bytes.empty() || n[0]==0u || n[1]>1000000u || n[2]>3u || n[3]>1000000u)
        return false;
    uint64_t sum=0u;
    for (size_t i=4u;i<8u;++i) { if (n[i]>1000000u) return false; sum+=n[i]; }
    if (sum != n[3]) return false;
    Journal candidate;
    candidate.seed=n[0];candidate.credits=n[1];candidate.gear=n[2];candidate.total=n[3];
    for (size_t i=0;i<4u;++i)candidate.caught[i]=n[i+4u];
    out=candidate;return true;
}
}} // namespace reverie::sitka
#endif
