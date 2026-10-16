#ifndef REVERIE_OMEGA_FRONT_SESSION_H
#define REVERIE_OMEGA_FRONT_SESSION_H

#include "omega_front_frontier.h"
#include "omega_front_controls.h"
#include <algorithm>
#include <cstdint>

namespace reverie { namespace omega {

enum class FrontMode : uint8_t { SectorMap, Tactical };
enum class FieldAction : uint8_t { Deploy, Forage, Recruit, Repair, Wait };

// Shell-independent, seated table interaction. The current sector is distinct
// from the focused cell: selecting a distant region does not teleport troops.
class Session {
public:
    Frontier frontier;
    Controller tactical;
    FrontMode view=FrontMode::SectorMap;
    FieldAction action=FieldAction::Deploy;
    int focused=6;
    bool rejected=false;
    uint32_t orders=0;

    explicit Session(uint32_t seed=414) : frontier(seed), tactical(seed) {}
    bool Input(const ControlsInput &in) {
        if(view==FrontMode::Tactical) {
            // An ended battle is an after-action report, not a reset button.
            // Select returns to the strategic map and applies losses once.
            if(tactical.battle.result()!=Result::Active) {
                const bool click=in.primary && !primary_;
                primary_=in.primary; secondary_=in.secondary;
                if(!click)return false;
                if(frontier.current_battle())*frontier.current_battle()=tactical.battle;
                const bool applied=frontier.ResolveOperation();
                rejected=!applied;
                if(applied){view=FrontMode::SectorMap;focused=frontier.location();++orders;}
                return true;
            }
            primary_=in.primary;secondary_=in.secondary;
            const bool changed=tactical.Input(in);
            if(changed && frontier.current_battle())
                *frontier.current_battle()=tactical.battle;
            return changed;
        }
        bool changed=false;
        if(in.pointer_x>=0 && in.pointer_x<4 && in.pointer_y>=0 && in.pointer_y<4 &&
           (in.pointer_x!=last_px_ || in.pointer_y!=last_py_)) {
            const int next=in.pointer_y*4+in.pointer_x;
            if(next!=focused){focused=next;changed=true;}
        }
        last_px_=in.pointer_x;last_py_=in.pointer_y;
        const int dx=in.x>.55f?1:in.x<-.55f?-1:0;
        const int dy=in.y>.55f?-1:in.y<-.55f?1:0;
        if(!dx && !dy){axis_x_=axis_y_=0;repeat_=0;}
        else {
            const bool new_axis=dx!=axis_x_||dy!=axis_y_;
            axis_x_=dx;axis_y_=dy;
            repeat_+=std::max(0.f,std::min(in.dt,.1f));
            if(new_axis||repeat_>=.115f){
                if(new_axis)repeat_=-.185f;
                else repeat_-=.115f;
                const int x=std::max(0,std::min(3,focused%4+dx));
                const int y=std::max(0,std::min(3,focused/4+dy));
                const int next=y*4+x;
                if(next!=focused){focused=next;changed=true;}
            }
        }
        if(in.secondary && !secondary_) {
            action=static_cast<FieldAction>((static_cast<int>(action)+1)%5);
            changed=true;
        }
        if(in.primary && !primary_){
            bool performed=false;
            if(focused!=frontier.location())performed=frontier.Travel(focused);
            else {
                switch(action){
                case FieldAction::Deploy:
                    performed=frontier.StartOperation();
                    if(performed){
                        tactical=Controller(frontier.seed());
                        tactical.battle=*frontier.current_battle();
                        view=FrontMode::Tactical;
                        // The click that enters combat must not instantly
                        // select a unit or advance the simulation clock.
                        tactical.Input({0,0,0,true,false,-1,-1});
                        tactical.rejected=false;
                    }
                    break;
                case FieldAction::Forage:performed=frontier.ForageRations();break;
                case FieldAction::Recruit:performed=frontier.Recruit();break;
                case FieldAction::Repair:
                    performed=frontier.location()==10?frontier.RepairPump():frontier.RepairBridge();
                    break;
                case FieldAction::Wait:performed=frontier.AdvanceDay();break;
                }
            }
            rejected=!performed;
            if(performed)++orders;
            changed=true;
        }
        primary_=in.primary;secondary_=in.secondary;
        return changed;
    }
    void ResetEdges(){
        primary_=secondary_=false;last_px_=last_py_=-1;
        axis_x_=axis_y_=0;repeat_=0;
    }
    uint64_t Checksum() const {
        uint64_t h=frontier.Checksum();
        auto add=[&h](uint64_t v){for(int i=0;i<8;++i){h^=(v>>(i*8))&255u;h*=UINT64_C(1099511628211);}};
        add(tactical.battle.Checksum());add(focused);add(static_cast<unsigned>(view));
        add(static_cast<unsigned>(action));add(orders);add(tactical.issued);
        return h;
    }
private:
    friend class CampaignSave;
    bool primary_=false,secondary_=false;
    int axis_x_=0,axis_y_=0,last_px_=-1,last_py_=-1;
    float repeat_=0;
};

}} // namespace reverie::omega
#endif
