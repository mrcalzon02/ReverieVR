#ifndef REVERIE_BREAKWATER_WAVE_SCHEDULE_H
#define REVERIE_BREAKWATER_WAVE_SCHEDULE_H
#include <cstdint>
namespace reverie { namespace breakwater { namespace waves {
// Keep all day/night periods within the same 32-wave checkpoint envelope.
// Opening day remains unchanged; later days and locations have more battles.
// More waves do not mean a larger live-enemy pool or a new save schema.
constexpr uint8_t ForPeriod(uint8_t location,uint8_t day,bool night,
                            uint8_t difficulty_tier) {
    const unsigned total=(night?8u:12u)+2u*difficulty_tier+
        (day>0?2u*static_cast<unsigned>(day-1u):0u)+3u*location;
    return static_cast<uint8_t>(total>32u?32u:total);
}
// Validate the *full serialized 32-bit word* before narrowing to uint8_t.
// This is shared by the campaign's checkpoint reader and an executable
// independent regression, avoiding the old hardcoded eight-wave cap.
constexpr bool CheckpointAllowed(uint32_t serialized_wave,uint8_t location,
                                 uint8_t day,bool night,uint8_t tier) {
    return serialized_wave>=1u &&
           serialized_wave<=ForPeriod(location,day,night,tier);
}
}}}
#endif
