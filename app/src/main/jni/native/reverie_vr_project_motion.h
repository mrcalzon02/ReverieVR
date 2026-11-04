#ifndef REVERIE_VR_PROJECT_MOTION_H
#define REVERIE_VR_PROJECT_MOTION_H
// Restricted native VR Workshop animation components. Pure C++17 and no
// arbitrary script, filesystem, GL call, dynamic code or network access.
#include "reverie_scene_document.h"
#include "reverie_vr_project_lighting.h"
#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <locale>

namespace reverie { namespace authoring {

constexpr size_t kVrBundleMaxBytes=65536u, kVrMotionMax=48u;
enum class VrMotionKind { Spin, Bob, Pulse };
struct VrMotion {
    std::string id;
    VrMotionKind kind=VrMotionKind::Spin;
    double speed=1.0, amount=1.0;
};
struct VrMotionSample {
    float offset_y=0.f;
    float spin=0.f;
    float emissive=0.f;
};
inline const char* MotionKindName(VrMotionKind k){
    switch(k){
        case VrMotionKind::Spin:return "SPIN";
        case VrMotionKind::Bob:return "BOB";
        case VrMotionKind::Pulse:return "PULSE";
    }
    return "";
}
inline VrMotionKind ParseMotionKind(const std::string& s){
    if(s=="SPIN")return VrMotionKind::Spin;
    if(s=="BOB")return VrMotionKind::Bob;
    if(s=="PULSE")return VrMotionKind::Pulse;
    throw std::invalid_argument("unknown animation type");
}
inline bool HasObject(const Scene& scene,const std::string& id){
    for(const auto& p:scene.pieces)if(p.id==id)return true;
    return false;
}
inline bool ValidateMotions(const Scene& scene,const std::vector<VrMotion>& motions){
    if(motions.size()>kVrMotionMax)return false;
    std::set<std::string> ids;
    for(const auto& m:motions){
        if(!ValidId(m.id)||!HasObject(scene,m.id)||!ids.insert(m.id).second ||
           !(m.kind==VrMotionKind::Spin||m.kind==VrMotionKind::Bob||
             m.kind==VrMotionKind::Pulse) ||
           !std::isfinite(m.speed)||m.speed<.1||m.speed>6. ||
           !std::isfinite(m.amount)||m.amount<.05||m.amount>2.)return false;
    }
    return true;
}
inline VrMotionSample SampleMotion(const VrMotion& m,double seconds){
    if(!std::isfinite(seconds)||seconds<0)return {};
    constexpr double tau=6.28318530717958647692;
    const double phase=std::fmod(seconds*m.speed,tau);
    if(m.kind==VrMotionKind::Spin)
        return {0.f,static_cast<float>(phase),0.f};
    if(m.kind==VrMotionKind::Bob)
        return {static_cast<float>(m.amount*std::sin(phase)),0.f,0.f};
    return {0.f,0.f,static_cast<float>(m.amount*(.5+.5*std::sin(phase)))};
}
inline std::string EncodeMotions(const Scene& scene,const std::vector<VrMotion>& motions){
    if(!ValidateMotions(scene,motions))throw std::invalid_argument("invalid animation bindings");
    std::ostringstream o;
    o.imbue(std::locale::classic());
    o<<std::setprecision(17)<<"RVMOTION\t1\n";
    for(const auto& m:motions)
        o<<"motion\t"<<m.id<<"\t"<<MotionKindName(m.kind)<<"\t"
          <<m.speed<<"\t"<<m.amount<<"\n";
    return o.str();
}
inline std::vector<VrMotion> ParseMotions(const Scene& scene,const std::string& text){
    if(text.empty()||text.size()>8192u||
       text.compare(0,11,"RVMOTION\t1\n")!=0||text.back()!='\n')
        throw std::invalid_argument("invalid motion document");
    std::vector<VrMotion> motions;
    size_t cursor=11;
    while(cursor<text.size()){
        const auto end=text.find('\n',cursor);
        if(end==std::string::npos)throw std::invalid_argument("unterminated motion");
        const auto fields=Fields(text.substr(cursor,end-cursor));
        if(fields.size()!=5||fields[0]!="motion")
            throw std::invalid_argument("invalid motion entry");
        VrMotion m;
        m.id=fields[1];m.kind=ParseMotionKind(fields[2]);
        m.speed=Number(fields[3]);m.amount=Number(fields[4]);
        motions.push_back(m);
        if(motions.size()>kVrMotionMax)throw std::invalid_argument("too many animations");
        cursor=end+1;
    }
    if(!ValidateMotions(scene,motions))
        throw std::invalid_argument("dangling or invalid motion binding");
    return motions;
}
// Atomic single-slot envelope; a complete REVSCENE v1 payload plus versioned
// motion records, not two independently saved sidecars that may tear.
inline std::string EncodeVrBundle(const std::string& sceneBytes,
                                  const std::string& motionBytes) {
    if(sceneBytes.size()>55000u||motionBytes.size()>8192u)
        throw std::invalid_argument("project data budget exceeded");
    const std::string out="RVPROJECT\t1\nscene\t"+std::to_string(sceneBytes.size())+
        "\n"+sceneBytes+"motions\t"+std::to_string(motionBytes.size())+"\n"+motionBytes;
    if(out.size()>kVrBundleMaxBytes)throw std::invalid_argument("project save too large");
    return out;
}
inline std::string EncodeVrBundle(const std::string& sceneBytes,
                                  const std::string& motionBytes,
                                  const std::string& lightBytes) {
    if(sceneBytes.size()>54000u||motionBytes.size()>8192u||lightBytes.size()>512u)
        throw std::invalid_argument("project payload exceeds budget");
    const std::string out="RVPROJECT\t2\nscene\t"+
        std::to_string(sceneBytes.size())+"\n"+sceneBytes+
        "motions\t"+std::to_string(motionBytes.size())+"\n"+motionBytes+
        "lighting\t"+std::to_string(lightBytes.size())+"\n"+lightBytes;
    if(out.size()>kVrBundleMaxBytes)throw std::invalid_argument("project save too large");
    return out;
}
struct VrDecodedBundle {
    Scene scene;
    std::vector<VrMotion> motions;
    VrLight light;
    bool legacy=false;
};
inline size_t ReadVrBundleLength(const std::string& text,size_t& cursor,
                                const std::string& prefix,size_t max){
    if(text.compare(cursor,prefix.size(),prefix)!=0)
        throw std::invalid_argument("invalid bundle section");
    cursor+=prefix.size();
    const size_t start=cursor;
    size_t count=0;
    while(cursor<text.size()&&text[cursor]>='0'&&text[cursor]<='9'){
        const size_t digit=static_cast<size_t>(text[cursor]-'0');
        if(count>(max-digit)/10)throw std::invalid_argument("bundle length over budget");
        count=count*10+digit;++cursor;
    }
    if(cursor==start||cursor>=text.size()||text[cursor]!='\n')
        throw std::invalid_argument("noncanonical bundle length");
    ++cursor;
    return count;
}
inline VrDecodedBundle DecodeVrBundle(const std::string& bytes){
    if(bytes.empty()||bytes.size()>kVrBundleMaxBytes)
        throw std::invalid_argument("invalid project size");
    VrDecodedBundle result;
    if(bytes.compare(0,11,"REVSCENE\t1\n")==0){
        result.scene=ParseScene(bytes);result.legacy=true;return result;
    }
    const bool v1=bytes.compare(0,12,"RVPROJECT\t1\n")==0;
    const bool v2=bytes.compare(0,12,"RVPROJECT\t2\n")==0;
    if(!v1&&!v2)throw std::invalid_argument("unsupported project schema");
    size_t cursor=12;
    const size_t sceneSize=ReadVrBundleLength(bytes,cursor,"scene\t",55000u);
    if(sceneSize>bytes.size()-cursor)throw std::invalid_argument("truncated scene");
    result.scene=ParseScene(bytes.substr(cursor,sceneSize));
    cursor+=sceneSize;
    const size_t motionSize=ReadVrBundleLength(bytes,cursor,"motions\t",8192u);
    if(motionSize>bytes.size()-cursor)throw std::invalid_argument("truncated motion");
    result.motions=ParseMotions(result.scene,bytes.substr(cursor,motionSize));
    cursor+=motionSize;
    if(v2){
        const size_t lightSize=ReadVrBundleLength(bytes,cursor,"lighting\t",512u);
        if(lightSize!=bytes.size()-cursor)
            throw std::invalid_argument("truncated light payload");
        result.light=ParseVrLight(result.scene,bytes.substr(cursor,lightSize));
    }else if(cursor!=bytes.size()){
        throw std::invalid_argument("trailing bytes in project");
    }
    return result;
}

}} // reverie::authoring
#endif
