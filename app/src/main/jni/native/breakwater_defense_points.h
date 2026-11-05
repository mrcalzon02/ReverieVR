#ifndef REVERIE_BREAKWATER_DEFENSE_POINTS_H
#define REVERIE_BREAKWATER_DEFENSE_POINTS_H
#include "breakwater_campaign.h"
#include "breakwater_materials.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace reverie { namespace breakwater { namespace defense {
// One real central gun berth and two elevated flanking fighting positions.
// Each coast has its own flank geometry; all use one persistent 3D world.
// Positions are local to the single authored terrain, NEVER a separate map.
constexpr int kPositions=3;
constexpr float kDoubleClickWindow=.44f;
struct Position {
    float x,y,z;
    const char* label;
};
constexpr Position kCoasts[4][kPositions] = {
    { {0.f,0.f,0.f,"CENTRAL BUNKER"},
      {-5.10f,.28f,-.08f,"DUNE LEFT"},
      {5.25f,.38f,.12f,"DUNE RIGHT"} },
    { {0.f,0.f,0.f,"QUAY BATTERY"},
      {-8.30f,.17f,.10f,"WEST PIER"},
      {8.30f,.17f,.10f,"EAST PIER"} },
    { {0.f,0.f,0.f,"BLACKCAP KEEP"},
      {-5.30f,.54f,.24f,"WEST BASALT"},
      {5.12f,.77f,.07f,"EAST RIDGE"} },
    { {0.f,0.f,0.f,"STORMWATCH"},
      {-6.30f,.45f,.20f,"LOW CAISSON"},
      {6.25f,.62f,-.08f,"HIGH CAISSON"} }
};
static_assert(sizeof(kCoasts)/sizeof(kCoasts[0])==Campaign::kLocations,
              "Every actual Breakwater coast needs three defense positions");
inline Position At(uint8_t location,int index) {
    if(location>=Campaign::kLocations || index<0 || index>=kPositions)
        return {0,0,0,nullptr};
    return kCoasts[location][index];
}
inline bool Valid(Position point) {return point.label!=nullptr;}
inline void ShiftOrigin(float* out,const float* original,Position site) {
    for(int i=0;i<3;++i)out[i]=original[i]+
        (i==0?site.x:i==1?site.y:site.z);
}
// Shift the virtual camera, not the entire simulation's 3D coastline.
// Column-major post-multiply of the existing VR view by T(-position).
// Both stereo eye views inherit the same station and preserve head tracking.
inline void ShiftEyeView(const float *view,Position station,float *result) {
    if(!view || !result)return;
    for(int i=0;i<16;++i)result[i]=view[i];
    for(int row=0;row<4;++row)
        result[12+row]=view[12+row]-view[row]*station.x-
            view[4+row]*station.y-view[8+row]*station.z;
}
inline void Marker(Position site,float* center) {
    center[0]=site.x;center[1]=site.y-.53f;
    // Return beacon sits on the seaward lip of the main fortification.
    // From either raised flank it is visible AHEAD of the side bunker wall;
    // unlike a rearward beacon it cannot hide *behind* opaque revetments.
    // It is never drawn while the central gun is the active station.
    center[2]=site.z+(site.x==0.f?-2.05f:-.86f);
}
// Check a genuine finite controller ray against real 3D stationary pads.
// It must be a forward ray hit; never choose an off-screen or rear pad.
inline int Hit(uint8_t location,int current,
               const float *origin,const float *direction) {
    if(location>=Campaign::kLocations || !origin || !direction)return -1;
    for(int i=0;i<3;++i)
        if(!std::isfinite(origin[i]) || !std::isfinite(direction[i]))
            return -1;
    const float dd=direction[0]*direction[0]+
                   direction[1]*direction[1]+direction[2]*direction[2];
    if(!std::isfinite(dd) || dd<.000001f)return -1;
    float nearest=40.f;int target=-1;
    for(int index=0;index<kPositions;++index) {
        if(index==current)continue;
        const Position site=At(location,index);
        float m[3];Marker(site,m);
        const float dx=origin[0]-m[0],dy=origin[1]-m[1],dz=origin[2]-m[2];
        const float b=(dx*direction[0]+dy*direction[1]+
                       dz*direction[2])/dd;
        const float c=(dx*dx+dy*dy+dz*dz-.60f*.60f)/dd;
        const float discriminant=b*b-c;
        if(discriminant<0.f || !std::isfinite(discriminant))continue;
        float t=-b-std::sqrt(discriminant);
        if(t<=.05f)t=-b+std::sqrt(discriminant);
        if(t>.05f && t<nearest){nearest=t;target=index;}
    }
    return target;
}
enum class Tap : uint8_t { Ignored, Armed, Transferred };
class Selection {
public:
    int current=0,hover=-1;
    int armed=-1;
    float remaining=0.f;
    void Tick(float seconds) {
        if(!std::isfinite(seconds) || seconds<=0.f)return;
        remaining=std::max(0.f,remaining-seconds);
        if(remaining<=0.f)armed=-1;
    }
    Tap Press(int site) {
        if(site<0 || site>=kPositions || site==current)
            return Tap::Ignored;
        if(armed==site && remaining>0.f) {
            current=site;armed=-1;remaining=0.f;hover=-1;
            return Tap::Transferred;
        }
        armed=site;remaining=kDoubleClickWindow;
        return Tap::Armed;
    }
    void LostFocus() {hover=-1;armed=-1;remaining=0.f;}
};
// Source-owned geometry for each location, using the same material atlas and
// existing XYZRGB/textured batches. Platforms stay off the battle center.
template<class BoxFn,class TriangleFn,class CladFn>
void EmitPlatforms(uint8_t location,int selected,int hovered,
                   BoxFn box,TriangleFn triangle,CladFn clad) {
    if(location>=Campaign::kLocations)return;
    for(int index=0;index<kPositions;++index) {
        const Position p=At(location,index);
        const float deckY=p.y-1.04f;
        const bool active=index==selected,focus=index==hovered;
        const materials::Surface deck=location==0?materials::Surface::DockPlanks:
            location==1?materials::Surface::DiamondPlate:
            location==2?materials::Surface::Basalt:
                        materials::Surface::ReinforcedConcrete;
        if(index!=0) {
            const float wide=location==1?2.20f:2.04f;
            // Physical floor, two posts, low rail and side-facing braces.
            box(p.x,deckY,p.z,wide,.26f,1.88f,.29f,.34f,.34f);
            clad(deck,p.x,deckY,p.z,wide,.26f,1.88f);
            for(int s:{-1,1}) {
                const float x=p.x+static_cast<float>(s)*(wide*.46f);
                box(x,p.y-.68f,p.z+.48f,.12f,.65f,.13f,
                    .28f,.32f,.32f);
                clad(materials::Surface::Steel,x,p.y-.68f,p.z+.48f,
                     .12f,.65f,.13f);
            }
            box(p.x,p.y-.43f,p.z+.48f,wide,.08f,.12f,
                .30f,.36f,.37f);
            clad(materials::Surface::Steel,p.x,p.y-.43f,p.z+.48f,
                 wide,.08f,.12f);
        }
        if(active)continue; // No lit navigation target across our own gun.
        float marker[3];Marker(p,marker);
        // Upright diamond/facing-plane target is easy to identify with an
        // actual hand/controller ray. It is not a floating 2D world card.
        const float red=focus?1.f:active?.24f:.31f;
        const float green=focus?.89f:active?1.f:.78f;
        const float blue=focus?.31f:active?.46f:.77f;
        const float x=marker[0],y=marker[1],z=marker[2];
        triangle(x-.30f,y,z,x,y+.33f,z,x+.30f,y,z,
                 red,green,blue);
        triangle(x-.30f,y,z,x+.30f,y,z,x,y-.33f,z,
                 red*.68f,green*.76f,blue*.79f);
        // Physical 3D mount/neck makes the marker intelligible from the
        // flank. No new dynamic light or particle system is necessary.
        box(x,y-.42f,z,.09f,.41f,.10f,
            focus?.78f:.36f,focus?.73f:.43f,.32f);
        box(x,y+.02f,z,.31f,.05f,.10f,red*.65f,green*.65f,blue*.65f);
    }
}
}}} // reverie::breakwater::defense
#endif
