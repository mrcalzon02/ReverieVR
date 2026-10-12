#ifndef REVERIE_BREAKWATER_DAMAGE_FEEDBACK_H
#define REVERIE_BREAKWATER_DAMAGE_FEEDBACK_H
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater {
// Render-only feedback driven by the simulation's actual battery integrity.
// No allocation, independent wall clock, or per-eye state mutation.
class BatteryDamageFeedback {
public:
    static constexpr float kDuration = .65f;
    bool Record(int32_t before, int32_t after) {
        if (before <= 0 || before > 100 || after < 0 || after >= before)
            return false;
        damage_ = before - after;
        remaining_ = kDuration;
        return true;
    }
    void Tick(float seconds) {
        if (!std::isfinite(seconds) || seconds <= 0.0f) return;
        remaining_ = std::max(0.0f, remaining_ - seconds);
    }
    bool active() const { return remaining_ > 0.0f; }
    int32_t damage() const { return damage_; }
    float remaining() const { return remaining_; }
private:
    float remaining_ = 0.0f;
    int32_t damage_ = 0;
};
}} // namespace reverie::breakwater
#endif
