#ifndef REVERIE_BREAKWATER_UNIT_GEOMETRY_H
#define REVERIE_BREAKWATER_UNIT_GEOMETRY_H
// Deterministic low-poly *world* geometry, not camera-facing sprites.
// Pure recipes are shared by both stereo eyes; no clock, mutable RNG, heap,
// new draw pass, target or collision object. Dimensions are authored to the
// existing campaign's world units and do not modify hit volumes.
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace reverie { namespace breakwater { namespace unit_geometry {
struct Color { float r,g,b; };
struct Section { float z,halfWidth; };
enum class HullKind : uint8_t { Rowboat, LandingCraft, Gunboat, HeavyTransport };

// A proper hard-chine hull: five tapered stations, sloped outward plating,
// paired chines, open interior and an upper gunwale rather than a Lego box.
// The bow is always +z, matching the existing beaching and targeting system.
template<class TriangleFn>
int ContouredHull(float x,float z,HullKind kind,TriangleFn triangle){
    if(!std::isfinite(x)||!std::isfinite(z))return 0;
    const bool small=kind==HullKind::Rowboat;
    const bool heavy=kind==HullKind::HeavyTransport;
    const float halfWidth=small?.34f:heavy?.91f:kind==HullKind::Gunboat?.59f:.55f;
    const float halfLength=small?.54f:heavy?1.20f:kind==HullKind::Gunboat?.64f:.56f;
    const Color paint=small?Color{.52f,.39f,.24f}:
        heavy?Color{.27f,.35f,.30f}:
        kind==HullKind::Gunboat?Color{.29f,.36f,.40f}:Color{.38f,.44f,.38f};
    const Section stations[5]={{-halfLength,halfWidth*.65f},
        {-halfLength*.48f,halfWidth*.98f},{0,halfWidth},
        {halfLength*.52f,halfWidth*.92f},{halfLength,halfWidth*.12f}};
    const float gunwale=small?-.70f:-.62f;
    const float chine=small?-.91f:-.88f;
    const float keel=-1.01f;
    int triangles=0;
    const auto face=[&](float ax,float ay,float az,
                        float bx,float by,float bz,
                        float cx,float cy,float cz,Color c){
        triangle(x+ax,ay,z+az,x+bx,by,z+bz,x+cx,cy,z+cz,
                 c.r,c.g,c.b);
        ++triangles;
    };
    for(int side : {-1,1}){
        const float s=static_cast<float>(side);
        for(int n=0;n<4;++n){
            const Section a=stations[n],b=stations[n+1];
            // Side plating is an actual bevel, not an upright cuboid wall.
            const float alo=s*a.halfWidth,blo=s*b.halfWidth;
            const float ac=s*a.halfWidth*.77f,bc=s*b.halfWidth*.77f;
            Color sideShade={paint.r*(side>0?.91f:.81f),
                             paint.g*(side>0?.92f:.82f),
                             paint.b*(side>0?.96f:.86f)};
            face(alo,gunwale,a.z,blo,gunwale,b.z,bc,chine,b.z,sideShade);
            face(alo,gunwale,a.z,bc,chine,b.z,ac,chine,a.z,sideShade);
            const Color under={paint.r*.55f,paint.g*.58f,paint.b*.61f};
            face(ac,chine,a.z,bc,chine,b.z,0,keel,b.z,under);
            face(ac,chine,a.z,0,keel,b.z,0,keel,a.z,under);
            // Thin inboard lip visibly contains an open well.
            const Color lip={paint.r*1.16f,paint.g*1.12f,paint.b*1.09f};
            face(alo,gunwale,a.z,alo-s*.045f,gunwale-.045f,a.z,
                 blo-s*.045f,gunwale-.045f,b.z,lip);
            face(alo,gunwale,a.z,blo-s*.045f,gunwale-.045f,b.z,
                 blo,gunwale,b.z,lip);
        }
    }
    // Flat fore/aft transoms leave a dark visible interior to the wells.
    for(int end : {0,4}){
        const Section st=stations[end];
        const Color dark={paint.r*.37f,paint.g*.41f,paint.b*.38f};
        face(-st.halfWidth,gunwale,st.z,st.halfWidth,gunwale,st.z,
             0,keel,st.z,dark);
    }
    return triangles; // 50, always stereo-independent
}

template<class BoxFn,class TriangleFn>
void RowboatOars(float x,float z,BoxFn box,TriangleFn triangle){
    if(!std::isfinite(x)||!std::isfinite(z))return;
    for(int side:{-1,1}){
        const float s=static_cast<float>(side);
        const float ox=x+s*.29f;
        box(ox,-.64f,z+.05f,.07f,.07f,.05f,.20f,.21f,.19f);
        triangle(ox,-.61f,z+.08f,x+s*.76f,-.67f,z-.26f,
                 x+s*.78f,-.70f,z-.19f,.59f,.41f,.23f);
        triangle(x+s*.76f,-.67f,z-.26f,x+s*1.02f,-.82f,z-.40f,
                 x+s*.89f,-.80f,z-.36f,.63f,.45f,.25f);
    }
}

template<class BoxFn,class TriangleFn>
int RowboatFurniture(float x,float z,unsigned passengers,BoxFn box,TriangleFn triangle){
    if(!std::isfinite(x)||!std::isfinite(z))return 0;
    // Dark inner hull, transverse ribs, seats, iron rowlocks and paired oars.
    box(x,-.80f,z,.49f,.035f,.72f,.12f,.12f,.11f);
    for(float dz:{-.20f,.15f})
        box(x,-.67f,z+dz,.55f,.046f,.075f,.66f,.48f,.28f);
    RowboatOars(x,z,box,triangle);
    if(passengers>0u){
        box(x,-.59f,z+.08f,.18f,.20f,.18f,.31f,.36f,.29f);
        box(x,-.42f,z+.08f,.15f,.15f,.15f,.60f,.53f,.39f);
        box(x,-.345f,z+.085f,.19f,.055f,.18f,.26f,.31f,.27f);
    }
    return 4; // Explicit triangles, excluding reuse of shared boxes
}

template<class BoxFn,class TriangleFn>
int LandingDetails(float x,float z,float ramp,unsigned troops,bool armor,
                   BoxFn box,TriangleFn triangle){
    if(!std::isfinite(x)||!std::isfinite(z)||!std::isfinite(ramp))return 0;
    ramp=std::clamp(ramp,0.f,1.f);
    // Long armored sides, angled bulwarks, aft engine cover, gun mounts.
    for(int side:{-1,1}){
        const float s=static_cast<float>(side);
        box(x+s*.48f,-.54f,z-.09f,.12f,.24f,.83f,
            .37f,.42f,.36f);
        box(x+s*.46f,-.41f,z-.25f,.11f,.07f,.24f,
            .19f,.24f,.21f);
        for(float step:{-.34f,.12f}){
            // Upright reinforcement gussets break up otherwise flat sides.
            triangle(x+s*.53f,-.47f,z+step,
                     x+s*.53f,-.65f,z+step+.12f,
                     x+s*.57f,-.47f,z+step+.11f,
                     .45f,.47f,.40f);
        }
    }
    box(x,-.67f,z-.39f,.83f,.17f,.12f,.23f,.27f,.24f);
    box(x,-.49f,z-.39f,.35f,.21f,.22f,.38f,.43f,.37f);
    // A single articulated two-triangle ramp; its tip follows arrival only.
    const float y=-.60f-.47f*ramp;
    const float tip=z+.57f+.18f*ramp;
    triangle(x-.44f,-.60f,z+.42f,x+.44f,-.60f,z+.42f,
             x+.41f,y,tip,.57f,.59f,.49f);
    triangle(x-.44f,-.60f,z+.42f,x+.41f,y,tip,
             x-.41f,y,tip,.57f,.59f,.49f);
    if(armor){
        box(x,-.48f,z-.03f,.50f,.24f,.43f,.30f,.35f,.28f);
        box(x,-.31f,z-.10f,.22f,.11f,.22f,.24f,.31f,.25f);
    }
    // Passengers are decorative aboard; real targetable soldiers appear only
    // on the beach when Campaign::Land() allocates their independent slots.
    const unsigned count=std::min(6u,troops);
    for(unsigned n=0;n<count;++n){
        const float dx=(static_cast<float>(n%3u)-1.f)*.25f;
        const float dz=(n<3u?-.20f:.16f);
        box(x+dx,-.52f,z+dz,.13f,.24f,.13f,.28f,.33f,.29f);
        box(x+dx,-.37f,z+dz,.13f,.13f,.13f,.63f,.57f,.42f);
        box(x+dx,-.30f,z+dz,.16f,.045f,.16f,.25f,.28f,.24f);
    }
    return 6; // side gussets and ramp triangles
}

template<class BoxFn,class TriangleFn>
void ShoreArmor(float x,float z,bool tank,BoxFn box,TriangleFn triangle){
    if(!std::isfinite(x)||!std::isfinite(z))return;
    // Separate unmistakable silhouettes with no billboard, separate target
    // ID or hitbox: tank has continuous tracks and a full cannon turret;
    // APC has wheels, passenger cabin, side ports and roof machine gun.
    const float bodyR=tank?.32f:.31f,bodyG=tank?.36f:.38f;
    const float bodyB=tank?.27f:.32f;
    for(int side:{-1,1}){
        const float s=float(side);
        if(tank){
            box(x+s*.30f,-.80f,z,.15f,.24f,.73f,
                .13f,.15f,.13f);
            for(float dz:{-.25f,0.f,.25f})
                box(x+s*.405f,-.81f,z+dz,.035f,.13f,.13f,
                    .32f,.34f,.28f);
        }else{
            for(float dz:{-.25f,.23f}){
                box(x+s*.315f,-.80f,z+dz,.15f,.22f,.18f,
                    .12f,.14f,.13f);
                box(x+s*.400f,-.80f,z+dz,.018f,.105f,.11f,
                    .42f,.44f,.38f);
            }
        }
        triangle(x+s*.30f,-.65f,z+.30f,x+s*.21f,-.55f,z+.34f,
                 x+s*.28f,-.68f,z+.39f,.44f,.45f,.35f);
    }
    box(x,-.70f,z,.62f,.22f,.72f,bodyR,bodyG,bodyB);
    box(x,-.56f,z-.07f,.46f,tank?.16f:.21f,.46f,
        .42f,.45f,.34f);
    // Distinct, angular glacis and the rear engine / troop doors.
    triangle(x-.30f,-.68f,z+.34f,x+.30f,-.68f,z+.34f,
             x,-.50f,z+.20f,.44f,.46f,.35f);
    if(tank){
        box(x,-.45f,z-.07f,.37f,.17f,.31f,.30f,.33f,.29f);
        box(x,-.345f,z-.08f,.26f,.055f,.22f,.25f,.28f,.23f);
        box(x,-.42f,z+.30f,.085f,.085f,.56f,.21f,.25f,.21f);
        box(x,-.42f,z+.56f,.11f,.10f,.08f,.12f,.14f,.13f);
        box(x,-.30f,z-.14f,.10f,.055f,.10f,.40f,.39f,.31f);
    }else{
        box(x,-.43f,z-.02f,.44f,.08f,.38f,.42f,.45f,.36f);
        box(x,-.37f,z-.13f,.22f,.06f,.24f,.25f,.29f,.25f);
        box(x,-.36f,z+.24f,.045f,.06f,.35f,.21f,.23f,.20f);
        for(int side:{-1,1})for(float dz:{-.20f,.14f})
            box(x+float(side)*.316f,-.58f,z+dz,.011f,.07f,.10f,
                .11f,.15f,.14f);
        box(x,-.63f,z-.365f,.34f,.09f,.018f,.18f,.20f,.17f);
    }
}

template<class BoxFn,class TriangleFn>
void SoldierEquipment(float x,float z,bool support,BoxFn box,
                      TriangleFn triangle){
    if(!std::isfinite(x)||!std::isfinite(z))return;
    // Helmet brim, ear tabs, pack, webbing, forearms and real slung rifle.
    box(x,-.365f,z,.195f,.065f,.18f,.24f,.28f,.23f);
    box(x,-.62f,z-.085f,.17f,.23f,.08f,.27f,.30f,.24f);
    box(x,-.70f,z+.078f,.17f,.05f,.025f,.44f,.43f,.30f);
    for(int side:{-1,1}){
        const float s=float(side);
        box(x+s*.14f,-.59f,z+.01f,.07f,.22f,.078f,.34f,.37f,.28f);
        box(x+s*.12f,-.68f,z+.08f,.10f,.06f,.055f,.55f,.48f,.35f);
    }
    const float barrel=support?.32f:.23f;
    box(x,-.65f,z+.16f,.10f,.06f,barrel,.19f,.21f,.20f);
    box(x,-.64f,z+.16f+barrel*.5f,.025f,.03f,.13f,
        .13f,.15f,.14f);
    // Shoulder sling is geometry, not a billboard glyph.
    triangle(x-.11f,-.60f,z+.09f,x+.11f,-.71f,z+.11f,
             x+.09f,-.73f,z+.08f,.29f,.26f,.18f);
}

}}} // namespace reverie::breakwater::unit_geometry
#endif
