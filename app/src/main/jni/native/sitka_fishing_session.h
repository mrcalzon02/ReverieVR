#ifndef REVERIE_SITKA_FISHING_SESSION_H
#define REVERIE_SITKA_FISHING_SESSION_H

// Game-private, CPU-only bridge: normalized actions + host-scoped persistence.
// No geometry assumptions. A verified shoreline/bathymetry target must be
// supplied by the caller; an authored water rectangle is NOT proof of ocean.
#include "sitka_fishing.h"
#include <cstdint>
#include <string>

namespace reverie { namespace sitka {
struct FishingStore {
    void* user = nullptr;
    // 1 = read a save, 0 = no save, negative = I/O error.
    int (*read)(void*, std::string&) = nullptr;
    bool (*write)(void*, const std::string&) = nullptr;
};
enum class FishingLoad : uint8_t { Fresh, Restored, Unavailable, Invalid, IoError };
enum class FishingEvent : uint8_t {
    None, Cast, CastRejected, Bite, ReelStarted, Landed, Missed, Cancelled, Acknowledged
};
struct FishingUpdate {
    FishingEvent event = FishingEvent::None;
    bool journal_changed = false;
};
class FishingSession {
public:
    Fishing fishing;
    bool dirty = false;
    bool write_failed = false;
    FishingLoad Load(const FishingStore& store) {
        if (!store.read || !store.write) return FishingLoad::Unavailable;
        std::string contents;
        const int result = store.read(store.user, contents);
        if (result == 0) return FishingLoad::Fresh;
        if (result != 1) return FishingLoad::IoError;
        Journal parsed;
        if (!LoadJournal(contents, parsed)) return FishingLoad::Invalid;
        fishing.journal = parsed;
        dirty = write_failed = false;
        primary_held_ = secondary_held_ = false;
        return FishingLoad::Restored;
    }
    FishingUpdate Update(float delta_seconds, bool primary, bool secondary,
                         bool verified_cast_target, float surveyed_depth_metres) {
        // A held control may never activate a second cast or acknowledge a catch.
        const bool pressed = primary && !primary_held_;
        const bool cancel = secondary && !secondary_held_;
        primary_held_ = primary;
        secondary_held_ = secondary;
        FishingUpdate result;
        const auto before = fishing.phase;
        const uint32_t old_seed = fishing.journal.seed;
        const uint32_t old_total = fishing.journal.total;
        if (before == FishingPhase::Ready && pressed) {
            if (verified_cast_target && fishing.Cast(true, surveyed_depth_metres))
                result.event = FishingEvent::Cast;
            else result.event = FishingEvent::CastRejected;
        } else if (before == FishingPhase::Landed && pressed) {
            if (fishing.AcknowledgeCatch()) result.event = FishingEvent::Acknowledged;
        } else {
            // Bite requires a new press, not a primary button held since casting.
            fishing.Step(delta_seconds, before == FishingPhase::Bite ? pressed : primary, cancel);
            const auto after = fishing.phase;
            if (after != before) {
                if (after == FishingPhase::Bite) result.event = FishingEvent::Bite;
                else if (after == FishingPhase::Reeling) result.event = FishingEvent::ReelStarted;
                else if (after == FishingPhase::Landed) result.event = FishingEvent::Landed;
                else if (after == FishingPhase::Ready)
                    result.event = cancel ? FishingEvent::Cancelled : FishingEvent::Missed;
            }
        }
        result.journal_changed = fishing.journal.seed != old_seed ||
                                 fishing.journal.total != old_total;
        if (result.journal_changed) dirty = true;
        return result;
    }
    bool UpgradeTackle() {
        if (!fishing.UpgradeTackle()) return false;
        dirty = true;
        return true;
    }
    bool Flush(const FishingStore& store) {
        if (!dirty) return true;
        if (!store.write || !store.write(store.user, SaveJournal(fishing.journal))) {
            write_failed = true;
            return false;
        }
        dirty = write_failed = false;
        return true;
    }
private:
    bool primary_held_ = false;
    bool secondary_held_ = false;
};
}} // namespace reverie::sitka
#endif
