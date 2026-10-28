#ifndef REVERIE_OMEGA_FRONT_MENU_H
#define REVERIE_OMEGA_FRONT_MENU_H

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace reverie { namespace omega {

// Menus are deliberately not part of the campaign checkpoint: reopening the
// module must reach a safe title screen, never fire an order on load.
enum class MenuScreen : uint8_t { Title, Campaign, Controls, ConfirmNew, Pause, FieldOrders, TacticalOrders, Playing };
enum class MenuCommand : uint8_t { None, Resume, StartNew, SetFieldAction, SetCommandMode, CommitTurn };

class Menu {
public:
    MenuScreen screen=MenuScreen::Title;
    int selected=0;
    int field_choice=0,mode_choice=0;
    MenuScreen return_to=MenuScreen::Title;

    static float ButtonY(int index,bool compact=false) {
        return compact?.585f-.187f*index:.42f-.225f*index;
    }
    static bool MenuShortcut(float x,float y) {
        return x>=.72f && x<=1.07f && y>=.965f && y<=1.13f;
    }
    static bool OrderShortcut(float x,float y) {
        return x>=-1.07f && x<=-.72f && y>=.965f && y<=1.13f;
    }
    static int ButtonAt(float x,float y,int count) {
        if(std::fabs(x)>.775f)return -1;
        for(int i=0;i<count;++i)
            if(std::fabs(y-ButtonY(i,count>4))<=(count>4?.072f:.095f))return i;
        return -1;
    }
    int Count(bool has_campaign) const {
        switch(screen) {
        case MenuScreen::Title: return 2;
        case MenuScreen::Campaign: return has_campaign?4:3;
        case MenuScreen::Controls: return 1;
        case MenuScreen::ConfirmNew: return 2;
        case MenuScreen::Pause: return 4;
        case MenuScreen::FieldOrders: return 8;
        case MenuScreen::TacticalOrders: return 5;
        case MenuScreen::Playing: return 0;
        }
        return 0;
    }
    const char *Label(int index,bool has_campaign) const {
        switch(screen) {
        case MenuScreen::Title: {
            const char *v[]={"CAMPAIGN","CONTROLS"};return index>=0&&index<2?v[index]:"";
        }
        case MenuScreen::Campaign: {
            const char *saved[]={"CONTINUE CAMPAIGN","NEW CAMPAIGN","CONTROLS","BACK"};
            const char *fresh[]={"BEGIN CAMPAIGN","CONTROLS","BACK"};
            return index>=0&&index<Count(has_campaign)?
                (has_campaign?saved[index]:fresh[index]):"";
        }
        case MenuScreen::Controls: return index==0?"BACK":"";
        case MenuScreen::ConfirmNew: {
            const char *v[]={"ERASE AND START NEW","CANCEL"};return index>=0&&index<2?v[index]:"";
        }
        case MenuScreen::Pause: {
            const char *v[]={"RESUME","CAMPAIGN BRIEFING","CONTROLS","MAIN MENU"};
            return index>=0&&index<4?v[index]:"";
        }
        case MenuScreen::FieldOrders: {
            const char *v[]={"DEPLOY","FORAGE / CONVOY","RECRUIT","REPAIR / SECURE",
                             "WAIT ONE DAY","SCOUT","PARLEY","ACCORD"};
            return index>=0&&index<8?v[index]:"";
        }
        case MenuScreen::TacticalOrders: {
            const char *v[]={"MOVE","ATTACK","REPAIR SELF","RESUPPLY SELF",
                             "END TURN / RESOLVE"};
            return index>=0&&index<5?v[index]:"";
        }
        case MenuScreen::Playing: return "";
        }
        return "";
    }
    void OpenPause(bool primary,bool secondary) {
        if(screen!=MenuScreen::Playing)return;
        Enter(MenuScreen::Pause);
        primary_down_=primary;secondary_down_=secondary;
    }
    void OpenOrders(bool tactical,int current,bool primary,bool secondary) {
        if(screen!=MenuScreen::Playing)return;
        Enter(tactical?MenuScreen::TacticalOrders:MenuScreen::FieldOrders);
        selected=std::max(0,std::min(tactical?3:7,current));
        primary_down_=primary;secondary_down_=secondary;
    }
    void SyncButtons(bool primary,bool secondary) {
        primary_down_=primary;secondary_down_=secondary;
    }
    MenuCommand Input(float dt,float axis_y,bool primary,bool secondary,
                      bool pointing=false,float pointer_x=0,float pointer_y=0,
                      bool has_campaign=false) {
        if(screen==MenuScreen::Playing) {
            SyncButtons(primary,secondary);
            return MenuCommand::None;
        }
        const bool click=primary&&!primary_down_;
        const bool back=secondary&&!secondary_down_;
        primary_down_=primary;secondary_down_=secondary;
        if(back) { Back();return MenuCommand::None; }
        const int count=Count(has_campaign);
        const int direction=axis_y>.55f?-1:axis_y<-.55f?1:0;
        if(direction==0){axis_direction_=0;repeat_=0;}
        else {
            const bool fresh=direction!=axis_direction_;
            axis_direction_=direction;
            repeat_+=std::max(0.f,std::min(.1f,dt));
            if(fresh||repeat_>=.18f){
                if(fresh)repeat_=-.18f;
                else repeat_-=.18f;
                selected=(selected+direction+count)%count;
            }
        }
        // Only a moving VR cursor changes hover selection. Thumbpad choices
        // remain usable even while the headset stays aimed at a menu button.
        if(pointing) {
            if(!pointer_seen_ || std::fabs(pointer_x-last_x_)>.025f ||
               std::fabs(pointer_y-last_y_)>.025f) {
                const int hit=ButtonAt(pointer_x,pointer_y,count);
                if(hit>=0)selected=hit;
            }
            last_x_=pointer_x;last_y_=pointer_y;pointer_seen_=true;
        } else pointer_seen_=false;
        if(!click)return MenuCommand::None;
        switch(screen) {
        case MenuScreen::Title:
            if(selected==0)Enter(MenuScreen::Campaign);
            else {return_to=MenuScreen::Title;Enter(MenuScreen::Controls);}
            break;
        case MenuScreen::Campaign:
            if(selected==0){Enter(MenuScreen::Playing);return MenuCommand::Resume;}
            if(has_campaign && selected==1){Enter(MenuScreen::ConfirmNew);break;}
            if(selected==(has_campaign?2:1)){
                return_to=MenuScreen::Campaign;Enter(MenuScreen::Controls);
            }else Enter(MenuScreen::Title);
            break;
        case MenuScreen::Controls: Back();break;
        case MenuScreen::ConfirmNew:
            if(selected==0){Enter(MenuScreen::Playing);return MenuCommand::StartNew;}
            Enter(MenuScreen::Campaign);
            break;
        case MenuScreen::FieldOrders:
            field_choice=selected;
            Enter(MenuScreen::Playing);
            return MenuCommand::SetFieldAction;
        case MenuScreen::TacticalOrders:
            if(selected==4){
                Enter(MenuScreen::Playing);
                return MenuCommand::CommitTurn;
            }
            mode_choice=selected;
            Enter(MenuScreen::Playing);
            return MenuCommand::SetCommandMode;
        case MenuScreen::Pause:
            if(selected==0){Enter(MenuScreen::Playing);return MenuCommand::Resume;}
            if(selected==1)Enter(MenuScreen::Campaign);
            else if(selected==2){return_to=MenuScreen::Pause;Enter(MenuScreen::Controls);}
            else Enter(MenuScreen::Title);
            break;
        case MenuScreen::Playing:break;
        }
        return MenuCommand::None;
    }
private:
    void Enter(MenuScreen next) {
        screen=next;selected=0;repeat_=0;axis_direction_=0;pointer_seen_=false;
    }
    void Back(){
        switch(screen){
        case MenuScreen::Title:break;
        case MenuScreen::Campaign:Enter(MenuScreen::Title);break;
        case MenuScreen::Controls:Enter(return_to);break;
        case MenuScreen::ConfirmNew:Enter(MenuScreen::Campaign);break;
        case MenuScreen::Pause:Enter(MenuScreen::Playing);break;
        case MenuScreen::FieldOrders:Enter(MenuScreen::Playing);break;
        case MenuScreen::TacticalOrders:Enter(MenuScreen::Playing);break;
        case MenuScreen::Playing:break;
        }
    }
    bool primary_down_=false,secondary_down_=false,pointer_seen_=false;
    float last_x_=0,last_y_=0,repeat_=0;
    int axis_direction_=0;
};
}} // namespace reverie::omega
#endif
