#ifndef REVERIE_SITKA_FISHING_H
#define REVERIE_SITKA_FISHING_H
// Game-private, CPU-only Sitka fishing economy. This is game currency,
// never a statement about Alaska harvest rules or actual seafood prices.
// Exactly one Step() per simulation tick, never once per stereo eye.
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace reverie { namespace sitka {
enum class FishingPhase : uint8_t { Ready, Waiting, Bite, Reeling, Landed };
enum class Fish : uint8_t { DollyVarden, Coho, PinkSalmon, Flounder, Count };
inline constexpr std::array<const char*,4> kFishNames={{
    "Dolly Varden","Coho salmon","Pink salmon","Flounder"
}};
inline constexpr std::array<uint32_t,4> kFishCreditAwards={{45u,100u,65u,35u}};

// First five bits are existing save IDs; append without renumbering.
enum class BeachFind : uint8_t {
    Shell, Crab, Rock, SeaGlass, Driftwood, MarineLife, Heritage, Count
};
enum class CityFind : uint8_t {Harbor,Heritage,PublicArt,Trailhead,Civic,Count};
constexpr uint32_t CityFindMask(CityFind kind){return 1u<<unsigned(kind);}
enum class CharacterSkill : uint8_t {Stride,RodHandling,Endurance,Observation,Deckhand,Count};
inline constexpr std::array<const char*,5> kCharacterSkillNames={{
    "Walking pace","Rod handling","Angler endurance",
    "Explorer awareness","Deckwork"
}};
inline constexpr std::array<uint32_t,5> kCharacterSkillMax={{8u,8u,8u,8u,8u}};
inline constexpr std::array<uint32_t,5> kCharacterSkillBaseCosts={{40u,50u,50u,45u,65u}};
inline constexpr std::array<const char*,7> kBeachFindNames={{
    "Shell","Crab sighting","Interesting rock","Sea glass","Driftwood",
    "Marine life observation","Cultural trace recorded"
}};
constexpr uint32_t FindMask(BeachFind f){return 1u<<unsigned(f);}

enum class Equipment : uint8_t { Rod, Reel, Line, Lures, ShoreKit, Count };
inline constexpr std::array<const char*,5> kEquipmentNames={{
    "Fishing rod","Reel","Fishing line","Lure assortment","Beach discovery kit"
}};
inline constexpr std::array<uint32_t,5> kEquipmentMax={{12u,10u,10u,8u,8u}};
inline constexpr std::array<uint32_t,5> kEquipmentBasePrice={{35u,28u,23u,21u,26u}};
inline constexpr std::array<const char*,13> kRodNames={{
    "Starter pole","Bamboo travel rod","Fiberglass caster","Dock spinning rod",
    "Coastal spinning rod","Carbon composite","Light saltwater",
    "Surf casting rod","Reinforced surf rod","Coastal graphite",
    "Precision long cast","Heavy offshore","Master angler rod"
}};

struct Journal {
    uint32_t seed=0x51A7CAu;
    uint32_t credits=0;
    uint32_t gear=0; // fishing rod levels, old V1/V2 field retained
    std::array<uint32_t,4> caught={{0,0,0,0}};
    uint32_t total=0;
    uint32_t helm_upgrades=0;
    uint32_t hull_tier=0;
    uint32_t reel_level=0,line_level=0,lure_level=0,shore_kit_level=0;
    uint32_t discoveries=0; // seven stable groups, no discovery credits
    uint64_t beach_sites=0; // 64 immutable, author-assigned site IDs
    uint32_t stride_level=0,rod_handling_level=0,endurance_level=0;
    uint32_t observation_level=0,deckhand_level=0;
    uint64_t city_sites=0; // author-assigned, public, sourced landmarks
    uint32_t city_discoveries=0; // categories encountered, no credits
    std::array<uint32_t,4> fish_inventory={{0,0,0,0}};
    uint32_t shellfish_inventory=0;
    uint64_t submitted_beach_sites=0;
    uint32_t starter_tokens=0,submitted_starter_tokens=0;
    uint32_t submitted_discoveries=0;
};
inline uint32_t BeachSiteCount(uint64_t bits){
    uint32_t count=0;
    while(bits){bits&=(bits-1u);++count;}
    return count;
}
// Starter supplies are granted only for a genuinely absent save, never migration.
// No invented geographic discovery ID is reserved for the supplied token.
inline Journal StarterJournal(){
    Journal j;
    j.fish_inventory[unsigned(Fish::Coho)]=1u;
    j.shellfish_inventory=1u;
    j.starter_tokens=1u;
    return j;
}
inline uint32_t RegisteredFindCount(const Journal& j){
    return BeachSiteCount(j.submitted_beach_sites)+j.submitted_starter_tokens;
}
// The Tier 0 pole, reel/spool, line, bait and shore kit cannot wear out.
// Basic bait is always available; Cast never consumes bait or starter gear.
inline constexpr bool kStarterGearInfiniteDurability=true;
inline constexpr bool kBasicBaitInfinite=true;
inline uint32_t EquipmentLevel(const Journal& j,Equipment item){
    switch(item){
        case Equipment::Rod:return j.gear;
        case Equipment::Reel:return j.reel_level;
        case Equipment::Line:return j.line_level;
        case Equipment::Lures:return j.lure_level;
        case Equipment::ShoreKit:return j.shore_kit_level;
        default:return 0u;
    }
}
inline uint32_t FindPrerequisites(uint32_t next){
    if(next<3u)return 0u;
    if(next<5u)return FindMask(BeachFind::Shell);
    if(next<7u)return FindMask(BeachFind::Shell)|FindMask(BeachFind::Crab);
    if(next<9u)return FindMask(BeachFind::Shell)|FindMask(BeachFind::Crab)|
                        FindMask(BeachFind::Rock);
    if(next<11u)return FindMask(BeachFind::Shell)|FindMask(BeachFind::Crab)|
                         FindMask(BeachFind::Rock)|FindMask(BeachFind::SeaGlass);
    // Respectful wildlife/cultural documentation is never mandatory for a rod.
    return (1u<<(unsigned(BeachFind::Driftwood)+1u))-1u;
}
struct EquipmentOffer {
    bool available=false,unlocked=false,affordable=false;
    uint32_t next_level=0,credits=0,required_sites=0,required_kinds=0;
};
inline EquipmentOffer NextEquipment(const Journal& j,Equipment item){
    EquipmentOffer result;
    const unsigned index=unsigned(item);
    if(index>=kEquipmentMax.size()||EquipmentLevel(j,item)>=kEquipmentMax[index])
        return result;
    const uint32_t next=EquipmentLevel(j,item)+1u;
    result.available=true;
    result.next_level=next;
    result.credits=kEquipmentBasePrice[index]*next*(next+1u)/2u;
    result.required_sites=next; // unique finds along authentic shoreline
    result.required_kinds=FindPrerequisites(next);
    result.unlocked=RegisteredFindCount(j)>=result.required_sites &&
        (j.submitted_discoveries&result.required_kinds)==result.required_kinds;
    result.affordable=j.credits>=result.credits;
    return result;
}

inline uint32_t CharacterLevel(const Journal& j,CharacterSkill skill){
    switch(skill){
        case CharacterSkill::Stride:return j.stride_level;
        case CharacterSkill::RodHandling:return j.rod_handling_level;
        case CharacterSkill::Endurance:return j.endurance_level;
        case CharacterSkill::Observation:return j.observation_level;
        case CharacterSkill::Deckhand:return j.deckhand_level;
        default:return 0u;
    }
}
inline float CharacterWalkScale(const Journal& j){
    return 1.f+.05f*float(j.stride_level);
}
struct CharacterOffer{
    bool available=false,unlocked=false,affordable=false;
    uint32_t next_level=0,credits=0,required_beach=0,required_city=0;
    uint32_t required_beach_kinds=0;
};
inline CharacterOffer NextCharacter(const Journal& j,CharacterSkill skill){
    CharacterOffer o;
    const unsigned id=unsigned(skill);
    if(id>=kCharacterSkillMax.size()||
       CharacterLevel(j,skill)>=kCharacterSkillMax[id])return o;
    o.available=true;
    o.next_level=CharacterLevel(j,skill)+1u;
    o.credits=kCharacterSkillBaseCosts[id]*
              o.next_level*(o.next_level+1u)/2u;
    o.required_beach=o.next_level;
    // Return to the city for advanced training, not every character level.
    o.required_city=o.next_level>2u?o.next_level-2u:0u;
    o.required_beach_kinds=FindPrerequisites(o.next_level);
    o.unlocked=RegisteredFindCount(j)>=o.required_beach &&
       BeachSiteCount(j.city_sites)>=o.required_city &&
       (j.submitted_discoveries&o.required_beach_kinds)==o.required_beach_kinds;
    o.affordable=j.credits>=o.credits;
    return o;
}

struct Fishing {
    Journal journal;
    FishingPhase phase=FishingPhase::Ready;
    Fish hooked=Fish::DollyVarden;
    float time=0.f,reel=0.f,tension=0.f,depth=0.f;
    uint32_t Next(){
        uint32_t v=journal.seed?journal.seed:0x51A7CAu;
        v^=v<<13u;v^=v>>17u;v^=v<<5u;
        journal.seed=v;return v;
    }
    bool Cast(bool confirmed_water,float depth_metres){
        if(phase!=FishingPhase::Ready||!confirmed_water||
           !std::isfinite(depth_metres)||depth_metres<.3f||
           depth_metres>200.f)return false;
        depth=depth_metres;
        time=std::max(.7f,2.0f+float(Next()%250u)*.012f-
                           .10f*float(journal.lure_level));
        reel=tension=0.f;phase=FishingPhase::Waiting;return true;
    }
    void Step(float delta,bool primary,bool secondary){
        if(!std::isfinite(delta)||delta<=0.f)return;
        const float dt=std::min(delta,.1f);
        if(secondary&&phase!=FishingPhase::Ready){
            phase=FishingPhase::Ready;time=reel=tension=0.f;return;
        }
        switch(phase){
            case FishingPhase::Waiting:
                time-=dt;
                if(time<=0.f){
                    const uint32_t pick=Next()%100u;
                    hooked=depth<3.f?
                      (pick<40u?Fish::Flounder:pick<70u?Fish::DollyVarden:
                       pick<85u?Fish::PinkSalmon:Fish::Coho):
                      (pick<38u?Fish::Coho:pick<67u?Fish::PinkSalmon:
                       pick<90u?Fish::DollyVarden:Fish::Flounder);
                    phase=FishingPhase::Bite;
                    time=8.f+.25f*float(journal.rod_handling_level);
                }
                break;
            case FishingPhase::Bite:
                if(primary){phase=FishingPhase::Reeling;time=0.f;}
                else if((time-=dt)<=0.f)phase=FishingPhase::Ready;
                break;
            case FishingPhase::Reeling:
                if(primary){
                    tension=std::min(1.f,tension+dt*
                        std::max(.20f,.52f-.018f*float(journal.line_level)-
                                          .014f*float(journal.endurance_level)));
                    reel=std::min(1.f,reel+dt*
                        (.18f+.025f*float(journal.gear)+
                         .013f*float(journal.reel_level)+
                         .016f*float(journal.rod_handling_level))*
                        (tension>.86f?.25f:1.f));
                }else tension=std::max(0.f,tension-dt*.85f);
                if(reel>=1.f){
                    const unsigned id=unsigned(hooked);
                    if(id<journal.caught.size()&&journal.total<1000000u&&
                       journal.caught[id]<1000000u&&
                       journal.fish_inventory[id]<1000000u){
                        ++journal.caught[id];++journal.total;
                        ++journal.fish_inventory[id];
                    }
                    phase=FishingPhase::Landed;
                }
                break;
            case FishingPhase::Ready:case FishingPhase::Landed:break;
        }
    }
    bool AcknowledgeCatch(){
        if(phase!=FishingPhase::Landed)return false;
        phase=FishingPhase::Ready;time=reel=tension=0.f;return true;
    }
    bool SellFish(){
        if(phase!=FishingPhase::Ready)return false;
        for(unsigned id=0;id<journal.fish_inventory.size();++id){
            if(!journal.fish_inventory[id])continue;
            if(journal.credits>1000000u-kFishCreditAwards[id])return false;
            --journal.fish_inventory[id];journal.credits+=kFishCreditAwards[id];
            return true;
        }
        return false;
    }
    bool SellShellfish(){
        constexpr uint32_t value=25u;
        if(phase!=FishingPhase::Ready||!journal.shellfish_inventory||
           journal.credits>1000000u-value)return false;
        --journal.shellfish_inventory;journal.credits+=value;return true;
    }
    bool SubmitFinds(){
        if(phase!=FishingPhase::Ready)return false;
        if(journal.submitted_beach_sites==journal.beach_sites&&
           !journal.starter_tokens)return false;
        journal.submitted_beach_sites=journal.beach_sites;
        journal.submitted_discoveries=journal.discoveries;
        journal.submitted_starter_tokens+=journal.starter_tokens;
        journal.starter_tokens=0u;
        return true; // registration unlocks purchases, never grants money
    }
    bool Discover(uint32_t site,BeachFind kind){
        if(phase!=FishingPhase::Ready||site>=64u||
           unsigned(kind)>=unsigned(BeachFind::Count)||
           (journal.beach_sites&(uint64_t(1)<<site)))return false;
        journal.beach_sites|=uint64_t(1)<<site;
        journal.discoveries|=FindMask(kind);
        return true; // no money, only equipment-shop permissions
    }
    bool DiscoverCity(uint32_t site,CityFind kind){
        if(site>=64u||unsigned(kind)>=unsigned(CityFind::Count)||
           (journal.city_sites&(uint64_t(1)<<site)))return false;
        journal.city_sites|=uint64_t(1)<<site;
        journal.city_discoveries|=CityFindMask(kind);
        return true;
    }
    bool PurchaseCharacter(CharacterSkill skill){
        if(phase!=FishingPhase::Ready)return false;
        const auto offer=NextCharacter(journal,skill);
        if(!offer.available||!offer.unlocked||!offer.affordable)return false;
        journal.credits-=offer.credits;
        switch(skill){
            case CharacterSkill::Stride:++journal.stride_level;break;
            case CharacterSkill::RodHandling:++journal.rod_handling_level;break;
            case CharacterSkill::Endurance:++journal.endurance_level;break;
            case CharacterSkill::Observation:++journal.observation_level;break;
            case CharacterSkill::Deckhand:++journal.deckhand_level;break;
            default:return false;
        }
        return true;
    }
    uint32_t UpgradeCost()const{return NextEquipment(journal,Equipment::Rod).credits;}
    bool Purchase(Equipment item){
        if(phase!=FishingPhase::Ready)return false;
        const auto offer=NextEquipment(journal,item);
        if(!offer.available||!offer.unlocked||!offer.affordable)return false;
        journal.credits-=offer.credits;
        switch(item){
            case Equipment::Rod:++journal.gear;break;
            case Equipment::Reel:++journal.reel_level;break;
            case Equipment::Line:++journal.line_level;break;
            case Equipment::Lures:++journal.lure_level;break;
            case Equipment::ShoreKit:++journal.shore_kit_level;break;
            default:return false;
        }
        return true;
    }
    bool UpgradeTackle(){return Purchase(Equipment::Rod);}
};
// V5 retains inventory and office registrations in the SAME save slot.
// Clean-start development format; old prototype versions are not migrated.
inline std::string SaveJournal(const Journal& j){
    std::string out="SITKA-FISH-5\n";
    const std::array<uint64_t,32> fields={{
        j.seed,j.credits,j.gear,j.total,j.caught[0],j.caught[1],
        j.caught[2],j.caught[3],j.helm_upgrades,j.hull_tier,
        j.reel_level,j.line_level,j.lure_level,j.shore_kit_level,
        j.discoveries,j.beach_sites,j.stride_level,
        j.rod_handling_level,j.endurance_level,j.observation_level,
        j.deckhand_level,j.city_sites,j.city_discoveries,
        j.fish_inventory[0],j.fish_inventory[1],j.fish_inventory[2],
        j.fish_inventory[3],j.shellfish_inventory,j.submitted_beach_sites,
        j.starter_tokens,j.submitted_starter_tokens,j.submitted_discoveries
    }};
    for(uint64_t n:fields)out+=std::to_string(n)+"\n";
    return out;
}
inline bool LoadJournal(std::string_view bytes,Journal& out){
    constexpr std::string_view version="SITKA-FISH-5\n";
    if(bytes.substr(0,version.size())!=version)return false;
    bytes.remove_prefix(version.size());
    constexpr unsigned fields=32u;
    if(bytes.size()>512u)return false;
    std::array<uint64_t,32> n{};
    for(unsigned i=0;i<fields;++i){
        const auto end=bytes.find('\n');
        if(end==std::string_view::npos||end==0u||end>20u)return false;
        const char* begin=bytes.data();
        const auto parsed=std::from_chars(begin,begin+end,n[i]);
        if(parsed.ec!=std::errc{}||parsed.ptr!=begin+end)return false;
        bytes.remove_prefix(end+1u);
    }
    if(!bytes.empty()||n[0]==0u||n[0]>UINT32_MAX||
       n[1]>1000000u||n[2]>kEquipmentMax[0]||n[3]>1000000u||
       n[8]>65535u||n[9]>3u||n[10]>kEquipmentMax[1]||
       n[11]>kEquipmentMax[2]||n[12]>kEquipmentMax[3]||
       n[13]>kEquipmentMax[4]||
       n[14]>=(uint32_t(1)<<unsigned(BeachFind::Count))||
       (n[15]!=0u&&n[14]==0u)||
       n[16]>kCharacterSkillMax[0]||n[17]>kCharacterSkillMax[1]||
       n[18]>kCharacterSkillMax[2]||n[19]>kCharacterSkillMax[3]||
       n[20]>kCharacterSkillMax[4]||
       n[22]>=(uint32_t(1)<<unsigned(CityFind::Count))||
       (n[21]!=0u&&n[22]==0u))return false;
    uint64_t sum=0;
    for(unsigned i=4;i<8;++i){
        if(n[i]>1000000u)return false;
        sum+=n[i];
    }
    if(sum!=n[3])return false;
    {
        for(unsigned i=23u;i<=27u;++i)if(n[i]>1000000u)return false;
        if((n[28]&~n[15])!=0u||n[29]>1u||n[30]>1u||n[29]+n[30]>1u||
           n[31]>=(uint32_t(1)<<unsigned(BeachFind::Count))||
           (n[31]&~n[14])!=0u||(n[28]!=0u&&n[31]==0u))return false;
    }
    Journal j;
    j.seed=static_cast<uint32_t>(n[0]);
    j.credits=static_cast<uint32_t>(n[1]);
    j.gear=static_cast<uint32_t>(n[2]);
    j.total=static_cast<uint32_t>(n[3]);
    for(unsigned i=0;i<4;++i)j.caught[i]=static_cast<uint32_t>(n[i+4u]);
    j.helm_upgrades=static_cast<uint32_t>(n[8]);
    j.hull_tier=static_cast<uint32_t>(n[9]);
    j.reel_level=static_cast<uint32_t>(n[10]);
    j.line_level=static_cast<uint32_t>(n[11]);
    j.lure_level=static_cast<uint32_t>(n[12]);
    j.shore_kit_level=static_cast<uint32_t>(n[13]);
    j.discoveries=static_cast<uint32_t>(n[14]);
    j.beach_sites=n[15];
    j.stride_level=static_cast<uint32_t>(n[16]);
    j.rod_handling_level=static_cast<uint32_t>(n[17]);
    j.endurance_level=static_cast<uint32_t>(n[18]);
    j.observation_level=static_cast<uint32_t>(n[19]);
    j.deckhand_level=static_cast<uint32_t>(n[20]);
    j.city_sites=n[21];
    j.city_discoveries=static_cast<uint32_t>(n[22]);
    {
        for(unsigned i=0u;i<4u;++i)j.fish_inventory[i]=static_cast<uint32_t>(n[23u+i]);
        j.shellfish_inventory=static_cast<uint32_t>(n[27]);
        j.submitted_beach_sites=n[28];
        j.starter_tokens=static_cast<uint32_t>(n[29]);
        j.submitted_starter_tokens=static_cast<uint32_t>(n[30]);
        j.submitted_discoveries=static_cast<uint32_t>(n[31]);
    }
    out=j;return true;
}
}} // namespace reverie::sitka
#endif
