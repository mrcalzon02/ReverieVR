#ifndef REVERIE_BREAKWATER_SCENE_REFRESH_H
#define REVERIE_BREAKWATER_SCENE_REFRESH_H
#include "breakwater_campaign.h"

namespace reverie { namespace breakwater { namespace scene {
// The full shore + sprite world is rebuilt by Draw only when game state or
// visible UI changes. Combat always advances/animates, but an idle shop,
// briefing or result panel should not rebake coastal meshes every frame.
// The host still renders both stereo eyes from the last complete GPU batch.
inline bool RefreshNeeded(Phase before, Phase after,
                          bool selection_changed, bool action_pressed,
                          bool pointer_focus_changed) {
    return before==Phase::Combat || after==Phase::Combat ||
           before!=after || selection_changed || action_pressed ||
           pointer_focus_changed;
}
}}}
#endif
