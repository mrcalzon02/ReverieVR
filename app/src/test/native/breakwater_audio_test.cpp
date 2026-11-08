// Integration tests execute the actual module Update, its real ray hit-tests,
// campaign rules and the host's bounded packed queues. No source-text checks.
#include "../../main/jni/native/breakwater_module.cpp"
#include "reverie_native_sound_queue.h"
#include "reverie_native_spatial_sound_queue.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
unsigned checks=0u;
void Check(bool okay,const std::string &why) {
    ++checks;
    if(!okay)throw std::runtime_error(why);
}
struct Spatial { uint32_t cue;float left,right; };
struct Record {
    uint32_t feedback=0u;
    std::vector<uint32_t> cues;
    std::vector<Spatial> spatial;
    unsigned Count(uint32_t cue) const {
        return static_cast<unsigned>(std::count(cues.begin(),cues.end(),cue));
    }
    bool Has(uint32_t cue) const {return Count(cue)>0u;}
};
struct Capture {
    ReverieNativeSoundQueueV1 sound;
    ReverieNativeSpatialSoundQueue spatial;
    uint32_t feedback=0u;
    unsigned rejected=0u,callbacks=0u;
    unsigned feedback_calls=0u,save_calls=0u;
    int32_t save_result=REVERIE_NATIVE_SAVE_OK;
    Record Drain() {
        Record r;r.feedback=feedback;feedback=0u;
        auto packet=sound.Drain();
        while(packet) {r.cues.push_back(static_cast<uint32_t>(packet&255u));packet>>=8u;}
        packet=spatial.Drain();
        while(packet) {
            r.spatial.push_back({static_cast<uint32_t>(packet&255u),
                static_cast<float>((packet>>8u)&255u)/255.f,
                static_cast<float>((packet>>16u)&255u)/255.f});
            packet>>=24u;
        }
        Check(rejected==0u,"module overflowed or sent an invalid host audio packet");
        Check(r.cues.size()<=8u && r.spatial.size()<=2u,"host packet budget");
        return r;
    }
};
Capture *capture=nullptr;
void OnLog(int32_t,const char *,const char *) {}
void OnFeedback(uint32_t flags) {
    capture->feedback|=flags;++capture->callbacks;++capture->feedback_calls;
}
int32_t OnReadSave(const char *,void *,uint32_t,uint32_t *out_size) {
    *out_size=0u;return REVERIE_NATIVE_SAVE_NOT_FOUND;
}
int32_t OnWriteSave(const char *slot,const void *bytes,uint32_t size) {
    Check(slot && std::strcmp(slot,CheckpointSlot(0u))==0,"write stays in selected checkpoint slot");
    Check(size==Campaign::kSerializedSize || size==0u,"bounded save or explicit reset");
    if(size) {
        Campaign snapshot;
        Check(snapshot.DeserializeShop(static_cast<const uint8_t *>(bytes),size),
              "write callback receives an actual valid campaign checkpoint");
    }
    ++capture->save_calls;
    return capture->save_result;
}
void OnSound(uint32_t cue) {
    if(!capture->sound.Push(cue))++capture->rejected;
    ++capture->callbacks;
}
void OnSpatial(uint32_t cue,float left,float right) {
    if(!capture->spatial.Push(cue,left,right))++capture->rejected;
    ++capture->callbacks;
}
ReverieNativeInputV1 Ray(float x=0.f,float y=.2f,float z=-1.f,float dt=.016f) {
    ReverieNativeInputV1 in{};
    in.struct_size=sizeof(in);in.delta_seconds=dt;
    in.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    in.pointer_direction[0]=x;in.pointer_direction[1]=y;in.pointer_direction[2]=z;
    in.listener_valid=1u;in.listener_right[0]=1.f;
    return in;
}
ReverieNativeInputV1 Menu(menu::Rect r) {return Ray(r.x,r.y,kShopZ);}
ReverieNativeInputV1 Hud(float x,float y=.97f) {
    float z=-1.8f;console::Point(x,y,z);return Ray(x,y,z);
}
ReverieNativeInputV1 Rack(uint8_t id) {
    const auto p=gunmesh::RackLocation(id);return Ray(p.x,p.y,p.z);
}
struct Fixture {
    Capture sounds;
    ReverieNativeHostV1 host{};
    State state;
    Fixture() {
        host.struct_size=sizeof(host);host.abi_version=REVERIE_NATIVE_MODULE_ABI_VERSION;
        host.log=OnLog;host.request_feedback=OnFeedback;
        host.request_sound=OnSound;host.request_spatial_sound=OnSpatial;
        state.host=&host;capture=&sounds;
        LoadState(&state); // exercise the existing no-save optional host path
    }
    Record Step(ReverieNativeInputV1 in) {
        capture=&sounds;Update(&state,&in);auto record=sounds.Drain();
        if(std::getenv("BREAKWATER_AUDIO_TRACE") &&
           (!record.cues.empty() || record.feedback || !record.spatial.empty())) {
            std::cout<<"Update phase="<<static_cast<unsigned>(state.campaign.phase())
                     <<" primary="<<in.primary_down<<" secondary="<<in.secondary_down
                     <<" pointer="<<in.pointer_kind<<" dt="<<in.delta_seconds
                     <<" feedback="<<record.feedback<<" cues=";
            for(auto cue:record.cues)std::cout<<cue<<",";
            std::cout<<" spatial=";
            for(const auto &event:record.spatial)
                std::cout<<event.cue<<":"<<event.left<<"/"<<event.right<<",";
            std::cout<<"\n";
        }
        return record;
    }
    Record Press(ReverieNativeInputV1 in) {
        auto release=in;release.primary_down=release.secondary_down=0u;
        release.delta_seconds=0.f;Step(release);
        in.primary_down=1u;return Step(in);
    }
    unsigned Hover(ReverieNativeInputV1 in,int frames=10) {
        in.primary_down=in.secondary_down=0u;
        unsigned focus=0u;
        for(int i=0;i<frames;++i)
            focus+=(Step(in).feedback&REVERIE_NATIVE_FEEDBACK_FOCUS)!=0u;
        return focus;
    }
    void Shop() {
        state.slot_picker=false;
        Check(state.campaign.Start(),"fixture shop");
    }
    void Saving(int32_t result) {
        host.read_save=OnReadSave;host.write_save=OnWriteSave;
        sounds.save_result=result;
    }
    void Word(unsigned word,uint32_t value) {
        uint8_t data[Campaign::kSerializedSize]{};
        Check(state.campaign.SerializeShop(data,sizeof(data)),"fixture encode");
        for(unsigned byte=0;byte<4;++byte)
            data[word*4u+byte]=static_cast<uint8_t>(value>>(8u*byte));
        Check(state.campaign.DeserializeShop(data,sizeof(data)),"fixture legal checkpoint");
    }
    void RichShop() {Shop();Word(8u,100000u);}
    void Combat() {Shop();Check(state.campaign.BeginWave(),"fixture wave");}
    auto &Enemies() {
        return const_cast<std::array<Enemy,Campaign::kMaxEnemies>&>(state.campaign.enemies());
    }
    auto &Armor() {
        return const_cast<std::array<ArmoredVehicle,Campaign::kMaxArmor>&>(state.campaign.armored_vehicles());
    }
};

void MenuControls() {
    Fixture f;
    Check(f.Hover(Menu(menu::SlotCard(1)))==1u,"slot focus is debounced once");
    Check(f.Hover(Menu(menu::SlotCard(1)))==0u,"stationary slot stays quiet");
    for(int i=0;i<10;++i)
        Check(f.Step(Menu(menu::SlotCard(i%2))).feedback==0u,"brief hover jitter stays quiet");
    auto r=f.Press(Menu(menu::SlotCard(0)));
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_ACTIVATION,"slot activation has one click");
    Check(!f.state.slot_picker,"slot chosen by actual input");
    auto held=Menu(menu::DifficultyCard(0));held.primary_down=1u;
    Check(f.Step(held).cues.empty(),"held slot press cannot choose difficulty");
    Check(f.Hover(Menu(menu::DifficultyCard(2)))==1u,"difficulty focus");
    r=f.Press(Menu(menu::DifficultyCard(2)));
    Check(r.Count(REVERIE_NATIVE_SOUND_BREAKWATER_DIFFICULTY_SELECT)==1u &&
          r.feedback==0u,"one contextual difficulty sound without duplicate click");
    Check(f.state.campaign.difficulty()==Difficulty::Veteran &&
          f.state.campaign.phase()==Phase::Shop,"actual difficulty selected");
    r=f.Press(Ray(0.f,3.f,-1.f));
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_FAILURE,"missed menu rejects distinctly");
    auto invalid=Ray(0.f,3.f,-1.f);invalid.primary_down=1u;
    Check(f.Step(invalid).feedback==0u,"held rejection is not repeated");

    Check(f.Hover(Menu(menu::ShopCard(0)))==1u,"shop card focus");
    const auto before=f.state.campaign.credits();
    r=f.Press(Menu(menu::ShopCard(0)));
    Check(r.Has(REVERIE_NATIVE_SOUND_SHOP_PURCHASE) &&
          f.state.campaign.credits()<before,"real purchase sound");
    r=f.Press(Menu(menu::ShopAction(9)));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_MENU_OPEN) && f.state.armory_page,
          "armory opens audibly");
    const auto owned_before=f.state.campaign.credits();
    r=f.Press(Menu(menu::ShopCard(0)));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_WEAPON_EQUIP) &&
          !r.Has(REVERIE_NATIVE_SOUND_SHOP_PURCHASE) &&
          f.state.campaign.credits()==owned_before,"owned weapon equips without false purchase");
    r=f.Press(Menu(menu::ShopCard(3)));
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_FAILURE,"unaffordable armory rejection");
    r=f.Press(Menu(menu::ShopAction(9)));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_MENU_BACK) && !f.state.armory_page,
          "armory return sound");
    r=f.Press(Menu(menu::kResetCard));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_MENU_OPEN) && f.state.reset_armed,
          "restart confirmation is audible");
    auto reset=Menu(menu::kResetCard);reset.primary_down=1u;
    Check(f.Step(reset).cues.empty() && f.state.reset_armed,"reset requires release");
    r=f.Press(Menu(menu::kResetCard));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_RESTART) &&
          f.state.campaign.phase()==Phase::Briefing,"confirmed reset sound");

    Fixture summary;summary.Shop();summary.state.round_summary=true;
    Check(summary.Hover(Menu(menu::DebriefAction(0)))==1u,"debrief focus");
    r=summary.Press(Menu(menu::DebriefAction(0)));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_MENU_BACK) &&
          !summary.state.round_summary,"debrief returns to shop audibly");
    summary.state.round_summary=true;
    r=summary.Press(Menu(menu::DebriefAction(1)));
    Check(r.Count(REVERIE_NATIVE_SOUND_WAVE_START)==1u && r.feedback==0u,
          "debrief next wave has one start cue");
    auto still=Ray();still.primary_down=1u;
    Check(!summary.Step(still).Has(REVERIE_NATIVE_SOUND_WEAPON_MACHINE),
          "held next-wave press cannot fire");

    Fixture start;start.Shop();r=start.Press(Menu(menu::ShopAction(8)));
    Check(r.Has(REVERIE_NATIVE_SOUND_WAVE_START) && start.state.campaign.phase()==Phase::Combat,
          "shop start button");
    Fixture secondary;secondary.Shop();auto next=Ray();next.secondary_down=1u;
    Check(secondary.Step(next).Has(REVERIE_NATIVE_SOUND_WAVE_START),"secondary wave start");
}

void PersistenceFeedback() {
    Fixture difficulty;difficulty.state.slot_picker=false;
    difficulty.Saving(REVERIE_NATIVE_SAVE_ERROR);
    auto r=difficulty.Press(Menu(menu::DifficultyCard(1)));
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_FAILURE &&
          !r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_DIFFICULTY_SELECT),
          "failed difficulty checkpoint rejects without a success cue");
    Check(difficulty.sounds.save_calls==1u && difficulty.sounds.feedback_calls==1u &&
          difficulty.state.campaign.phase()==Phase::Shop && !difficulty.state.slot_exists[0],
          "failed supported save is reported once and preserves session state");
    auto held=Menu(menu::DifficultyCard(1));held.primary_down=1u;
    r=difficulty.Step(held);
    Check(r.feedback==0u && difficulty.sounds.save_calls==1u,"held failed save does not repeat");

    Fixture purchase;purchase.Shop();purchase.Saving(REVERIE_NATIVE_SAVE_ERROR);
    const auto credits=purchase.state.campaign.credits();
    r=purchase.Press(Menu(menu::ShopCard(0)));
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_FAILURE &&
          !r.Has(REVERIE_NATIVE_SOUND_SHOP_PURCHASE) &&
          purchase.state.campaign.credits()<credits && purchase.sounds.save_calls==1u &&
          purchase.sounds.feedback_calls==1u,"failed purchase save has one error and no misleading success");
    r=purchase.Press(Menu(menu::ShopAction(9)));
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_FAILURE &&
          !r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_MENU_OPEN) && purchase.state.armory_page,
          "save failure suppresses a page confirmation queued before the write");

    Fixture debrief;debrief.Shop();debrief.state.round_summary=true;
    debrief.Saving(REVERIE_NATIVE_SAVE_ERROR);
    r=debrief.Press(Menu(menu::DebriefAction(0)));
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_FAILURE &&
          !r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_MENU_BACK) &&
          !debrief.state.round_summary && debrief.sounds.feedback_calls==1u,
          "failed debrief save is audible without changing existing session flow");
    debrief.state.round_summary=true;
    r=debrief.Press(Menu(menu::DebriefAction(1)));
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_FAILURE &&
          r.Has(REVERIE_NATIVE_SOUND_WAVE_START) &&
          debrief.state.campaign.phase()==Phase::Combat,
          "save error preserves the actual combat-start event");

    Fixture reset;reset.Shop();reset.Saving(REVERIE_NATIVE_SAVE_ERROR);
    reset.Press(Menu(menu::kResetCard));
    r=reset.Press(Menu(menu::kResetCard));
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_FAILURE &&
          !r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_RESTART) &&
          reset.state.campaign.phase()==Phase::Shop && reset.sounds.save_calls==1u,
          "failed checkpoint clear does not claim a confirmed restart");

    Fixture success;success.state.slot_picker=false;success.Saving(REVERIE_NATIVE_SAVE_OK);
    r=success.Press(Menu(menu::DifficultyCard(2)));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_DIFFICULTY_SELECT) &&
          r.feedback==0u && success.sounds.save_calls==1u && success.state.slot_exists[0],
          "successful supported persistence keeps its contextual confirmation");
    Fixture optional;optional.state.slot_picker=false;
    optional.host.write_save=OnWriteSave; // incomplete optional service remains unavailable
    r=optional.Press(Menu(menu::DifficultyCard(0)));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_DIFFICULTY_SELECT) &&
          r.feedback==0u && optional.sounds.save_calls==0u,
          "missing optional save service never rejects an accepted control");

    // Failed confirmations may be queued earlier than the error. Check both
    // orders against the real host packet and retain real shots and outcomes.
    for(bool error_first:{false,true}) {
        Fixture mixed;capture=&mixed.sounds;audio::Frame frame;
        if(error_first)frame.Feedback(REVERIE_NATIVE_FEEDBACK_FAILURE);
        frame.Cue(REVERIE_NATIVE_SOUND_BREAKWATER_MENU_OPEN,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        frame.Cue(REVERIE_NATIVE_SOUND_SHOP_PURCHASE,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        frame.Cue(REVERIE_NATIVE_SOUND_WEAPON_CANNON,REVERIE_NATIVE_FEEDBACK_WEAPON_FIRE);
        frame.Cue(REVERIE_NATIVE_SOUND_WAVE_CLEAR);
        frame.Cue(REVERIE_NATIVE_SOUND_BATTERY_LOST);
        if(!error_first)frame.Feedback(REVERIE_NATIVE_FEEDBACK_FAILURE);
        frame.Flush(&mixed.host);r=mixed.sounds.Drain();
        Check(r.feedback==REVERIE_NATIVE_FEEDBACK_FAILURE &&
              !r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_MENU_OPEN) &&
              !r.Has(REVERIE_NATIVE_SOUND_SHOP_PURCHASE) &&
              r.Has(REVERIE_NATIVE_SOUND_WEAPON_CANNON) &&
              r.Has(REVERIE_NATIVE_SOUND_WAVE_CLEAR) && r.Has(REVERIE_NATIVE_SOUND_BATTERY_LOST),
              "same-frame failure suppresses control confirmations before dispatch, not combat outcomes");
    }
}

void WeaponAndReloadControls() {
    Fixture f;f.RichShop();
    Check(f.state.campaign.PurchaseWeapon(WeaponKind::HeavyGun),"fixture heavy purchase");
    Check(f.state.campaign.BeginWave(),"fixture armed wave");
    Check(f.Hover(Rack(0u))==1u,"physical rack focus");
    auto r=f.Press(Rack(0u));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_WEAPON_EQUIP) &&
          f.state.campaign.selected_weapon()==WeaponKind::MachineGun,"actual rack equip");
    auto drag=Ray();drag.primary_down=1u;
    Check(!f.Step(drag).Has(REVERIE_NATIVE_SOUND_WEAPON_MACHINE),"rack press cannot drag to fire");
    r=f.Press(Rack(4u));
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_FAILURE,"locked rack rejection");
    r=f.Press(Hud(-1.2f,-1.44f));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_WEAPON_EQUIP) &&
          f.state.campaign.selected_weapon()==WeaponKind::HeavyGun,"console weapon equip");
    auto cycle=Ray();cycle.move_x=1.f;
    r=f.Step(cycle);
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_WEAPON_EQUIP) &&
          f.state.campaign.selected_weapon()==WeaponKind::MachineGun,"axis cycle equip");
    Check(!f.Step(cycle).Has(REVERIE_NATIVE_SOUND_BREAKWATER_WEAPON_EQUIP),"axis held quiet");
    cycle.move_x=0.f;f.Step(cycle);cycle.move_x=-1.f;
    Check(f.Step(cycle).Has(REVERIE_NATIVE_SOUND_BREAKWATER_WEAPON_EQUIP),"axis rearmed");
    f.Press(Rack(0u));

    r=f.Press(Ray());
    Check(r.Count(REVERIE_NATIVE_SOUND_WEAPON_MACHINE)==1u &&
          r.cues.front()==REVERIE_NATIVE_SOUND_WEAPON_MACHINE,"player report precedes impact");
    r=f.Press(Ray());
    Check(r.Count(REVERIE_NATIVE_SOUND_BREAKWATER_COOLDOWN)==1u &&
          !r.Has(REVERIE_NATIVE_SOUND_WEAPON_MACHINE),"fresh cadence rejection");
    Check(f.Step(drag).cues.empty(),"held cadence gap does not buzz");
    r=f.Press(Ray(0.f,2.f,-.1f));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_AIM_LIMIT),"out-of-traverse rejection");
    auto reload=Ray();reload.secondary_down=1u;reload.primary_down=1u;
    auto release=Ray();f.Step(release);r=f.Step(reload);
    Check((r.feedback&REVERIE_NATIVE_FEEDBACK_RELOAD_START)!=0u &&
          !r.Has(REVERIE_NATIVE_SOUND_WEAPON_MACHINE),"reload wins simultaneous fire");
    f.Step(release);reload.primary_down=0u;r=f.Step(reload);
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_COOLDOWN),"reload-in-progress rejection");
    unsigned ready=0u;
    release.delta_seconds=.1f;
    for(int i=0;i<20;++i)
        ready+=(f.Step(release).feedback&REVERIE_NATIVE_FEEDBACK_RELOAD_READY)!=0u;
    Check(ready==1u && !f.state.campaign.reloading(),"one real reload completion");
    r=f.Step(reload);
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_FAILURE,"full magazine reload rejection");

    Fixture automatic;automatic.Combat();
    auto fire=Ray(0.f,.35f,-1.f,.1f);fire.primary_down=1u;
    unsigned starts=0u,shots=0u;
    for(int i=0;i<70 && !automatic.state.campaign.reloading();++i) {
        r=automatic.Step(fire);
        starts+=(r.feedback&REVERIE_NATIVE_FEEDBACK_RELOAD_START)!=0u;
        shots+=r.Count(REVERIE_NATIVE_SOUND_WEAPON_MACHINE);
    }
    Check(starts==1u && shots==12u && automatic.state.campaign.magazine_left()==0u,
          "actual magazine depletion starts one automatic reload");
    r=automatic.Press(Ray());
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_DRY_FIRE),"empty weapon dry-fire cue");
    auto held=Ray();held.primary_down=1u;
    Check(!automatic.Step(held).Has(REVERIE_NATIVE_SOUND_BREAKWATER_DRY_FIRE),
          "held empty trigger stays quiet");
}

void SupportAndStationControls() {
    Fixture f;f.RichShop();
    Check(f.state.campaign.Purchase(Upgrade::Artillery),"fixture artillery");
    Check(f.state.campaign.Purchase(Upgrade::Airstrike),"fixture airstrike");
    Check(f.state.campaign.PurchaseSam(),"fixture SAM");
    Check(f.state.campaign.BeginWave(),"fixture support wave");
    f.Enemies()[0]={UnitType::LandingCraft,0.f,-6.f,85,0u,true};
    for(auto &v:f.Armor()) {v.active=true;v.hp=95;v.z=-2.0f;}
    Check(f.Hover(Hud(-2.f))==1u,"support console focus");
    auto r=f.Press(Hud(-2.f));
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_ARTILLERY &&
          r.Count(REVERIE_NATIVE_SOUND_ARMOR_DESTROYED)==1u,
          "real artillery and coalesced armor destruction fit host queue");
    auto drag=Ray();drag.primary_down=1u;
    Check(!f.Step(drag).Has(REVERIE_NATIVE_SOUND_WEAPON_MACHINE),"support press cannot drag to fire");
    r=f.Press(Hud(-2.f));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_SUPPORT_UNAVAILABLE),"spent artillery rejection");
    f.Enemies()[0]={UnitType::Aircraft,0.f,-5.f,100,0u,true};
    r=f.Press(Hud(2.f));
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_AIRSTRIKE &&
          !f.Enemies()[0].active,"real airstrike sound");
    r=f.Press(Hud(2.f));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_SUPPORT_UNAVAILABLE),"spent airstrike rejection");
    r=f.Press(Hud(0.f));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_SUPPORT_ARM) && f.state.sam_armed,"SAM arm");
    r=f.Press(Hud(0.f));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_SUPPORT_DISARM) && !f.state.sam_armed,"SAM disarm");
    f.Press(Hud(0.f));
    r=f.Press(Ray());
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_SUPPORT_UNAVAILABLE) &&
          f.state.sam_armed,"SAM invalid lock rejection");
    f.Enemies()[0]={UnitType::Aircraft,0.f,-5.f,100,0u,true};
    const auto plane=battlefield::Vessel(f.Enemies()[0]);
    r=f.Press(Ray(plane.wx,plane.wy,plane.wz,0.f));
    Check(r.Has(REVERIE_NATIVE_SOUND_SAM_LAUNCH) && !f.Enemies()[0].active &&
          !f.state.sam_armed && f.state.campaign.sam_charges()==0u,"actual SAM launch");
    Check(!f.Step(drag).Has(REVERIE_NATIVE_SOUND_WEAPON_MACHINE),"SAM press stays captured");
    r=f.Press(Hud(0.f));
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_SUPPORT_UNAVAILABLE),"empty SAM rejection");

    Fixture transfer;transfer.Combat();
    float marker[3];defense::Marker(defense::At(0u,1),marker);
    auto site=Ray(marker[0],marker[1],marker[2]);
    Check(transfer.Hover(site)==1u,"defense station focus");
    r=transfer.Press(site);
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_ACTIVATION &&
          transfer.state.defense_points.current==0,"station first press acknowledges arming");
    r=transfer.Press(site);
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_STATION_MOVE) &&
          transfer.state.defense_points.current==1,"actual station transfer sound");
}

void TypedReportsAndImpacts() {
    for(uint8_t kind=0u;kind<kWeaponCount;++kind) {
        Fixture f;f.Shop();f.Word(16u,31u);f.Word(17u,kind);
        Check(f.state.campaign.BeginWave(),"fixture typed weapon wave");
        auto &target=f.Enemies()[0];
        target={UnitType::ArmoredTransport,0.f,-11.f,320,0u,true};
        float x=target.x,y=-.63f,z=target.z;
        battlefield::Vessel(target).Point(x,y,z);
        if(kind==static_cast<uint8_t>(WeaponKind::DirectCannon))y+=3.5f;
        const auto r=f.Press(Ray(x,y,z,0.f));
        const uint32_t cue=REVERIE_NATIVE_SOUND_WEAPON_MACHINE+kind;
        Check(r.Count(cue)==1u && r.cues.front()==cue &&
              target.hp<320 && r.Has(REVERIE_NATIVE_SOUND_HULL_IMPACT),
              "real weapon "+std::to_string(kind)+" report/hull impact; HP="+
              std::to_string(target.hp)+" cues="+
              (r.cues.empty()?std::string("none"):std::to_string(r.cues.front())));
    }
    Fixture sand;sand.Combat();
    auto r=sand.Press(Ray(0.f,-.45f,-1.f,0.f));
    Check(r.Has(REVERIE_NATIVE_SOUND_WEAPON_MACHINE) &&
          r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_SAND_IMPACT),"actual beach miss has sand impact");
    Fixture water;water.Combat();
    r=water.Press(Ray(0.f,0.f,-1.f,0.f));
    Check(r.Has(REVERIE_NATIVE_SOUND_WEAPON_MACHINE) &&
          r.Has(REVERIE_NATIVE_SOUND_WATER_SPLASH),"actual sea miss has water impact");

    Fixture simultaneous;simultaneous.Combat();
    auto &armor=simultaneous.Armor()[0];
    armor.active=true;armor.hp=95;armor.z=-.8f;
    r=simultaneous.Press(Ray());
    Check(r.Has(REVERIE_NATIVE_SOUND_ARMORED_GUN) &&
          r.Has(REVERIE_NATIVE_SOUND_WEAPON_MACHINE) &&
          r.cues.front()==REVERIE_NATIVE_SOUND_WEAPON_MACHINE,
          "actual player report precedes incoming armor fire from the same Update");
}

Record FinishWave(Fixture &f) {
    auto idle=Ray(0.f,.3f,-1.f,.1f);
    for(int n=0;n<1000;++n) {
        for(auto &e:f.Enemies())e.active=false;
        const auto r=f.Step(idle);
        if(f.state.campaign.phase()!=Phase::Combat)return r;
    }
    throw std::runtime_error("fixture wave did not finish");
}
void OutcomesAndLifecycle() {
    Fixture wave;wave.Combat();
    auto r=FinishWave(wave);
    Check(r.Count(REVERIE_NATIVE_SOUND_WAVE_CLEAR)==1u && wave.state.round_summary,
          "one real combat-to-shop cue");
    Check(!wave.Step(Ray()).Has(REVERIE_NATIVE_SOUND_WAVE_CLEAR),"wave-clear does not repeat");
    Fixture victory;victory.Shop();victory.Word(4u,3u);victory.Word(5u,2u);
    victory.Word(6u,1u);victory.Word(7u,victory.state.campaign.waves_this_period());
    Check(victory.state.campaign.BeginWave(),"fixture final wave");
    r=FinishWave(victory);
    Check(victory.state.campaign.phase()==Phase::Victory &&
          r.Count(REVERIE_NATIVE_SOUND_CAMPAIGN_VICTORY)==1u,"actual campaign victory cue");
    r=victory.Press(Ray());
    Check(r.Has(REVERIE_NATIVE_SOUND_BREAKWATER_RESTART) &&
          victory.state.campaign.phase()==Phase::Briefing,"result restart sound");

    Fixture defeat;defeat.Shop();defeat.Word(9u,1u);
    Check(defeat.state.campaign.BeginWave(),"fixture fragile battery");
    auto &armor=defeat.Armor()[0];armor.active=true;armor.hp=95;armor.z=-.8f;
    for(int i=0;i<30 && defeat.state.campaign.phase()==Phase::Combat;++i)
        r=defeat.Step(Ray(0.f,.2f,-1.f,.1f));
    Check(defeat.state.campaign.phase()==Phase::Defeat &&
          r.Count(REVERIE_NATIVE_SOUND_BATTERY_LOST)==1u &&
          r.cues.front()==REVERIE_NATIVE_SOUND_BATTERY_LOST,"defeat precedes its incoming damage cue");

    Fixture pause;pause.Combat();pause.Press(Ray());
    Pause(&pause.state);
    auto held=Ray();held.primary_down=1u;
    Check(!pause.Step(held).Has(REVERIE_NATIVE_SOUND_WEAPON_MACHINE),"resume blocks held trigger");
    for(int i=0;i<4;++i)pause.Step(Ray(0.f,.2f,-1.f,.1f));
    Check(pause.Press(Ray()).Has(REVERIE_NATIVE_SOUND_WEAPON_MACHINE),"fresh post-pause fire");
    capture=&pause.sounds;
    const auto callbacks=pause.sounds.callbacks;
    PrepareFrame(&pause.state);PrepareFrame(&pause.state);
    ReverieNativeEyeV1 eye{};eye.struct_size=sizeof(eye);
    for(int i:{0,5,10,15})eye.view[i]=eye.projection[i]=1.f;
    Draw(&pause.state,&eye);eye.eye_index=1;Draw(&pause.state,&eye);
    Check(pause.sounds.callbacks==callbacks,"prepare and both eye callbacks remain audio-free");
}

void SoundscapeAndCompatibility() {
  for(const float frame_seconds:{1.f/30.f,.1f}) {
    Fixture f;f.Shop();f.Word(6u,1u);
    Check(f.state.campaign.BeginWave(),"fixture night wave");
    f.Enemies()[0]={UnitType::LandingCraft,5.f,-8.f,85,0u,true};
    f.Enemies()[1]={UnitType::Rowboat,-4.f,-8.f,20,0u,true};
    f.Enemies()[2]={UnitType::Aircraft,2.f,-8.f,100,0u,true};
    std::array<float,audio::Count> last{};last.fill(-1.f);
    std::array<bool,audio::Count> active{};
    float now=0.f;
    for(int i=0;i<static_cast<int>(2.2f/frame_seconds);++i) {
        const auto in=Ray(i%2?.08f:-.08f,.2f,-1.f,frame_seconds);
        now+=in.delta_seconds;
        const auto r=f.Step(in);
        for(const auto &event:r.spatial) {
            const auto layer=event.cue-REVERIE_NATIVE_SOUND_BREAKWATER_AMBIENCE_SURF;
            Check(layer<audio::Count,"only approved loop layers use spatial queue");
            Check(event.left>=0.f && event.left<=1.f && event.right>=0.f && event.right<=1.f,
                  "finite normalized stereo gains");
            if(last[layer]>=0.f && event.left+event.right>0.f)
                Check(now-last[layer]<.72f,"loop refresh stays inside 900ms lease");
            last[layer]=now;active[layer]=event.left+event.right>0.f;
            if(layer==audio::Boat)Check(event.right>event.left,"boat pan follows true headset right");
            if(layer==audio::Rowing)Check(event.left>event.right,"rowing pan follows true headset left");
        }
        if(now>.70f)for(unsigned layer=0;layer<audio::Count;++layer)
            Check(active[layer] && now-last[layer]<.90f,"seven loops coexist without queue starvation");
    }
    Check(f.sounds.rejected==0u,"real two-event spatial packet never overflowed");
    const auto enemy_before=f.Enemies();
    const auto hull=f.state.campaign.integrity();
    auto absent=Ray();absent.pointer_kind=REVERIE_NATIVE_POINTER_NONE;
    absent.primary_down=absent.secondary_down=1u;
    for(int i=0;i<8;++i) {
        const auto r=f.Step(absent);
        Check(r.cues.empty() && r.feedback==0u,"pointer loss creates no stray controls");
        for(const auto &event:r.spatial) {
            Check(event.left==0.f && event.right==0.f,"only stop intents while pointer paused");
            active[event.cue-REVERIE_NATIVE_SOUND_BREAKWATER_AMBIENCE_SURF]=false;
        }
    }
    Check(std::none_of(active.begin(),active.end(),[](bool a){return a;}),"all seven loops stopped across bounded packets");
    Check(f.state.campaign.integrity()==hull && f.Enemies()[0].z==enemy_before[0].z,
          "audio shutdown preserves pointer-loss game pause");
  }

    Fixture positions;positions.Combat();
    positions.Enemies()[0]={UnitType::LandingCraft,0.f,-2.6f,85,0u,true};
    const auto engine=[&](ReverieNativeInputV1 input) {
        positions.state.audio_loops.Reset();input.delta_seconds=0.f;
        Spatial boat{};
        for(int i=0;i<4;++i)for(const auto &event:positions.Step(input).spatial)
            if(event.cue==REVERIE_NATIVE_SOUND_BREAKWATER_ENGINE_BOAT)boat=event;
        Check(boat.cue==REVERIE_NATIVE_SOUND_BREAKWATER_ENGINE_BOAT,"placed boat is audible");
        return boat;
    };
    positions.state.defense_points.current=1;
    const auto from_left=engine(Ray());
    positions.state.defense_points.current=2;
    const auto from_right=engine(Ray());
    Check(from_left.right>from_left.left && from_right.left>from_right.right,
          "virtual defense station shifts the true listener in world space");
    const auto other_aim=engine(Ray(2.f,.2f,.3f));
    Check(other_aim.left==from_right.left && other_aim.right==from_right.right,
          "controller pointing does not pan environmental audio");
    auto head_turn=Ray();head_turn.listener_right[0]=-1.f;
    const auto reversed=engine(head_turn);
    Check(reversed.right==from_right.left && reversed.left==from_right.right,
          "headset right vector rotates the stereo image");

    Fixture mono;mono.Combat();mono.Enemies()[0]={UnitType::LandingCraft,5.f,-7.f,85,0u,true};
    auto invalid=Ray();invalid.delta_seconds=0.f;
    invalid.listener_position[0]=std::numeric_limits<float>::quiet_NaN();
    bool centered=false;
    for(int i=0;i<4;++i)for(const auto &event:mono.Step(invalid).spatial)
        if(event.cue==REVERIE_NATIVE_SOUND_BREAKWATER_ENGINE_BOAT)
            centered=event.left==event.right;
    Check(centered,"invalid listener has centered fallback independent of controller");
    mono.Enemies()[0].active=false;
    bool stopped=false;
    for(int i=0;i<3;++i)for(const auto &event:mono.Step(invalid).spatial)
        if(event.cue==REVERIE_NATIVE_SOUND_BREAKWATER_ENGINE_BOAT)
            stopped=event.left==0.f && event.right==0.f;
    Check(stopped,"destroyed/missing boat engine stops");

    Fixture legacy;legacy.Shop();
    legacy.host.struct_size=REVERIE_NATIVE_HOST_V1_FEEDBACK_MIN_SIZE;
    auto r=legacy.Press(Menu(menu::ShopAction(9)));
    Check(r.cues.empty() && r.feedback==REVERIE_NATIVE_FEEDBACK_ACTIVATION,
          "feedback-only host keeps control acknowledgment");
    legacy.state.armory_page=false;
    legacy.Press(Menu(menu::ShopAction(8)));
    r=legacy.Press(Ray());
    Check((r.feedback&REVERIE_NATIVE_FEEDBACK_WEAPON_FIRE)!=0u,"feedback-only weapon fallback");
    r=legacy.Press(Ray(0.f,2.f,-.1f));
    Check(r.feedback==REVERIE_NATIVE_FEEDBACK_FAILURE,"feedback-only rejection fallback");

    Fixture prefix;prefix.Combat();
    // Allocate precisely the old pointer prefix: an unchecked full-struct copy
    // is an out-of-bounds read under ASan, not merely a benign missing field.
    alignas(ReverieNativeInputV1) std::array<uint8_t,REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE> old{};
    auto old_input=Ray();old_input.struct_size=old.size();
    std::memcpy(old.data(),&old_input,old.size());
    capture=&prefix.sounds;
    Update(&prefix.state,reinterpret_cast<const ReverieNativeInputV1 *>(old.data()));
    prefix.sounds.Drain();
    Check(prefix.state.pointer_visible,"old pointer-prefix host remains playable");
}

void SaturatedFramePriority() {
    // Complement module paths with adversarial simultaneous event pressure,
    // flushed through the identical eight-event host packet used above.
    Fixture f;capture=&f.sounds;
    audio::Frame frame;
    for(uint32_t cue:{6u,7u,8u,21u,34u,9u,10u,11u,12u,15u,20u})frame.Cue(cue);
    frame.Cue(REVERIE_NATIVE_SOUND_WEAPON_CANNON,REVERIE_NATIVE_FEEDBACK_WEAPON_FIRE);
    frame.Cue(REVERIE_NATIVE_SOUND_BATTERY_LOST);
    frame.Feedback(REVERIE_NATIVE_FEEDBACK_FOCUS);
    frame.Flush(&f.host);
    const auto r=f.sounds.Drain();
    Check(r.cues.size()==8u && r.cues[0]==REVERIE_NATIVE_SOUND_BATTERY_LOST &&
          r.cues[1]==REVERIE_NATIVE_SOUND_WEAPON_CANNON,"outcome and player report survive saturated frame");
    Check(r.feedback==0u,"focus suppressed by foreground audio");
}
} // namespace

int main() {
    try {
        MenuControls();PersistenceFeedback();WeaponAndReloadControls();SupportAndStationControls();
        TypedReportsAndImpacts();
        OutcomesAndLifecycle();SoundscapeAndCompatibility();SaturatedFramePriority();
        std::cout<<"PASS actual Breakwater Update audio: "<<checks
                 <<" checks, controls/combat/lifecycle, eight cue and two spatial slots\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr<<"FAIL Breakwater audio: "<<error.what()<<" (check "<<checks<<")\n";
        return 1;
    }
}
