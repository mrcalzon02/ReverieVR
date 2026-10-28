#ifndef REVERIE_OMEGA_FRONT_CONTROLS_H
#define REVERIE_OMEGA_FRONT_CONTROLS_H
#include "omega_front_tactics.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace reverie { namespace omega {

enum class CommandMode : uint8_t { Move, Attack, Repair, Resupply };
struct ControlsInput {
    float dt=0;
    float x=0, y=0;
    bool primary=false, secondary=false;
    int pointer_x=-1, pointer_y=-1;
    // Deliberate single tactical turn resolution; never tied to frame dt.
    bool commit_turn=false;
};

// UI commands are a separate deterministic consumer of the headless battle.
// Daydream axes work without holding click, while gaze ray may select tiles.
class Controller {
public:
    Battle battle;
    int cursor_x=4, cursor_y=12;
    int selected=-1;
    CommandMode mode=CommandMode::Move;
    bool paused=false;
    bool rejected=false;
    uint32_t issued=0;

    explicit Controller(uint32_t seed=104) : battle(seed), seed_(seed) {}
    bool Input(const ControlsInput& in) {
        bool changed=false;
        if (in.pointer_x>=0 && in.pointer_x<kWidth && in.pointer_y>=0 && in.pointer_y<kWidth &&
            (in.pointer_x!=last_pointer_x_ || in.pointer_y!=last_pointer_y_)) {
            changed |= (cursor_x!=in.pointer_x || cursor_y!=in.pointer_y);
            cursor_x=in.pointer_x; cursor_y=in.pointer_y;
        }
        last_pointer_x_=in.pointer_x; last_pointer_y_=in.pointer_y;
        const int dx=in.x>0.55f ? 1 : in.x<-.55f ? -1 : 0;
        const int dy=in.y>0.55f ? -1 : in.y<-.55f ? 1 : 0;
        // Repeat after 300 ms then every 115 ms, independent of mouse click.
        if (!dx && !dy) {
            repeat_=0; axis_x_=axis_y_=0;
        } else {
            const bool new_axis=dx!=axis_x_ || dy!=axis_y_;
            axis_x_=dx; axis_y_=dy;
            repeat_+=std::max(0.f,std::min(in.dt,.1f));
            if(new_axis || repeat_>=.115f) {
                if(new_axis) repeat_= -.185f;
                else repeat_ -= .115f;
                cursor_x=std::max(0,std::min(kWidth-1,cursor_x+dx));
                cursor_y=std::max(0,std::min(kWidth-1,cursor_y+dy));
                changed=true;
            }
        }
        if(in.secondary && !secondary_down_) {
            if(battle.result()!=Result::Active) {
                Reset(); changed=true;
            } else if(selected>=0) {
                mode=static_cast<CommandMode>((static_cast<int>(mode)+1)%4);
                changed=true;
            } else {
                paused=!paused;changed=true;
            }
        }
        if(in.primary && !primary_down_) {
            bool valid=Select();
            rejected=!valid;
            if(valid)++issued;
            changed=true;
        }
        primary_down_=in.primary;
        secondary_down_=in.secondary;
        // Player commands are queued while time is completely stopped.
        // An explicit edge advances one logical turn (friendly queued actions,
        // then enemy AI, then objectives). No frame time can ever advance it.
        if(in.commit_turn && !turn_down_) {
            if(!paused && battle.result()==Result::Active) {
                battle.Step();rejected=false;changed=true;
            }else{
                rejected=true;changed=true;
            }
        }
        turn_down_=in.commit_turn;
        return changed;
    }
    void Reset() {
        battle=Battle(seed_);
        selected=-1;
        cursor_x=4;cursor_y=12;
        paused=false;rejected=false;
        mode=CommandMode::Move;
        clock_=0;repeat_=0;axis_x_=axis_y_=0;
        turn_down_=false;
    }
    int CursorUnit() const {
        for(int j=0;j<battle.unit_count();++j) {
            const Unit* u=battle.unit(j);
            if(u && u->alive() && u->x==cursor_x && u->y==cursor_y) return j;
        }
        return -1;
    }
    bool Select() {
        if(battle.result()!=Result::Active) return false;
        const int occupant=CursorUnit();
        if(selected<0) {
            const Unit* u=battle.unit(occupant);
            if(!u || u->side!=Side::Hearthwell) return false;
            // Preserve a mode chosen from the visible tactical palette.
            // A new battle still defaults to Move; selecting a unit does not
            // quietly discard a deliberately chosen Repair/Attack order.
            selected=occupant;return true;
        }
        // Clicking the selected soldier must activate a self-targeted
        // repair/resupply order, not silently deselect the unit.
        if(mode==CommandMode::Repair)return battle.QueueRepair(selected);
        if(mode==CommandMode::Resupply)return battle.QueueResupply(selected);
        if(occupant==selected) { selected=-1;return true; }
        const Unit* actor=battle.unit(selected);
        if(!actor || !actor->alive()) {selected=-1;return false;}
        if(mode==CommandMode::Move) {
            if(occupant>=0) {
                const Unit* u=battle.unit(occupant);
                if(u && u->side==Side::Hearthwell) {selected=occupant;return true;}
            }
            return battle.QueueMove(selected,cursor_x,cursor_y);
        }
        if(mode==CommandMode::Attack) return battle.QueueAttack(selected,occupant);
        return false;
    }
private:
    friend class Checkpoint;
    uint32_t seed_;
    float clock_=0,repeat_=0;
    int axis_x_=0,axis_y_=0,last_pointer_x_=-1,last_pointer_y_=-1;
    bool primary_down_=false,secondary_down_=false,turn_down_=false;
};
}} // namespace reverie::omega
#endif
