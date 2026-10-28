#include "omega_front_menu.h"
#include <cassert>
#include <iostream>

using reverie::omega::Menu;
using reverie::omega::MenuCommand;
using reverie::omega::MenuScreen;

static MenuCommand Tap(Menu &menu,bool saved,int button=0){
    const MenuCommand result=menu.Input(.016f,0,true,false,true,
                                        0,Menu::ButtonY(button,menu.Count(saved)>4),saved);
    menu.Input(.016f,0,false,false,false,0,0,saved);
    return result;
}
static void Back(Menu &menu,bool saved){
    menu.Input(.016f,0,false,true,false,0,0,saved);
    menu.Input(.016f,0,false,false,false,0,0,saved);
}
int main(){
    Menu menu;
    assert(menu.screen==MenuScreen::Title);
    assert(menu.Count(false)==2);
    assert(Tap(menu,false,0)==MenuCommand::None);
    assert(menu.screen==MenuScreen::Campaign);
    assert(menu.Count(false)==3);
    assert(Tap(menu,false,0)==MenuCommand::Resume);
    assert(menu.screen==MenuScreen::Playing);

    // A menu click cannot double as a field action; menu events are explicit.
    assert(menu.Input(.016f,0,false,false)==MenuCommand::None);
    assert(Menu::MenuShortcut(.88f,1.06f));
    assert(Menu::OrderShortcut(-.88f,1.06f));
    assert(!Menu::MenuShortcut(0.f,0.f));
    assert(Menu::ButtonAt(0,Menu::ButtonY(7,true),8)==7);
    assert(Menu::ButtonAt(.91f,Menu::ButtonY(7,true),8)==-1);

    menu.OpenOrders(false,0,false,false);
    assert(menu.screen==MenuScreen::FieldOrders);
    assert(menu.Count(true)==8);
    assert(Tap(menu,true,7)==MenuCommand::SetFieldAction);
    assert(menu.field_choice==7);
    assert(menu.screen==MenuScreen::Playing);

    menu.OpenOrders(true,0,false,false);
    assert(menu.screen==MenuScreen::TacticalOrders);
    assert(Tap(menu,true,3)==MenuCommand::SetCommandMode);
    assert(menu.mode_choice==3);
    assert(menu.screen==MenuScreen::Playing);

    // Pause suspends game advancement in the native update loop; selecting
    // controls or campaign briefing never mutates the running campaign.
    menu.OpenPause(false,true);
    assert(menu.screen==MenuScreen::Pause);
    menu.Input(.016f,0,false,false,false,0,0,true);
    assert(Tap(menu,true,2)==MenuCommand::None);
    assert(menu.screen==MenuScreen::Controls);
    Back(menu,true);
    assert(menu.screen==MenuScreen::Pause);
    assert(Tap(menu,true,1)==MenuCommand::None);
    assert(menu.screen==MenuScreen::Campaign);
    assert(menu.Count(true)==4);
    assert(Tap(menu,true,1)==MenuCommand::None);
    assert(menu.screen==MenuScreen::ConfirmNew);
    Back(menu,true);
    assert(menu.screen==MenuScreen::Campaign);
    assert(Tap(menu,true,1)==MenuCommand::None);
    assert(Tap(menu,true,0)==MenuCommand::StartNew);
    assert(menu.screen==MenuScreen::Playing);

    menu.OpenPause(false,false);
    assert(Tap(menu,true,3)==MenuCommand::None);
    assert(menu.screen==MenuScreen::Title);
    assert(Tap(menu,true,0)==MenuCommand::None);
    assert(Tap(menu,true,0)==MenuCommand::Resume);
    assert(menu.screen==MenuScreen::Playing);

    // The thumbpad navigates menus without requiring any pointing ray.
    Menu axis;
    axis.Input(.016f,-1.f,false,false,false,0,0,false);
    assert(axis.selected==1);
    Back(axis,false); // Title has no parent; does not leave the screen.
    assert(axis.screen==MenuScreen::Title);
    assert(Tap(axis,false,1)==MenuCommand::None);
    assert(axis.screen==MenuScreen::Controls);
    Back(axis,false);
    assert(axis.screen==MenuScreen::Title);
    std::cout << "Omega Front full menu-chain navigation OK\n";
    return 0;
}
