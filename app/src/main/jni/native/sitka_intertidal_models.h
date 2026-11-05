#ifndef REVERIE_SITKA_INTERTIDAL_MODELS_H
#define REVERIE_SITKA_INTERTIDAL_MODELS_H
// Small, source-ecology-based shoreline specimens. These describe ordinary
// Southeast Alaska intertidal ecology; NO real specimen coordinates are
// asserted. Sites come only from separate reviewed geography authoring.
// All "heritage" objects are possible findings, inspect/document-only.
// Single small fixed-geometry triangle meshes; no distant glowing icons.
#include "sitka_terrain_mesh.h"
#include "sitka_fishing.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace reverie { namespace sitka {
enum class ShoreSpecimen : uint8_t { EmptyClamShell, CrabMolt, BeachPebble, SeaGlass, DriftwoodChip, PurpleShoreCrab, GreenShoreCrab, HermitCrab, SeaStar, Limpet, Chiton, BarnaclePatch, MusselCluster, LittleneckClam, SeaAnemone, Nudibranch, SeaCucumber, SeaUrchin, SeaSnail, KelpFloat, WorkedStone, NetWeight, MiddenTrace, CarvedWood, SeaStarVariation, Count };
enum class ShoreAction : uint8_t { Collect, Observe, Document };
enum class ShoreShape : uint8_t {
    Crab, Star, Cone, Chiton, Barnacles, Mussels, Bivalve,
    Anemone, Slug, Urchin, Stone, Crystal, Wood, Kelp, Weight, Midden
};
enum class ShoreZone : uint8_t { Rocky=1u,Pool=2u,Gravel=4u,Wrack=8u,Cultural=16u };
struct ShoreDefinition {
    const char* token;
    const char* name;
    BeachFind category;
    ShoreAction action;
    ShoreShape shape;
    unsigned zones;
    float radius_m;
    std::array<uint8_t,3> color;
    unsigned rarity; // 1 common, 2 uncommon, 3 rare, 4 exceptional
    const char* reference;
};
inline constexpr float kShoreRevealRadiusM=1.524f; // exactly five feet
inline constexpr std::array<ShoreDefinition,25> kShoreSpecimens={{
    {"empty_clam_shell","Empty clam shell",BeachFind::Shell,
     ShoreAction::Collect,ShoreShape::Bivalve,12u,
     0.065f,{{202,188,160}},1u,"nps_sitka"},
    {"crab_molt","Discarded crab molt",BeachFind::Crab,
     ShoreAction::Collect,ShoreShape::Crab,9u,
     0.092f,{{174,116,89}},2u,"adfg_shore_crabs"},
    {"beach_pebble","Interesting beach stone",BeachFind::Rock,
     ShoreAction::Collect,ShoreShape::Stone,12u,
     0.052f,{{86,100,99}},1u,"nps_sitka"},
    {"sea_glass","Sea glass shard",BeachFind::SeaGlass,
     ShoreAction::Collect,ShoreShape::Crystal,12u,
     0.035f,{{59,164,145}},2u,"ambient_game"},
    {"driftwood_chip","Driftwood fragment",BeachFind::Driftwood,
     ShoreAction::Collect,ShoreShape::Wood,12u,
     0.077f,{{133,103,78}},1u,"ambient_game"},
    {"purple_shore_crab","Purple shore crab",BeachFind::Crab,
     ShoreAction::Observe,ShoreShape::Crab,3u,
     0.047f,{{92,67,108}},1u,"adfg_shore_crabs"},
    {"green_shore_crab","Green shore crab",BeachFind::Crab,
     ShoreAction::Observe,ShoreShape::Crab,3u,
     0.041f,{{72,104,73}},1u,"adfg_shore_crabs"},
    {"hermit_crab","Hermit crab",BeachFind::Crab,
     ShoreAction::Observe,ShoreShape::Crab,3u,
     0.060f,{{158,133,104}},2u,"adfg_tidepool"},
    {"small_sea_star","Small sea star",BeachFind::MarineLife,
     ShoreAction::Observe,ShoreShape::Star,3u,
     0.095f,{{166,94,73}},1u,"nps_sitka"},
    {"limpet","Limpet",BeachFind::MarineLife,
     ShoreAction::Observe,ShoreShape::Cone,1u,
     0.035f,{{114,108,89}},1u,"nps_sitka"},
    {"chiton","Chiton",BeachFind::MarineLife,
     ShoreAction::Observe,ShoreShape::Chiton,3u,
     0.074f,{{115,87,74}},1u,"nps_sitka"},
    {"barnacle_patch","Barnacle patch",BeachFind::MarineLife,
     ShoreAction::Observe,ShoreShape::Barnacles,1u,
     0.055f,{{179,179,159}},1u,"nps_sitka"},
    {"mussel_cluster","Mussel cluster",BeachFind::MarineLife,
     ShoreAction::Observe,ShoreShape::Mussels,3u,
     0.063f,{{48,62,93}},1u,"adfg_tidepool"},
    {"littleneck_clam","Littleneck-style clam",BeachFind::MarineLife,
     ShoreAction::Observe,ShoreShape::Bivalve,6u,
     0.065f,{{176,163,131}},2u,"nps_sitka"},
    {"sea_anemone","Tidepool anemone",BeachFind::MarineLife,
     ShoreAction::Observe,ShoreShape::Anemone,3u,
     0.057f,{{141,79,112}},2u,"adfg_tidepool"},
    {"nudibranch","Nudibranch",BeachFind::MarineLife,
     ShoreAction::Observe,ShoreShape::Slug,2u,
     0.061f,{{195,119,67}},3u,"adfg_tidepool"},
    {"sea_cucumber","Sea cucumber",BeachFind::MarineLife,
     ShoreAction::Observe,ShoreShape::Slug,2u,
     0.084f,{{117,79,63}},3u,"adfg_tidepool"},
    {"sea_urchin","Sea urchin",BeachFind::MarineLife,
     ShoreAction::Observe,ShoreShape::Urchin,3u,
     0.073f,{{116,64,97}},2u,"adfg_tidepool"},
    {"sea_snail","Sea snail",BeachFind::MarineLife,
     ShoreAction::Observe,ShoreShape::Cone,3u,
     0.033f,{{128,115,92}},1u,"nps_sitka"},
    {"kelp_float","Washed kelp float",BeachFind::Driftwood,
     ShoreAction::Collect,ShoreShape::Kelp,8u,
     0.073f,{{95,108,45}},1u,"adfg_tidepool"},
    {"possible_worked_stone","Possible worked-stone fragment",BeachFind::Heritage,
     ShoreAction::Document,ShoreShape::Stone,16u,
     0.053f,{{101,97,89}},4u,"artifact_guidance"},
    {"possible_net_weight","Possible historic fishing weight",BeachFind::Heritage,
     ShoreAction::Document,ShoreShape::Weight,16u,
     0.062f,{{119,112,94}},4u,"artifact_guidance"},
    {"possible_shell_midden","Possible shell midden trace",BeachFind::Heritage,
     ShoreAction::Document,ShoreShape::Midden,16u,
     0.100f,{{146,137,108}},4u,"artifact_guidance"},
    {"possible_carved_wood","Possible carved wooden fragment",BeachFind::Heritage,
     ShoreAction::Document,ShoreShape::Wood,16u,
     0.076f,{{119,83,58}},4u,"artifact_guidance"},
    {"small_mottled_sea_star","Mottled sea star",BeachFind::MarineLife,
     ShoreAction::Observe,ShoreShape::Star,3u,
     0.080f,{{144,99,122}},2u,"nps_sitka"}
}};
static_assert(kShoreSpecimens.size()==unsigned(ShoreSpecimen::Count),
              "Every Sitka shore find needs a physical model brief");
inline const ShoreDefinition& Specimen(ShoreSpecimen kind){
    return kShoreSpecimens[unsigned(kind)];
}
inline bool ShoreSpecimenByToken(std::string_view token,ShoreSpecimen& out){
    for(size_t i=0;i<kShoreSpecimens.size();++i)
        if(token==kShoreSpecimens[i].token){
            out=static_cast<ShoreSpecimen>(i);return true;
        }
    return false;
}
inline bool ShoreZoneByToken(std::string_view name,unsigned& out){
    if(name=="rocky")out=unsigned(ShoreZone::Rocky);
    else if(name=="tidepool")out=unsigned(ShoreZone::Pool);
    else if(name=="gravel")out=unsigned(ShoreZone::Gravel);
    else if(name=="wrack")out=unsigned(ShoreZone::Wrack);
    else if(name=="protected_cultural")out=unsigned(ShoreZone::Cultural);
    else return false;
    return true;
}
inline std::array<uint8_t,3> ShoreTint(
    std::array<uint8_t,3> color,float scale){
    for(auto& channel:color)
        channel=static_cast<uint8_t>(std::clamp(float(channel)*scale,0.f,255.f));
    return color;
}
inline void ShoreFacet(TerrainMesh& mesh,
    const std::array<float,3>& a,const std::array<float,3>& b,
    const std::array<float,3>& c,const std::array<uint8_t,3>& color){
    const uint32_t first=static_cast<uint32_t>(mesh.vertices.size());
    for(const auto& p:std::array<std::array<float,3>,3>{{a,b,c}})
        mesh.vertices.push_back({p[0],p[1],p[2],color[0],color[1],color[2]});
    mesh.indices.insert(mesh.indices.end(),{first,first+1u,first+2u});
}
inline void ShoreDome(TerrainMesh& mesh,float x,float y,float z,
    float rx,float rz,float h,std::array<uint8_t,3> color,unsigned sides=7u){
    constexpr float tau=6.28318530718f;
    const std::array<float,3> apex={{x,y+h,z}};
    const std::array<float,3> bottom={{x,y-.005f,z}};
    for(unsigned i=0;i<sides;++i){
        const float a=tau*float(i)/float(sides);
        const float b=tau*float(i+1u)/float(sides);
        const std::array<float,3> pa={{x+rx*std::cos(a),y,z+rz*std::sin(a)}};
        const std::array<float,3> pb={{x+rx*std::cos(b),y,z+rz*std::sin(b)}};
        ShoreFacet(mesh,pa,pb,apex,ShoreTint(color,i%2u?.86f:1.f));
        ShoreFacet(mesh,pb,pa,bottom,ShoreTint(color,.66f));
    }
}
inline void ShoreBlade(TerrainMesh& mesh,
    const std::array<float,3>& a,const std::array<float,3>& b,
    const std::array<float,3>& c,std::array<uint8_t,3> color){
    ShoreFacet(mesh,a,b,c,color);
    ShoreFacet(mesh,b,a,c,ShoreTint(color,.82f));
}
inline void ShoreStar(TerrainMesh& mesh,float x,float y,float z,
    float r,std::array<uint8_t,3> color){
    constexpr float tau=6.28318530718f;
    const std::array<float,3> center={{x,y+.018f,z}};
    for(unsigned i=0;i<10u;++i){
        const float a=tau*float(i)/10.f,b=tau*float(i+1u)/10.f;
        const float ra=(i%2u==0u)?r:r*.42f;
        const float rb=((i+1u)%2u==0u)?r:r*.42f;
        ShoreFacet(mesh,
            {{x+ra*std::cos(a),y,z+ra*std::sin(a)}},
            {{x+rb*std::cos(b),y,z+rb*std::sin(b)}},
            center,ShoreTint(color,i%2u?.82f:1.f));
    }
}
inline void ShoreCrab(TerrainMesh& mesh,float x,float y,float z,
    float r,std::array<uint8_t,3> color,bool is_molt){
    ShoreDome(mesh,x,y,z,r*.65f,r*.48f,r*.19f,color,7u);
    // Deliberate little angular legs rather than large cartoon claws.
    for(int side=-1;side<=1;side+=2){
        for(unsigned i=0;i<4u;++i){
            const float zoff=(float(i)-1.5f)*r*.23f;
            const float sx=x+side*r*.38f;
            ShoreBlade(mesh,{{sx,y+.012f,z+zoff}},
                       {{x+side*r*.82f,y+.024f,z+zoff+.007f}},
                       {{x+side*r*(.9f+float(i%2u)*.1f),y,z+zoff+r*.13f}},
                       ShoreTint(color,is_molt?.62f:.8f));
        }
        ShoreBlade(mesh,{{x+side*r*.43f,y+.015f,z+r*.30f}},
                   {{x+side*r*.9f,y+.025f,z+r*.47f}},
                   {{x+side*r*1.12f,y,z+r*.64f}},color);
    }
}
inline void ShoreChiton(TerrainMesh& mesh,float x,float y,float z,
    float r,std::array<uint8_t,3> color){
    for(unsigned i=0;i<8u;++i){
        const float start=z-r+float(i)*2.f*r/8.f;
        const float finish=start+2.f*r/8.f;
        const float width=r*(.65f+.15f*std::sin(float(i)*.6f));
        ShoreFacet(mesh,{{x-width,y,start}},{{x+width,y,start}},
            {{x,y+.015f,finish}},ShoreTint(color,i%2u?.75f:1.f));
    }
}
inline void ShoreUrchin(TerrainMesh& mesh,float x,float y,float z,
    float r,std::array<uint8_t,3> color){
    ShoreDome(mesh,x,y,z,r*.58f,r*.58f,r*.52f,color,8u);
    constexpr float tau=6.28318530718f;
    for(unsigned i=0;i<8u;++i){
        const float a=tau*float(i)/8.f;
        ShoreBlade(mesh,{{x+r*.4f*std::cos(a),y+.04f,z+r*.4f*std::sin(a)}},
            {{x+r*std::cos(a),y+.07f,z+r*std::sin(a)}},
            {{x+r*.65f*std::cos(a+.25f),y,z+r*.65f*std::sin(a+.25f)}},
            ShoreTint(color,.83f));
    }
}
inline void ShoreWeight(TerrainMesh& mesh,float x,float y,float z,
    float r,std::array<uint8_t,3> color){
    constexpr float tau=6.28318530718f;
    for(unsigned i=0;i<8u;++i){
        const float a=tau*i/8.f,b=tau*(i+1u)/8.f;
        ShoreFacet(mesh,
            {{x+r*.38f*std::cos(a),y+.008f,z+r*.38f*std::sin(a)}},
            {{x+r*std::cos(a),y+.012f,z+r*std::sin(a)}},
            {{x+r*std::cos(b),y+.012f,z+r*std::sin(b)}},color);
        ShoreFacet(mesh,
            {{x+r*.38f*std::cos(a),y+.008f,z+r*.38f*std::sin(a)}},
            {{x+r*std::cos(b),y+.012f,z+r*std::sin(b)}},
            {{x+r*.38f*std::cos(b),y+.008f,z+r*.38f*std::sin(b)}},
            ShoreTint(color,.72f));
    }
}
inline void ShoreWood(TerrainMesh& mesh,float x,float y,float z,float r,
    std::array<uint8_t,3> color,bool carved){
    for(int side=-1;side<=1;side+=2){
        ShoreFacet(mesh,{{x-r*.25f,y,z-r}},{{x+r*.25f,y,z-r}},
            {{x+side*r*.25f,y+.035f,z+r}},color);
    }
    if(carved){
        for(unsigned i=0;i<3u;++i){
            const float zz=z-r*.6f+float(i)*r*.5f;
            ShoreBlade(mesh,{{x-r*.18f,y+.036f,zz}},
                {{x+r*.13f,y+.037f,zz+r*.19f}},
                {{x+r*.02f,y+.039f,zz+r*.08f}},
                ShoreTint(color,1.25f));
        }
    }
}
inline void AppendShoreSpecimen(TerrainMesh& mesh,ShoreSpecimen specimen,
    float x,float ground_y,float z,bool undiscovered_glimmer){
    const auto& def=Specimen(specimen);
    const float y=ground_y+.018f,r=def.radius_m;
    const auto color=def.color;
    switch(def.shape){
        case ShoreShape::Crab:
            ShoreCrab(mesh,x,y,z,r,color,specimen==ShoreSpecimen::CrabMolt);
            break;
        case ShoreShape::Star:ShoreStar(mesh,x,y,z,r,color);break;
        case ShoreShape::Cone:
            ShoreDome(mesh,x,y,z,r,r*.86f,r*.64f,color,8u);break;
        case ShoreShape::Chiton:ShoreChiton(mesh,x,y,z,r,color);break;
        case ShoreShape::Barnacles:
            for(int i=0;i<3;++i)
                ShoreDome(mesh,x+(i-1)*r*.65f,y,z+((i%2)?r*.3f:-r*.2f),
                    r*.35f,r*.35f,r*.5f,color,5u);
            break;
        case ShoreShape::Mussels:
            for(int i=0;i<3;++i)
                ShoreDome(mesh,x+(i-1)*r*.6f,y,z+(i%2?r*.3f:0.f),
                    r*.4f,r*.65f,r*.33f,color,6u);
            break;
        case ShoreShape::Bivalve:
            ShoreDome(mesh,x-r*.13f,y,z,r*.7f,r*.44f,r*.2f,color,8u);
            break;
        case ShoreShape::Anemone:
            ShoreDome(mesh,x,y,z,r*.58f,r*.58f,r*.34f,color,7u);
            ShoreUrchin(mesh,x,y,z,r*.55f,color);break;
        case ShoreShape::Slug:
            ShoreDome(mesh,x,y,z,r*.52f,r,r*.25f,color,7u);
            ShoreBlade(mesh,{{x-r*.18f,y+.018f,z+r*.6f}},
                {{x,y+r*.45f,z+r*.9f}},
                {{x+r*.18f,y+.018f,z+r*.6f}},color);
            break;
        case ShoreShape::Urchin:ShoreUrchin(mesh,x,y,z,r,color);break;
        case ShoreShape::Stone:
            ShoreDome(mesh,x,y,z,r*.8f,r*.65f,r*.43f,color,7u);break;
        case ShoreShape::Crystal:
            ShoreDome(mesh,x,y,z,r*.7f,r*.9f,r*.43f,color,5u);break;
        case ShoreShape::Wood:
            ShoreWood(mesh,x,y,z,r,color,specimen==ShoreSpecimen::CarvedWood);
            break;
        case ShoreShape::Kelp:
            ShoreDome(mesh,x,y,z,r*.43f,r*.43f,r*.38f,color,6u);
            ShoreBlade(mesh,{{x,y,z}},{{x+r*1.6f,y+.03f,z+r*.3f}},
                {{x+r*.7f,y+.01f,z+r*.5f}},color);
            break;
        case ShoreShape::Weight:ShoreWeight(mesh,x,y,z,r,color);break;
        case ShoreShape::Midden:
            for(int i=0;i<4;++i)
                ShoreDome(mesh,x+(i%2?r*.4f:-r*.4f),y,
                    z+(i/2?r*.4f:-r*.4f),r*.38f,r*.3f,r*.10f,
                    ShoreTint(color,i%2u?.72f:1.f),6u);
            break;
    }
    if(undiscovered_glimmer){
        // A tiny 3D two-tone PIXEL-like shine, not a floating beacon.
        // Kept within the 5-foot streamed mesh, without per-eye mutation.
        const float gy=y+std::min(.045f,r*.50f);
        const auto soft=ShoreTint(color,1.32f);
        const auto light=std::array<uint8_t,3>{{226,213,171}};
        ShoreFacet(mesh,{{x-.009f,gy,z}},{{x+.009f,gy,z}},
                        {{x,gy+.014f,z}},soft);
        ShoreFacet(mesh,{{x,gy,z-.009f}},{{x,gy,z+.009f}},
                        {{x,gy+.014f,z}},light);
    }
}
}} // namespace reverie::sitka
#endif
