#ifndef REVERIE_SITKA_HARBOR_SHOP_H
#define REVERIE_SITKA_HARBOR_SHOP_H
// Source-grounded Sitka harbor upgrade storefront. Only the central
// Harbor Department office at Thomsen Harbor is independently confirmed;
// other authorized harbors may offer *in-game service desks* at real
// verified waterfront features, never fabricated real-world offices.
// One wallet and the existing V4 boat/rod/skill save transaction.
#include "sitka_helm.h"
#include "sitka_free_roam.h"
#include "sitka_beach_discoveries.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace reverie { namespace sitka {
inline constexpr size_t kMaxHarborShopBytes=4096u;
inline constexpr std::array<const char*,5> kMunicipalHarbors={{
    "crescent","sealing_cove","anb","thomsen","eliason"
}};
inline constexpr std::array<const char*,4> kHullLabels={{
    "Dinghy","Fishing skiff","Pilothouse","Commercial trawler"
}};
inline constexpr size_t kShopItems=1u+16u+5u+5u;
enum class HarborLocationKind : uint8_t { RealOffice, ServiceDesk };
struct HarborShopSite {
    std::string harbor;
    std::string feature_id;
    HarborLocationKind kind=HarborLocationKind::ServiceDesk;
    float x=0.f,z=0.f;
};
inline bool KnownMunicipalHarbor(const std::string& name){
    for(const char* harbor:kMunicipalHarbors)if(name==harbor)return true;
    return false;
}
inline std::vector<HarborShopSite> ParseHarborShops(
    const std::string& source,double origin_east,double origin_north){
    if(source.size()>kMaxHarborShopBytes)
        throw std::invalid_argument("Sitka harbor shop source oversized");
    std::istringstream in(source);
    std::string row;
    if(!std::getline(in,row)||row!="SITKA-HARBOR-SHOPS-1")
        throw std::invalid_argument("Sitka harbor shop version");
    bool provenance=false,origin=false;
    std::vector<HarborShopSite> sites;
    std::set<std::string> seen;
    while(std::getline(in,row)){
        if(!row.empty()&&row.back()=='\r')row.pop_back();
        if(row.empty()||row[0]=='#')continue;
        const auto fields=BeachFields(row);
        if(fields[0]=="P"){
            if(provenance||fields.size()!=5u||
               !BeachId(fields[1])||fields[2].size()!=64u||
               fields[3]!="EPSG32608"||fields[4]!="reviewed")
                throw std::invalid_argument("Sitka harbor shop provenance");
            for(char c:fields[2])
                if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))
                    throw std::invalid_argument("Sitka harbor source SHA256");
            provenance=true;
        }else if(fields[0]=="O"){
            if(origin||fields.size()!=3u||
               std::abs(BeachNumber(fields[1],-1.e8,1.e8)-origin_east)>.001||
               std::abs(BeachNumber(fields[2],-1.e8,1.e8)-origin_north)>.001)
                throw std::invalid_argument("Sitka harbor origin mismatch");
            origin=true;
        }else if(fields[0]=="H"){
            if(fields.size()!=6u||sites.size()>=5u||
               !KnownMunicipalHarbor(fields[1])||
               !BeachId(fields[3])||!seen.insert(fields[1]).second)
                throw std::invalid_argument("Sitka harbor desk site or ID");
            HarborLocationKind kind;
            if(fields[2]=="office"){
                if(fields[1]!="thomsen")
                    throw std::invalid_argument("Only Thomsen real office verified");
                kind=HarborLocationKind::RealOffice;
            }else if(fields[2]=="game_desk")
                kind=HarborLocationKind::ServiceDesk;
            else throw std::invalid_argument("Unsupported Harbor Department desk");
            sites.push_back({fields[1],fields[3],kind,
                static_cast<float>(BeachNumber(fields[4],-100000.,100000.)),
                static_cast<float>(BeachNumber(fields[5],-100000.,100000.))});
        }else throw std::invalid_argument("Unsupported Sitka harbor record");
    }
    if(!provenance||!origin||sites.empty())
        throw std::invalid_argument("No reviewed Sitka harbor interaction sites");
    return sites;
}
inline bool VerifiedHarborSite(const HarborShopSite& shop,
    const FreeRoamSession& roam,
    const std::vector<ShoreFeature>& authored){
    if(!KnownMunicipalHarbor(shop.harbor)||
       !std::isfinite(shop.x)||!std::isfinite(shop.z))return false;
    if(!roam.GroundAt(shop.x,shop.z).valid)return false;
    // Author-sourced feature ID is mandatory. Never instantiate a new
    // fictitious office simply because the player knows the harbor name.
    for(const auto& f:authored){
        if(f.id!=shop.feature_id)continue;
        const bool building=f.material=="harbor_shack"||
            f.material=="storefront"||f.material=="warehouse"||
            f.material=="heritage"||f.material=="cannery";
        const bool waterfront=f.material=="pier"||f.material=="pontoon";
        if(!(building||(shop.kind==HarborLocationKind::ServiceDesk&&waterfront)))
            return false;
        double mid_x=f.x,mid_z=f.z;
        double radius=std::max(3.,double(f.width+f.depth));
        if(f.polygon&& !f.footprint.empty()){
            mid_x=mid_z=0.;
            for(const auto& pt:f.footprint){mid_x+=pt.x;mid_z+=pt.z;}
            mid_x/=double(f.footprint.size());
            mid_z/=double(f.footprint.size());
            radius=0.;
            for(const auto& pt:f.footprint)
                radius=std::max(radius,std::hypot(double(pt.x)-mid_x,
                                                 double(pt.z)-mid_z));
            radius+=3.;
        }
        return std::hypot(double(shop.x)-mid_x,double(shop.z)-mid_z)<=radius;
    }
    return false;
}
inline int HarborShopTarget(const std::vector<HarborShopSite>& shops,
    const FreeRoamSession& roam,const float origin[3],
    const float direction[3]){
    if(!origin||!direction)return -1;
    for(unsigned axis=0;axis<3u;++axis)
        if(!std::isfinite(origin[axis])||!std::isfinite(direction[axis]))
            return -1;
    const double length=std::sqrt(double(direction[0])*direction[0]+
        double(direction[1])*direction[1]+double(direction[2])*direction[2]);
    if(length<.8||length>1.2)return -1;
    const auto player=roam.pose();
    double nearest=3.2;
    int chosen=-1;
    for(size_t i=0;i<shops.size();++i){
        const auto& shop=shops[i];
        if(std::hypot(double(shop.x)-player.x,
                      double(shop.z)-player.z)>2.6)continue;
        const auto ground=roam.GroundAt(shop.x,shop.z);
        if(!ground.valid||std::abs(ground.y-player.ground_y)>1.5f)continue;
        const double dx=double(shop.x)-origin[0];
        const double dy=double(ground.y)+1.25-origin[1];
        const double dz=double(shop.z)-origin[2];
        const double along=dx*direction[0]+dy*direction[1]+dz*direction[2];
        if(along<.15||along>nearest)continue;
        const double offset=dx*dx+dy*dy+dz*dz-along*along;
        if(offset>.16)continue; // actual aimed interaction, not proximity auto-shop
        chosen=int(i);nearest=along;
    }
    return chosen;
}
// Tiny 3D game signage at already-authored physical facilities.
// Distinct from false extra real-world office buildings.
inline TerrainMesh HarborShopSigns(const std::vector<HarborShopSite>& shops,
                                  const FreeRoamSession& roam){
    TerrainMesh m;
    for(const auto& site:shops){
        const auto hit=roam.GroundAt(site.x,site.z);
        if(!hit.valid)continue;
        const float x=site.x,z=site.z,y=hit.y;
        const uint32_t base=uint32_t(m.vertices.size());
        const std::array<std::array<float,3>,8> p={{
            {{x-.05f,y,z-.05f}},{{x+.05f,y,z-.05f}},
            {{x+.05f,y+1.45f,z-.05f}},{{x-.05f,y+1.45f,z-.05f}},
            {{x-.36f,y+1.11f,z-.055f}},{{x+.36f,y+1.11f,z-.055f}},
            {{x+.36f,y+1.47f,z-.055f}},{{x-.36f,y+1.47f,z-.055f}}
        }};
        for(size_t i=0;i<p.size();++i){
            const auto& pt=p[i];
            m.vertices.push_back({pt[0],pt[1],pt[2],
                static_cast<uint8_t>(i<4u?80:39),
                static_cast<uint8_t>(i<4u?100:135),
                static_cast<uint8_t>(i<4u?101:141)});
        }
        m.indices.insert(m.indices.end(),{
            base+0u,base+1u,base+2u,base+0u,base+2u,base+3u,
            base+4u,base+5u,base+6u,base+4u,base+6u,base+7u
        });
    }
    return m;
}
inline TerrainMesh HarborShopPanelMesh(){
    // A small reusable dark world-space clipboard, not another real office.
    TerrainMesh m;
    m.vertices={
        {-.78f,-.50f,-.024f,17,32,38},{.78f,-.50f,-.024f,17,32,38},
        {.78f,.50f,-.024f,17,32,38},{-.78f,.50f,-.024f,17,32,38}};
    m.indices={0,1,2,0,2,3};
    return m;
}
enum class ShopEvent : uint8_t {
    None,Open,Close,Next,Previous,Purchased,Denied
};
inline uint32_t HullPrice(unsigned hull){
    constexpr uint32_t costs[]={300u,1400u,4200u};
    return hull<3u?costs[hull]:0u;
}
inline unsigned RequiredShopBeach(unsigned hull){
    return hull==0u?1u:hull==1u?4u:9u;
}
inline unsigned RequiredShopCity(unsigned hull){
    return hull<2u?0u:hull==2u?2u:4u;
}
inline bool HarborShopUnlocked(const Journal& journal,unsigned tier){
    return BeachSiteCount(journal.beach_sites)>=RequiredShopBeach(tier) &&
           BeachSiteCount(journal.city_sites)>=RequiredShopCity(tier);
}
struct ShopOffer{
    std::string title;
    uint32_t price=0u;
    bool available=false,owned=false,unlocked=false,affordable=false;
};
inline ShopOffer HarborOffer(const Journal& journal,unsigned index){
    ShopOffer o;
    const auto progress=HelmFromJournal(journal);
    if(index==0u){
        o.title=journal.hull_tier<3u?
            std::string("Hull: ")+kHullLabels[journal.hull_tier+1u]:
            "Hull: maximum";
        o.available=journal.hull_tier<3u;
        o.owned=!o.available;
        o.price=HullPrice(progress.hull);
        o.unlocked=o.available&&HarborShopUnlocked(journal,progress.hull);
    }else if(index<=kParts.size()){
        const unsigned id=index-1u;
        const auto& part=kParts[id];
        o.title=std::string("Fitting: ")+part.model;
        o.price=part.price;
        o.owned=progress.Has(HelmItem(id));
        o.available=!o.owned;
        o.unlocked=progress.hull>=part.hull &&
            (progress.owned&part.requires_bits)==part.requires_bits &&
            HarborShopUnlocked(journal,part.hull);
    }else if(index<=kParts.size()+kEquipmentMax.size()){
        const unsigned id=index-1u-unsigned(kParts.size());
        const auto offer=NextEquipment(journal,Equipment(id));
        o.title=std::string("Gear: ")+kEquipmentNames[id];
        o.price=offer.credits;
        o.available=offer.available;
        o.owned=!o.available;
        o.unlocked=offer.unlocked;
    }else if(index<kShopItems){
        const unsigned id=index-1u-unsigned(kParts.size())-
                          unsigned(kEquipmentMax.size());
        const auto offer=NextCharacter(journal,CharacterSkill(id));
        o.title=std::string("Training: ")+kCharacterSkillNames[id];
        o.price=offer.credits;
        o.available=offer.available;
        o.owned=!o.available;
        o.unlocked=offer.unlocked;
    }else return o;
    o.affordable=o.available&&journal.credits>=o.price;
    return o;
}
inline std::string HarborOfferStatus(const ShopOffer& offer){
    if(offer.owned)return "MAX / OWNED";
    if(!offer.unlocked)return "LOCKED: EXPLORE";
    if(!offer.affordable)return "NEED CREDITS";
    return std::to_string(offer.price)+" CR - SELECT";
}
class HarborShopSession{
public:
    bool open()const{return open_;}
    unsigned selected()const{return selected_;}
    int active_site()const{return site_;}
    bool consumes_primary()const{return open_||consumed_primary_;}
    ShopEvent Update(bool primary,bool secondary,float navigation,
        int aimed_site,FishingSession& fishing,bool underway){
        consumed_primary_=false;
        const bool press=primary&&!primary_held_;
        const bool back=secondary&&!secondary_held_;
        primary_held_=primary;secondary_held_=secondary;
        if(!std::isfinite(navigation))navigation=0.f;
        if(std::abs(navigation)<.35f)navigation_held_=false;
        if(underway){
            if(open_){open_=false;site_=-1;return ShopEvent::Close;}
            return ShopEvent::None;
        }
        if(!open_){
            if(press&&aimed_site>=0&&
               fishing.fishing.phase==FishingPhase::Ready){
                open_=true;site_=aimed_site;selected_=0u;
                consumed_primary_=true;return ShopEvent::Open;
            }
            return ShopEvent::None;
        }
        consumed_primary_=primary;
        if(back){open_=false;site_=-1;consumed_primary_=true;
                 return ShopEvent::Close;}
        if(!navigation_held_&&std::abs(navigation)>.65f){
            navigation_held_=true;
            if(navigation<0.f){
                selected_=(selected_+1u)%unsigned(kShopItems);
                return ShopEvent::Next;
            }
            selected_=(selected_+unsigned(kShopItems)-1u)%
                      unsigned(kShopItems);
            return ShopEvent::Previous;
        }
        if(!press)return ShopEvent::None;
        consumed_primary_=true;
        const auto offer=HarborOffer(fishing.fishing.journal,selected_);
        if(!offer.available||!offer.unlocked||!offer.affordable)
            return ShopEvent::Denied;
        bool purchased=false;
        if(selected_==0u)purchased=PurchaseHull(fishing);
        else if(selected_<=kParts.size())
            purchased=PurchaseHelm(fishing,HelmItem(selected_-1u));
        else if(selected_<=kParts.size()+kEquipmentMax.size())
            purchased=fishing.Purchase(Equipment(selected_-1u-
                                               unsigned(kParts.size())));
        else purchased=fishing.PurchaseCharacter(CharacterSkill(
            selected_-1u-unsigned(kParts.size())-
            unsigned(kEquipmentMax.size())));
        return purchased?ShopEvent::Purchased:ShopEvent::Denied;
    }
private:
    bool open_=false,primary_held_=false,secondary_held_=false;
    bool navigation_held_=false,consumed_primary_=false;
    int site_=-1;unsigned selected_=0u;
};
inline std::array<std::string,6> HarborShopPanel(
    const HarborShopSession& shop,const FishingSession& fishing){
    std::array<std::string,6> lines{};
    const Journal& j=fishing.fishing.journal;
    lines[0]="HARBOR DEPARTMENT / BOAT UPGRADES";
    lines[1]="CREDITS "+std::to_string(j.credits)+
             "  HULL "+std::string(kHullLabels[std::min(3u,j.hull_tier)]);
    lines[2]="ITEM "+std::to_string(shop.selected()+1u)+"/"+
             std::to_string(kShopItems);
    const ShopOffer offer=HarborOffer(j,shop.selected());
    lines[3]=offer.title;
    lines[4]=HarborOfferStatus(offer);
    lines[5]="STICK: BROWSE  SELECT: BUY  BACK: CLOSE";
    return lines;
}
}} // namespace reverie::sitka
#endif
