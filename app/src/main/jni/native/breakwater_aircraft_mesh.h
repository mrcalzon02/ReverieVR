#ifndef REVERIE_BREAKWATER_AIRCRAFT_MESH_H
#define REVERIE_BREAKWATER_AIRCRAFT_MESH_H
#include "breakwater_campaign.h"
#include <algorithm>
#include <cmath>

namespace reverie { namespace breakwater {
// Low-poly stereoscopic geometry: no extra GL draw, PNG, dynamic allocation,
// physics body, RNG, new save field, or render-eye-specific animation state.
// The existing authoritative Campaign pose and bank control every vertex.
template<typename TriFn>
void EmitAircraftMesh(const Enemy &e,TriFn triangle) {
    if(!e.active || e.type!=UnitType::Aircraft ||
       (e.aircraft_pattern!=AircraftPattern::Strafer &&
        e.aircraft_pattern!=AircraftPattern::Bomber) ||
       !std::isfinite(e.x) || !std::isfinite(e.z) ||
       !std::isfinite(e.bank))return;
    const bool bomber=e.aircraft_pattern==AircraftPattern::Bomber;
    const float bank=std::max(-.32f,std::min(.32f,e.bank));
    const float sine=std::sin(bank),cosine=std::cos(bank);
    const float span=bomber?1.25f:.96f;
    const float root=bomber?.21f:.15f;
    const float tail=bomber?-.99f:-.79f;
    const auto face=[&](float ax,float ay,float az,
                        float bx,float by,float bz,
                        float cx,float cy,float cz,
                        float r,float g,float b) {
        triangle(e.x+ax*cosine-ay*sine,1.6f+ax*sine+ay*cosine,e.z+az,
                 e.x+bx*cosine-by*sine,1.6f+bx*sine+by*cosine,e.z+bz,
                 e.x+cx*cosine-cy*sine,1.6f+cx*sine+cy*cosine,e.z+cz,
                 r,g,b);
    };
    const auto quad=[&](float ax,float ay,float az,
                        float bx,float by,float bz,
                        float cx,float cy,float cz,
                        float dx,float dy,float dz,
                        float r,float g,float b) {
        face(ax,ay,az,bx,by,bz,cx,cy,cz,r,g,b);
        face(ax,ay,az,cx,cy,cz,dx,dy,dz,r,g,b);
    };
    // Closed-looking faceted fuselage, proper nose cone and tapered rear.
    const float nose=bomber?1.03f:.89f;
    const float olive=bomber?.42f:.33f;
    for(int side:{-1,1}) {
        const float sign=static_cast<float>(side);
        face(0,0,nose,sign*root,.05f,.42f,0,.16f,.42f,
             olive+.10f,olive+.08f,olive+.03f);
        face(0,0,nose,0,-.13f,.42f,sign*root,.05f,.42f,
             olive*.75f,olive*.79f,olive*.70f);
        quad(sign*root,.05f,.42f,sign*root,-.13f,.42f,
             sign*root,-.13f,tail,sign*root,.05f,tail,
             olive*.77f,olive*.80f,olive*.76f);
        face(sign*root,.05f,tail,sign*root,-.13f,tail,
             0,0,tail-.17f,olive*.68f,olive*.69f,olive*.65f);
        // Swept wings: actual thin 3D slabs with dark undersides and tips.
        quad(sign*root,.02f,.19f,sign*span,-.03f,-.15f,
             sign*span,-.03f,-.41f,sign*root,.02f,-.35f,
             olive+.11f,olive+.08f,olive+.045f);
        quad(sign*root,-.09f,.19f,sign*span,-.10f,-.15f,
             sign*span,-.10f,-.41f,sign*root,-.09f,-.35f,
             olive*.62f,olive*.65f,olive*.63f);
        quad(sign*span,-.03f,-.15f,sign*span,-.10f,-.15f,
             sign*span,-.10f,-.41f,sign*span,-.03f,-.41f,
             .29f,.32f,.30f);
        // Pale field-identification stripe on each wing.
        quad(sign*(span*.68f),.027f,-.20f,
             sign*(span*.81f),-.026f,-.24f,
             sign*(span*.81f),-.026f,-.30f,
             sign*(span*.68f),.027f,-.29f,.69f,.65f,.45f);
        quad(sign*root,.05f,tail+.19f,sign*(span*.43f),.05f,tail+.05f,
             sign*(span*.43f),.05f,tail-.10f,
             sign*root,.05f,tail-.09f,
             olive+.11f,olive+.08f,olive*.88f);
    }
    quad(-root,-.13f,.42f,root,-.13f,.42f,
         root,-.13f,tail,-root,-.13f,tail,
         .19f,.21f,.19f);
    quad(-root,.05f,.42f,root,.05f,.42f,
         root,.05f,tail,-root,.05f,tail,
         olive+.04f,olive+.04f,olive*.91f);
    // Vertical fin and framed, visibly raised cockpit.
    face(0,.06f,tail+.20f,0,.45f,tail-.02f,
         0,.06f,tail-.15f,.46f,.49f,.43f);
    face(-root*.8f,.07f,.25f,root*.8f,.07f,.25f,
         0,.25f,-.08f,.23f,.32f,.38f);
    face(-root*.8f,.07f,.25f,0,.25f,-.08f,
         -root*.8f,.05f,-.12f,.14f,.20f,.26f);
    face(root*.8f,.07f,.25f,root*.8f,.05f,-.12f,
         0,.25f,-.08f,.16f,.22f,.28f);
    // One nose motor for the strafer, two underslung wing engines on the
    // bomber; they have faceted three-dimensional cowls and static propellers.
    const auto motor=[&](float x) {
        const float w=bomber?.11f:.13f;
        quad(x-w,-.005f,.35f,x+w,-.005f,.35f,
             x+w,-.005f,-.18f,x-w,-.005f,-.18f,
             .49f,.49f,.46f);
        quad(x-w,-.005f,.35f,x-w,-.14f,.35f,
             x-w,-.14f,-.18f,x-w,-.005f,-.18f,
             .25f,.27f,.27f);
        quad(x+w,-.005f,.35f,x+w,-.14f,.35f,
             x+w,-.14f,-.18f,x+w,-.005f,-.18f,
             .30f,.32f,.30f);
        face(x-w,-.005f,.35f,x+w,-.005f,.35f,
             x,-.15f,.35f,.63f,.63f,.59f);
        face(x-.14f,-.015f,.39f,x+.14f,-.015f,.39f,
             x,.015f,.392f,.24f,.26f,.28f);
        face(x,-.16f,.39f,x,.14f,.39f,
             x+.018f,-.015f,.392f,.26f,.27f,.29f);
    };
    if(bomber) {
        motor(-.54f);motor(.54f);
        // Two-door bomb bay and raised second observation blister.
        quad(-.12f,-.14f,-.15f,.12f,-.14f,-.15f,
             .12f,-.14f,-.52f,-.12f,-.14f,-.52f,
             .15f,.19f,.20f);
        face(-.09f,.055f,-.40f,.09f,.055f,-.40f,
             0,.17f,-.52f,.24f,.32f,.37f);
    } else {
        motor(0);
        for(int side:{-1,1}) {
            const float x=static_cast<float>(side)*.30f;
            quad(x-.026f,-.03f,.35f,x+.026f,-.03f,.35f,
                 x+.026f,-.03f,.68f,x-.026f,-.03f,.68f,
                 .23f,.25f,.26f);
        }
    }
    // Independent wingtip navigation surfaces, twin tail stabilizers and
    // real under-wing stores. These sit on the same banked local frame as
    // the actual cockpit, wings, weapon line and hit pose. Both aircraft
    // are volumetric from spawn to departure (never sprite billboards).
    for(int side:{-1,1}) {
        const float sign=static_cast<float>(side);
        quad(sign*(span*.79f),-.04f,-.17f,
             sign*(span*.93f),-.04f,-.17f,
             sign*(span*.93f),-.04f,-.23f,
             sign*(span*.79f),-.04f,-.23f,
             bomber?.68f:.74f,bomber?.66f:.70f,.45f);
        if(bomber) {
            // Two faceted bombs remain visibly distinct from nacelles.
            const float px=sign*.85f;
            quad(px-.055f,-.155f,-.17f,px+.055f,-.155f,-.17f,
                 px+.035f,-.155f,-.45f,px-.035f,-.155f,-.45f,
                 .22f,.24f,.21f);
            face(px-.04f,-.15f,-.45f,px+.04f,-.15f,-.45f,
                 px,-.155f,-.56f,.33f,.34f,.29f);
        }else{
            // Underwing machine-gun fairing and rearward exhaust streak.
            const float px=sign*.37f;
            quad(px-.033f,-.11f,.18f,px+.033f,-.11f,.18f,
                 px+.023f,-.13f,.54f,px-.023f,-.13f,.54f,
                 .18f,.21f,.21f);
        }
    }
}
}} // namespace reverie::breakwater
#endif
