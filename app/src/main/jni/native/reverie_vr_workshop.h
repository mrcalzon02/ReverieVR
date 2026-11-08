#ifndef REVERIE_VR_WORKSHOP_H
#define REVERIE_VR_WORKSHOP_H

// Native headset authoring core. Same REVSCENE model/primitive renderer as
// the desktop tool. No Java VM, executable imports, external paths or new ABI.
#include "reverie_scene_document.h"
#include "reverie_vr_project_motion.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace reverie { namespace authoring {
constexpr size_t kVrProjectSlots=3, kVrEditMaxPieces=48, kVrUndo=16;
constexpr size_t kVrSlotMaxBytes=65536;
constexpr double kVrMenuLeft=-2.05,kVrMenuRight=-.45;
constexpr double kVrMenuTop=1.11,kVrMenuRowHeight=.235;
enum class VrTool { Move, Rotate, Scale };
enum class VrEvent { None, Selected, Modified, Saved, Play, Edit, Project, Rejected };
struct VrRay { double ox=0,oy=0,oz=0,dx=0,dy=0,dz=0;bool valid=false; };
struct VrStore {
    void* context=nullptr;
    // Read returns 0 for absent, 1 for present, -1 for failed/malformed.
    int (*read)(void*,size_t,std::string&)=nullptr;
    bool (*write)(void*,size_t,const std::string&)=nullptr;
};
inline bool ValidVrRay(const VrRay& ray) {
    const double len=ray.dx*ray.dx+ray.dy*ray.dy+ray.dz*ray.dz;
    return ray.valid&&std::isfinite(len)&&len>=.95&&len<=1.05 &&
        std::isfinite(ray.ox)&&std::isfinite(ray.oy)&&std::isfinite(ray.oz)&&
        std::abs(ray.ox)<1000&&std::abs(ray.oy)<1000&&std::abs(ray.oz)<1000;
}
inline bool PlaneHit(const VrRay& ray,double y,double& x,double& z) {
    if(!ValidVrRay(ray)||std::abs(ray.dy)<.04)return false;
    const double t=(y-ray.oy)/ray.dy;
    if(t<.25||t>12.0)return false;
    x=ray.ox+t*ray.dx;z=ray.oz+t*ray.dz;
    return std::isfinite(x)&&std::isfinite(z);
}
inline int VrMenuHit(const VrRay& ray) {
    if(!ValidVrRay(ray)||ray.dz>=-.04)return -1;
    const double t=(-2.62-ray.oz)/ray.dz;
    if(t<.3||t>9.)return -1;
    const double x=ray.ox+t*ray.dx,y=ray.oy+t*ray.dy;
    if(x<kVrMenuLeft||x>kVrMenuRight)return -1;
    const double row=(kVrMenuTop-y)/kVrMenuRowHeight;
    const int index=static_cast<int>(std::floor(row));
    return index>=0&&index<10?index:-1;
}
inline bool ScenePlayable(const Scene& scene) {
    if(scene.environment!="white_room"&&scene.environment!="forest_glade"&&
       scene.environment!="dune_beach")return false;
    if(scene.pieces.size()>kVrEditMaxPieces)return false;
    try{
        for(const Piece& p:scene.pieces){
            if(p.kind!="box"&&p.kind!="cylinder")return false;
            if(!ValidId(p.id)||p.asset!="-")return false;
            if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||
               !std::isfinite(p.scale)||!std::isfinite(p.yaw)||
               std::abs(p.x)>10000||std::abs(p.y)>10000||std::abs(p.z)>10000||
               p.scale<=0||p.scale>500)return false;
        }
        (void)PrimitiveTriangles(scene);
    }catch(const std::exception&){return false;}
    return true;
}
inline std::string EncodeVrScene(const Scene& scene) {
    if(!ScenePlayable(scene))throw std::invalid_argument("unsafe VR scene");
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out<<std::setprecision(17)<<"REVSCENE\t1\nenvironment\t"<<scene.environment<<"\n";
    for(const auto& p:scene.pieces)
        out<<"piece\t"<<p.id<<"\t"<<p.kind<<"\t"<<p.asset<<"\t"
           <<p.x<<"\t"<<p.y<<"\t"<<p.z<<"\t"<<p.yaw<<"\t"<<p.scale<<"\n";
    const std::string bytes=out.str();
    if(bytes.size()>kVrSlotMaxBytes||ParseScene(bytes).pieces.size()!=scene.pieces.size())
        throw std::invalid_argument("VR save exceeds slot budget");
    return bytes;
}
class VrWorkshop {
public:
    Scene scene;
    std::vector<VrMotion> motions;
    VrLight light;
    VrSound sound;
    bool motion_page=false, lighting_page=false, audio_page=false;
    bool editing=false, dirty=false, mesh_dirty=false, last_primary=false,
         last_secondary=false;
    size_t slot=0, selected=0;
    VrTool tool=VrTool::Move;
    std::string last_error;
    bool recovering=false;
    std::array<bool,kVrProjectSlots> blocked_slots{};
    std::string recovery_notice;
    bool Open(const Scene& seed,const VrStore& store) {
        if(dirty){last_error="SAVE CHANGES BEFORE REOPENING PROJECT";return false;}
        if(!ScenePlayable(seed)||!store.read||!store.write)return false;
        seed_=seed;
        if(LoadSlot(0,store))return true;
        // Prefer an existing valid project before choosing a fresh fallback.
        for(size_t target=1;target<kVrProjectSlots;++target)
            if(LoadSlot(target,store,true))return true;
        for(size_t target=1;target<kVrProjectSlots;++target)
            if(LoadSlot(target,store))return true;
        // A bad set of slots must not prevent launching the recovery screen.
        // The preview is read-only and none of the original bytes are replaced.
        scene=seed_;motions.clear();light=VrLight{};sound=VrSound{};
        selected=slot=0;editing=false;recovering=true;dirty=false;mesh_dirty=true;
        undo_.clear();motion_page=lighting_page=audio_page=false;
        last_error="NO READABLE PROJECT - SELECT TO RETRY";
        UpdateRecoveryNotice();
        return true;
    }
    bool LoadSlot(size_t target,const VrStore& store,bool existingOnly=false) {
        if(target>=kVrProjectSlots||!store.read)return false;
        if(dirty){last_error="SAVE CHANGES BEFORE SWITCHING PROJECT";return false;}
        std::string bytes;
        const int rc=store.read(store.context,target,bytes);
        if(rc<0){BlockSlot(target,"PROJECT READ FAILED");return false;}
        if(rc==0&&existingOnly){blocked_slots[target]=false;return false;}
        Scene loaded;
        std::vector<VrMotion> loadedMotions;
        VrLight loadedLight;
        VrSound loadedSound;
        if(rc==0){loaded=target==0?seed_:Scene{"white_room",{}};}
        else {
            if(bytes.empty()||bytes.size()>kVrSlotMaxBytes){
                BlockSlot(target,"CORRUPT PROJECT");return false;
            }
            try {
                auto result=DecodeVrBundle(bytes);
                loaded=std::move(result.scene);
                loadedMotions=std::move(result.motions);
                loadedLight=std::move(result.light);
                loadedSound=std::move(result.sound);
            }catch(const std::exception&){
                BlockSlot(target,"CORRUPT PROJECT");return false;
            }
        }
        if(!ScenePlayable(loaded)||!ValidateMotions(loaded,loadedMotions)||
           !ValidLight(loaded,loadedLight)||!ValidVrSound(loaded,loadedSound)){
            BlockSlot(target,"UNSUPPORTED PROJECT");return false;
        }
        scene=std::move(loaded);motions=std::move(loadedMotions);
        light=std::move(loadedLight);
        sound=std::move(loadedSound);sound_elapsed_=0.;
        slot=target;selected=0;motion_page=false;lighting_page=false;
        audio_page=false;undo_.clear();
        dirty=false;mesh_dirty=true;last_error.clear();recovering=false;
        blocked_slots[target]=false;UpdateRecoveryNotice();
        return true;
    }
    bool Save(const VrStore& store) {
        if(recovering||slot>=kVrProjectSlots||blocked_slots[slot]) {
            last_error="UNREADABLE SLOT RETAINED - RETRY OR USE ANOTHER PROJECT";return false;
        }
        if(!store.write){last_error="SAVE SERVICE ABSENT";return false;}
        try{
            const std::string data=EncodeVrBundle(EncodeVrScene(scene),
                EncodeMotions(scene,motions),EncodeVrLight(scene,light),
                EncodeVrSound(scene,sound));
            if(!store.write(store.context,slot,data)){
                last_error="SAVE FAILED - CHANGES RETAINED";return false;
            }
        }catch(const std::exception&){
            last_error="INVALID PROJECT - NOT SAVED";return false;
        }
        dirty=false;last_error.clear();return true;
    }
    bool NextProject(const VrStore& store) {
        if(recovering)return Open(seed_,store)&&!recovering;
        if(dirty&&!Save(store))return false;
        const size_t previous=slot;
        for(size_t offset=1;offset<kVrProjectSlots;++offset)
            if(LoadSlot((previous+offset)%kVrProjectSlots,store))return true;
        last_error="OTHER PROJECTS UNREADABLE - CURRENT KEPT";
        return false;
    }
    bool Add(const std::string& kind) {
        if(!editing||(kind!="box"&&kind!="cylinder")||
           scene.pieces.size()>=kVrEditMaxPieces){last_error="OBJECT LIMIT";return false;}
        Scene old=scene;Piece p;p.kind=kind;p.asset="-";
        do {p.id="vr"+std::to_string(next_id_++);}while(HasId(p.id));
        p.x=.75;p.y=-1.5;p.z=-3.;p.scale=.34;
        scene.pieces.push_back(p);
        Remember(std::move(old),motions);selected=scene.pieces.size()-1;
        return true;
    }
    bool Remove() {
        if(!editing||scene.pieces.empty())return false;
        Scene old=scene;
        std::vector<VrMotion> before=motions;
        const VrLight beforeLight=light;
        const VrSound beforeSound=sound;
        const std::string removed=scene.pieces[selected].id;
        scene.pieces.erase(scene.pieces.begin()+static_cast<long>(selected));
        motions.erase(std::remove_if(motions.begin(),motions.end(),
            [&](const VrMotion& m){return m.id==removed;}),motions.end());
        if(light.enabled&&light.anchor_id==removed)light=VrLight{};
        if(sound.enabled&&sound.anchor_id==removed)sound=VrSound{};
        selected=std::min(selected,scene.pieces.empty()?size_t(0):scene.pieces.size()-1);
        Remember(std::move(old),std::move(before),true,&beforeLight,&beforeSound);return true;
    }
    bool Undo() {
        if(!editing||undo_.empty())return false;
        scene=std::move(undo_.back().scene);
        motions=std::move(undo_.back().motions);
        light=std::move(undo_.back().light);
        sound=std::move(undo_.back().sound);undo_.pop_back();
        selected=std::min(selected,scene.pieces.empty()?size_t(0):scene.pieces.size()-1);
        dirty=mesh_dirty=true;return true;
    }
    bool Select(const VrRay& ray) {
        if(!editing||!ValidVrRay(ray))return false;
        double nearest=12.0;size_t hit=scene.pieces.size();
        for(size_t i=0;i<scene.pieces.size();++i){
            const auto& p=scene.pieces[i];
            const double cx=p.x,cy=p.y+p.scale,cz=p.z;
            const double vx=cx-ray.ox,vy=cy-ray.oy,vz=cz-ray.oz;
            const double proj=vx*ray.dx+vy*ray.dy+vz*ray.dz;
            if(proj<.15||proj>=nearest)continue;
            const double r=p.scale*1.8;
            const double d2=vx*vx+vy*vy+vz*vz-proj*proj;
            if(d2<=r*r){hit=i;nearest=proj;}
        }
        if(hit==scene.pieces.size())return false;
        selected=hit;return true;
    }
    bool BeginDrag(){if(!editing||scene.pieces.empty())return false;
        drag_base_=scene;dragging_=true;return true;}
    bool Drag(const VrRay& ray) {
        if(!dragging_||!editing||selected>=scene.pieces.size()||
           !ValidVrRay(ray))return false;
        Piece& p=scene.pieces[selected];
        Piece candidate=p;
        if(tool==VrTool::Move){
            double x,z;
            if(!PlaneHit(ray,-1.5,x,z))return false;
            if(std::abs(x)>3.1||z< -4.25||z>-.6)return false;
            candidate.x=std::round(x*4.)/4.;
            candidate.z=std::round(z*4.)/4.;
        }else if(tool==VrTool::Rotate) {
            if(std::abs(ray.dz)<.04)return false;
            const double t=(p.z-ray.oz)/ray.dz;
            if(t<.25||t>12.)return false;
            const double x=ray.ox+t*ray.dx;
            candidate.yaw=std::round((x-p.x)*45./15.)*15.;
        }else{
            if(std::abs(ray.dz)<.04)return false;
            const double t=(p.z-ray.oz)/ray.dz;
            if(t<.25||t>12.)return false;
            candidate.scale=std::max(.1,std::min(2.,std::round(std::abs(ray.ox+t*ray.dx-p.x)*4.)/4.));
        }
        if(p.x==candidate.x&&p.z==candidate.z&&p.yaw==candidate.yaw&&
           p.scale==candidate.scale)return false;
        p=std::move(candidate);dirty=mesh_dirty=true;return true;
    }
    void EndDrag() {
        if(!dragging_)return;
        dragging_=false;
        if(scene.pieces.size()==drag_base_.pieces.size() &&
           EncodeVrScene(scene)!=EncodeVrScene(drag_base_))
            PushUndo({std::move(drag_base_),motions,light,sound});
    }

    const VrMotion* SelectedMotion()const{
        if(scene.pieces.empty()||selected>=scene.pieces.size())return nullptr;
        const std::string& id=scene.pieces[selected].id;
        for(const auto& motion:motions)if(motion.id==id)return &motion;
        return nullptr;
    }
    VrMotionSample SampleFor(const Piece& piece,double time)const{
        if(editing)return {};
        for(const auto& motion:motions)
            if(motion.id==piece.id)return SampleMotion(motion,time);
        return {};
    }
    bool SetMotion(VrMotionKind kind){
        if(!editing||scene.pieces.empty())return false;
        auto before=motions;
        const std::string id=scene.pieces[selected].id;
        bool updated=false;
        for(auto& motion:motions)if(motion.id==id){
            motion.kind=kind;updated=true;break;
        }
        if(!updated){
            if(motions.size()>=kVrMotionMax){last_error="MOTION LIMIT";return false;}
            motions.push_back({id,kind,1.0,.5});
        }
        Remember(scene,std::move(before),false);return true;
    }
    bool ClearMotion(){
        if(!editing||scene.pieces.empty())return false;
        auto before=motions;
        const std::string id=scene.pieces[selected].id;
        motions.erase(std::remove_if(motions.begin(),motions.end(),
            [&](const VrMotion& m){return m.id==id;}),motions.end());
        if(before.size()==motions.size())return false;
        Remember(scene,std::move(before),false);return true;
    }
    bool TuneMotion(bool speed,bool increase){
        if(!editing)return false;
        const VrMotion* selectedMotion=SelectedMotion();
        if(!selectedMotion)return false;
        auto before=motions;
        for(auto& motion:motions)if(motion.id==selectedMotion->id){
            double& value=speed?motion.speed:motion.amount;
            const double maximum=speed?6.0:2.0,minimum=speed?.1:.05;
            const double delta=speed?.25:.10;
            const double next=std::max(minimum,std::min(maximum,
                std::round((value+(increase?delta:-delta))*100.)/100.));
            if(next==value)return false;
            value=next;break;
        }
        Remember(scene,std::move(before),false);return true;
    }


    bool AttachLight(){
        if(!editing||scene.pieces.empty())return false;
        const std::string id=scene.pieces[selected].id;
        if(light.enabled&&light.anchor_id==id)return false;
        const VrLight prior=light;
        light.enabled=true;light.anchor_id=id;
        Remember(scene,motions,false,&prior);return true;
    }
    bool ClearLight(){
        if(!editing||!light.enabled)return false;
        const VrLight prior=light;
        light=VrLight{};
        Remember(scene,motions,false,&prior);return true;
    }
    bool TuneLight(int parameter,bool increase){
        if(!editing||!light.enabled||parameter<0||parameter>2)return false;
        const VrLight prior=light;
        double& value=parameter==0?light.power:
            parameter==1?light.radius:light.ambient;
        const double min=parameter==0?.10:parameter==1?1.:0.;
        const double max=parameter==0?3.:parameter==1?8.:1.05;
        const double step=parameter==0?.25:parameter==1?.5:.05;
        const double next=std::max(min,std::min(max,
            std::round((value+(increase?step:-step))*100.)/100.));
        if(next==value)return false;
        value=next;Remember(scene,motions,false,&prior);return true;
    }
    bool LightPosition(double time,float& x,float& y,float& z)const{
        if(!light.enabled)return false;
        for(const auto& p:scene.pieces)if(p.id==light.anchor_id){
            x=static_cast<float>(p.x);
            y=static_cast<float>(p.y+1.5*p.scale+SampleFor(p,time).offset_y);
            z=static_cast<float>(p.z);
            return true;
        }
        return false;
    }


    bool AttachSound(){
        if(!editing||scene.pieces.empty())return false;
        const std::string id=scene.pieces[selected].id;
        if(sound.enabled&&sound.anchor_id==id)return false;
        const VrSound old=sound;
        sound.enabled=true;sound.anchor_id=id;
        Remember(scene,motions,false,nullptr,&old);return true;
    }
    bool ClearSound(){
        if(!editing||!sound.enabled)return false;
        const VrSound old=sound;
        sound=VrSound{};
        Remember(scene,motions,false,nullptr,&old);return true;
    }
    bool TuneSound(int parameter,bool increase){
        if(!editing||!sound.enabled||parameter<0||parameter>2)return false;
        const VrSound old=sound;
        if(parameter==0){
            if(increase){if(sound.cue>=21u)return false;++sound.cue;}
            else{if(sound.cue<=1u)return false;--sound.cue;}
        }else{
            double& value=parameter==1?sound.interval:sound.radius;
            const double lo=parameter==1?.5:1.,hi=parameter==1?10.:12.;
            const double delta=parameter==1?.5:1.;
            const double next=std::max(lo,std::min(hi,value+(increase?delta:-delta)));
            if(next==value)return false;
            value=next;
        }
        Remember(scene,motions,false,nullptr,&old);return true;
    }
    bool TickSound(double dt,double time,double lx,double ly,double lz,
                   double rx,double rz,uint32_t& cue,VrStereoCue& spatial){
        if(editing||!sound.enabled||!std::isfinite(dt)||dt<0||dt>.1)return false;
        sound_elapsed_=std::min(sound.interval,sound_elapsed_+dt);
        // Allow sub-microsecond accumulation error without firing early by a frame.
        if(sound_elapsed_+1e-7<sound.interval)return false;
        for(const auto& p:scene.pieces)if(p.id==sound.anchor_id){
            const VrMotionSample motion=SampleFor(p,time);
            spatial=SpatializeVrSound(p.x,p.y+p.scale+motion.offset_y,p.z,
                                      lx,ly,lz,rx,rz,sound.radius);
            if(!spatial.audible)return false;
            sound_elapsed_=0.;cue=sound.cue;return true;
        }
        return false;
    }

    VrEvent Step(bool primary,bool secondary,const VrRay& ray,const VrStore& store) {
        const bool pressed=primary&&!last_primary;
        const bool secondary_pressed=secondary&&!last_secondary;
        last_primary=primary;last_secondary=secondary;
        if(recovering) {
            if(pressed||secondary_pressed)
                return Open(seed_,store)&&!recovering?VrEvent::Project:VrEvent::Rejected;
            return VrEvent::None;
        }
        if(secondary_pressed){
            EndDrag();
            if(editing&&dirty&&!Save(store))return VrEvent::Rejected;
            editing=!editing;
            motion_page=false;lighting_page=false;audio_page=false;
            sound_elapsed_=0.;
            return editing?VrEvent::Edit:VrEvent::Play;
        }
        if(!editing){
            // Preview only. No editor input reaches unrelated native games.
            return VrEvent::None;
        }
        if(!primary){EndDrag();return VrEvent::None;}
        if(pressed){
            const int menu=VrMenuHit(ray);
            if(menu>=0){
                if(audio_page){
                    switch(menu){
                        case 0:return AttachSound()?VrEvent::Modified:VrEvent::Rejected;
                        case 1:return ClearSound()?VrEvent::Modified:VrEvent::Rejected;
                        case 2:return TuneSound(0,true)?VrEvent::Modified:VrEvent::Rejected;
                        case 3:return TuneSound(0,false)?VrEvent::Modified:VrEvent::Rejected;
                        case 4:return TuneSound(1,true)?VrEvent::Modified:VrEvent::Rejected;
                        case 5:return TuneSound(1,false)?VrEvent::Modified:VrEvent::Rejected;
                        case 6:return TuneSound(2,true)?VrEvent::Modified:VrEvent::Rejected;
                        case 7:return TuneSound(2,false)?VrEvent::Modified:VrEvent::Rejected;
                        case 8:return NextProject(store)?VrEvent::Project:VrEvent::Rejected;
                        case 9:audio_page=false;return VrEvent::Selected;
                    }
                }
                if(lighting_page){
                    switch(menu){
                        case 0:return AttachLight()?VrEvent::Modified:VrEvent::Rejected;
                        case 1:return ClearLight()?VrEvent::Modified:VrEvent::Rejected;
                        case 2:return TuneLight(0,true)?VrEvent::Modified:VrEvent::Rejected;
                        case 3:return TuneLight(0,false)?VrEvent::Modified:VrEvent::Rejected;
                        case 4:return TuneLight(1,true)?VrEvent::Modified:VrEvent::Rejected;
                        case 5:return TuneLight(1,false)?VrEvent::Modified:VrEvent::Rejected;
                        case 6:return TuneLight(2,true)?VrEvent::Modified:VrEvent::Rejected;
                        case 7:return TuneLight(2,false)?VrEvent::Modified:VrEvent::Rejected;
                        case 8:audio_page=true;return VrEvent::Selected;
                        case 9:lighting_page=false;return VrEvent::Selected;
                    }
                }
                if(motion_page){
                    switch(menu){
                        case 0:return SetMotion(VrMotionKind::Bob)?VrEvent::Modified:VrEvent::Rejected;
                        case 1:return SetMotion(VrMotionKind::Spin)?VrEvent::Modified:VrEvent::Rejected;
                        case 2:return SetMotion(VrMotionKind::Pulse)?VrEvent::Modified:VrEvent::Rejected;
                        case 3:return ClearMotion()?VrEvent::Modified:VrEvent::Rejected;
                        case 4:return TuneMotion(true,true)?VrEvent::Modified:VrEvent::Rejected;
                        case 5:return TuneMotion(true,false)?VrEvent::Modified:VrEvent::Rejected;
                        case 6:return TuneMotion(false,true)?VrEvent::Modified:VrEvent::Rejected;
                        case 7:return TuneMotion(false,false)?VrEvent::Modified:VrEvent::Rejected;
                        case 8:lighting_page=true;return VrEvent::Selected;
                        case 9:motion_page=false;return VrEvent::Selected;
                    }
                }
                switch(menu){
                    case 0:return Add("box")?VrEvent::Modified:VrEvent::Rejected;
                    case 1:return Add("cylinder")?VrEvent::Modified:VrEvent::Rejected;
                    case 2:tool=VrTool::Move;return VrEvent::Selected;
                    case 3:tool=VrTool::Rotate;return VrEvent::Selected;
                    case 4:tool=VrTool::Scale;return VrEvent::Selected;
                    case 5:return Remove()?VrEvent::Modified:VrEvent::Rejected;
                    case 6:return Undo()?VrEvent::Modified:VrEvent::Rejected;
                    case 7:return Save(store)?VrEvent::Saved:VrEvent::Rejected;
                    case 8:if(dirty&&!Save(store))return VrEvent::Rejected;
                           editing=false;return VrEvent::Play;
                    case 9:motion_page=true;return VrEvent::Selected;
                }
            }
            if(Select(ray)){BeginDrag();return VrEvent::Selected;}
            return VrEvent::None;
        }
        // Drag only after a selection began on a prior press; bounded updates
        // are driven by actual pointer movement, not simulated per-eye steps.
        return Drag(ray)?VrEvent::Modified:VrEvent::None;
    }
private:
    Scene seed_,drag_base_;
    bool dragging_=false;
    uint32_t next_id_=1;
    struct Snapshot {Scene scene;std::vector<VrMotion> motions;VrLight light;VrSound sound;};
    double sound_elapsed_=0.;
    std::vector<Snapshot> undo_;
    void UpdateRecoveryNotice() {
        recovery_notice.clear();
        for(size_t index=0;index<kVrProjectSlots;++index)if(blocked_slots[index]) {
            if(recovery_notice.empty())recovery_notice="UNREADABLE PROJECTS KEPT: ";
            else recovery_notice+=", ";
            recovery_notice+=std::to_string(index+1);
        }
    }
    void BlockSlot(size_t target,const char* reason) {
        blocked_slots[target]=true;last_error=reason;UpdateRecoveryNotice();
    }
    bool HasId(const std::string& id)const{
        for(const auto& p:scene.pieces)if(p.id==id)return true;
        return false;
    }
    void PushUndo(Snapshot previous){
        if(undo_.size()==kVrUndo)undo_.erase(undo_.begin());
        undo_.push_back(std::move(previous));
    }
    void Remember(Scene previous,std::vector<VrMotion> previousMotions,
                  bool geometry=true,const VrLight* previousLight=nullptr,
                  const VrSound* previousSound=nullptr){
        PushUndo({std::move(previous),std::move(previousMotions),
            previousLight?*previousLight:light,
            previousSound?*previousSound:sound});
        dirty=true;if(geometry)mesh_dirty=true;
        last_error.clear();
    }
};
}}
#endif
