#ifndef REVERIE_VR_WORKSHOP_H
#define REVERIE_VR_WORKSHOP_H

// Native headset authoring core. Same REVSCENE model/primitive renderer as
// the desktop tool. No Java VM, executable imports, external paths or new ABI.
#include "reverie_scene_document.h"
#include <algorithm>
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
    if(x< -3.25||x> -1.06)return -1;
    const double row=(1.11-y)/.235;
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
    bool editing=false, dirty=false, mesh_dirty=false, last_primary=false,
         last_secondary=false;
    size_t slot=0, selected=0;
    VrTool tool=VrTool::Move;
    std::string last_error;
    bool Open(const Scene& seed,const VrStore& store) {
        if(!ScenePlayable(seed)||!store.read||!store.write)return false;
        seed_=seed;return LoadSlot(0,store);
    }
    bool LoadSlot(size_t target,const VrStore& store) {
        if(target>=kVrProjectSlots||!store.read)return false;
        std::string bytes;
        const int rc=store.read(store.context,target,bytes);
        if(rc<0){last_error="PROJECT READ FAILED";return false;}
        Scene loaded;
        if(rc==0){loaded=target==0?seed_:Scene{"white_room",{}};}
        else {
            if(bytes.empty()||bytes.size()>kVrSlotMaxBytes){last_error="CORRUPT PROJECT";return false;}
            try {loaded=ParseScene(bytes);}
            catch(const std::exception&){last_error="CORRUPT PROJECT";return false;}
        }
        if(!ScenePlayable(loaded)){last_error="UNSUPPORTED PROJECT";return false;}
        scene=std::move(loaded);slot=target;selected=0;undo_.clear();
        dirty=false;mesh_dirty=true;last_error.clear();
        return true;
    }
    bool Save(const VrStore& store) {
        if(!store.write){last_error="SAVE SERVICE ABSENT";return false;}
        try{
            const std::string data=EncodeVrScene(scene);
            if(!store.write(store.context,slot,data)){
                last_error="SAVE FAILED - CHANGES RETAINED";return false;
            }
        }catch(const std::exception&){
            last_error="INVALID PROJECT - NOT SAVED";return false;
        }
        dirty=false;last_error.clear();return true;
    }
    bool NextProject(const VrStore& store) {
        if(dirty&&!Save(store))return false;
        return LoadSlot((slot+1)%kVrProjectSlots,store);
    }
    bool Add(const std::string& kind) {
        if(!editing||(kind!="box"&&kind!="cylinder")||
           scene.pieces.size()>=kVrEditMaxPieces){last_error="OBJECT LIMIT";return false;}
        Scene old=scene;Piece p;p.kind=kind;p.asset="-";
        do {p.id="vr"+std::to_string(next_id_++);}while(HasId(p.id));
        p.x=.75;p.y=-1.5;p.z=-3.;p.scale=.34;
        scene.pieces.push_back(p);
        Remember(std::move(old));selected=scene.pieces.size()-1;
        return true;
    }
    bool Remove() {
        if(!editing||scene.pieces.empty())return false;
        Scene old=scene;scene.pieces.erase(scene.pieces.begin()+static_cast<long>(selected));
        selected=std::min(selected,scene.pieces.empty()?size_t(0):scene.pieces.size()-1);
        Remember(std::move(old));return true;
    }
    bool Undo() {
        if(!editing||undo_.empty())return false;
        scene=std::move(undo_.back());undo_.pop_back();
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
            PushUndo(std::move(drag_base_));
    }
    VrEvent Step(bool primary,bool secondary,const VrRay& ray,const VrStore& store) {
        const bool pressed=primary&&!last_primary;
        const bool secondary_pressed=secondary&&!last_secondary;
        last_primary=primary;last_secondary=secondary;
        if(secondary_pressed){
            EndDrag();
            if(editing&&dirty&&!Save(store))return VrEvent::Rejected;
            editing=!editing;
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
                    case 9:return NextProject(store)?VrEvent::Project:VrEvent::Rejected;
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
    std::vector<Scene> undo_;
    bool HasId(const std::string& id)const{
        for(const auto& p:scene.pieces)if(p.id==id)return true;
        return false;
    }
    void PushUndo(Scene previous){
        if(undo_.size()==kVrUndo)undo_.erase(undo_.begin());
        undo_.push_back(std::move(previous));
    }
    void Remember(Scene previous){
        PushUndo(std::move(previous));dirty=mesh_dirty=true;
    }
};
}}
#endif
