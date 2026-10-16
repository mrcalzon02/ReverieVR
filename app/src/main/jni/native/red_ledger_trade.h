#ifndef REVERIE_RED_LEDGER_TRADE_H
#define REVERIE_RED_LEDGER_TRADE_H
#include "red_ledger_simulation.h"
#include <array>
#include <cstddef>
#include <cstdint>
namespace reverie { namespace redledger { namespace trade {
constexpr int kDrinks=19; // house lager and 18 original Unreal catalog drinks
constexpr int kSnacks=6;
constexpr int kFactions=6; // six supplier factions 101..606
constexpr int kOffers=kDrinks+kSnacks;
enum class Room : int32_t { Bar=0, Stockroom=1, Living=2 };
struct Product {
    const char *name;
    int32_t wholesale_cents;
    int32_t units;
    int32_t service_premium_cents;
    int32_t faction;
};
static constexpr Product kDrinkCatalog[kDrinks] = {
    {"HOUSE LAGER",0,0,0,-1}, // legacy house tap, outside the 24 branded SKUs
    {"BLUE HARBOR DRAFT",800,3,80,0},
    {"GREEN HARBOR LAGER",1000,3,100,0},
    {"HARBOR RESERVE WHISKEY",3200,3,220,0},
    {"AMBER UNION ALE",700,3,80,1},
    {"UNION STAR LAGER",900,3,100,1},
    {"UNION HOUSE WHISKEY",3000,3,220,1},
    {"REDLINE DARK ALE",900,3,80,2},
    {"REDLINE SPICED RUM",1100,3,100,2},
    {"REDLINE BARREL WHISKEY",3300,3,220,2},
    {"IRON MUG STOUT",800,3,80,3},
    {"CROSSMARK BITTER",1000,3,100,3},
    {"DOCKSIDE BROWN WHISKEY",3400,3,220,3},
    {"MARKET AMBER ALE",1000,3,80,4},
    {"GREENMARK BITTER",1200,3,100,4},
    {"GREENMARK CLEAR WHISKEY",3500,3,220,4},
    {"GREEN DRAFT LAGER",900,3,80,5},
    {"MAZR BLUE VODKA",1100,3,100,5},
    {"MAZR CRYSTAL VODKA",3600,3,220,5}
};
static constexpr Product kSnackCatalog[kSnacks] = {
    {"CANDIED PLUMS",600,3,65,0},
    {"GOLDEN FRY CHIPS",500,3,65,1},
    {"HOT RATION CRISPS",700,3,65,2},
    {"SALT PRETZEL BAG",600,3,65,3},
    {"STREET SAUSAGE TIN",800,3,65,4},
    {"BEANS CHICKEN AND POTATO",700,3,65,5}
};
static constexpr const char *kFactionNames[kFactions] = {
    "WESTERN CONCORDANT",
    "UED PROVISIONERS",
    "BARFORSOGRAD UNION",
    "SALIHORSK DRY STILLS",
    "MOZOLI BREW'S",
    "MAZR VODKA"
};
// Patron identities are independent of supplier-array ordering.
constexpr int FactionFor(PatronKind p) {
    switch(p) {
        case PatronKind::RailWorker: return 2;
        case PatronKind::NightClerk: return 0;
        case PatronKind::MilitiaVeteran: return 5;
        case PatronKind::Mechanic: return 4;
        case PatronKind::DirectorateCourier: return 1;
        case PatronKind::SalihorskHauler: return 3;
    }
    return -1;
}
constexpr int FavoriteDrink(PatronKind p) {
    switch(p) {
        case PatronKind::RailWorker: return 7;
        case PatronKind::NightClerk: return 1;
        case PatronKind::MilitiaVeteran: return 16;
        case PatronKind::Mechanic: return 13;
        case PatronKind::DirectorateCourier: return 4;
        case PatronKind::SalihorskHauler: return 10;
    }
    return 0;
}
constexpr int FavoriteSnack(PatronKind p) {
    const int faction=FactionFor(p);
    return faction>=0&&faction<kFactions?faction:-1;
}
struct ServiceBonus { int32_t cents; int32_t reputation; int32_t favor; };
inline ServiceBonus RateService(PatronKind patron,int drink,int snack) {
    const int faction=FactionFor(patron);
    if (faction<0||faction>=kFactions||drink<0||drink>=kDrinks||snack < -1||snack>=kSnacks)
        return {0,0,0};
    const bool favorite_drink=(drink==FavoriteDrink(patron));
    const bool aligned_drink=(kDrinkCatalog[drink].faction==faction);
    const bool favorite_snack=(snack==FavoriteSnack(patron));
    const bool aligned_snack=(snack>=0 && kSnackCatalog[snack].faction==faction);
    const int32_t money=kDrinkCatalog[drink].service_premium_cents
        +(snack>=0?kSnackCatalog[snack].service_premium_cents:0)
        +(favorite_drink?110:(aligned_drink?45:0))
        +(favorite_snack?70:(aligned_snack?25:0));
    const int32_t favor=(favorite_drink?2:(aligned_drink?1:-1))
        +(favorite_snack?2:(aligned_snack?1:0));
    const int32_t reputation=(favorite_drink?2:(aligned_drink?1:0))
        +(favorite_snack?1:0);
    return {money,reputation,favor};
}
class Trade {
public:
    static constexpr size_t kSerializedSize=4u*(2u+kDrinks+kSnacks+kFactions+4u+1u); // checked v2
    Trade(){ Reset(); }
    void Reset(){
        drink_stock_.fill(0); snack_stock_.fill(0); favor_.fill(0);
        for(int i=1;i<kDrinks;i+=3)drink_stock_[i]=1;
        snack_stock_[0]=1; snack_stock_[2]=1; snack_stock_[4]=1;
        chosen_drink_=0; chosen_snack_=-1; offer_=0; room_=Room::Bar;
    }
    Room room() const {return room_;}
    bool Enter(Room room) {
        if(static_cast<int>(room)<0 || static_cast<int>(room)>2)return false;
        if(room_==room)return false;
        room_=room;return true;
    }
    int drink() const {return chosen_drink_;}
    int snack() const {return chosen_snack_;}
    int offer() const {return offer_;}
    int stock_drink(int i) const {return (i>=0&&i<kDrinks)?drink_stock_[i]:0;}
    int stock_snack(int i) const {return (i>=0&&i<kSnacks)?snack_stock_[i]:0;}
    int favor(int faction) const {
        return (faction>=0&&faction<kFactions)?favor_[faction]:0;
    }
    int offer_cost() const {
        const Product &product=offer_<kDrinks?kDrinkCatalog[offer_]:
            kSnackCatalog[offer_-kDrinks];
        const int base=product.wholesale_cents;
        if(product.faction<0)return base;
        const int standing=favor_[product.faction];
        // Favor is an actual economic leverage point, not only a HUD score.
        // Favorable suppliers offer a capped wholesale discount; hostility
        // causes a modest surcharge. All figures remain non-negative/capped.
        const int percent=standing>=20?85:standing>=10?90:
            standing>=5?95:standing<=-5?110:100;
        return (base*percent+99)/100;
    }
    const char *offer_faction() const {
        const int owner=offer_<kDrinks?kDrinkCatalog[offer_].faction:
            kSnackCatalog[offer_-kDrinks].faction;
        return owner>=0?kFactionNames[owner]:"INDEPENDENT";
    }
    const char *offer_name() const {
        return offer_<kDrinks?kDrinkCatalog[offer_].name:
            kSnackCatalog[offer_-kDrinks].name;
    }
    bool CycleOffer() {offer_=(offer_+1)%kOffers;return true;}
    bool SelectDrink(int house_units) {
        for(int step=1;step<=kDrinks;++step) {
            const int next=(chosen_drink_+step)%kDrinks;
            if((next==0&&house_units>0)||(next>0&&drink_stock_[next]>0)){
                if(next==chosen_drink_)return false;
                chosen_drink_=next;return true;
            }
        }
        return false;
    }
    bool SelectSnack(){
        for(int step=1;step<=kSnacks+1;++step){
            const int next=((chosen_snack_+1+step)%(kSnacks+1))-1;
            if(next==-1||snack_stock_[next]>0){
                if(next==chosen_snack_)return false;
                chosen_snack_=next;return true;
            }
        }
        return false;
    }
    bool DrinkAvailable(int house_units) const {
        return chosen_drink_==0?house_units>0:drink_stock_[chosen_drink_]>0;
    }
    bool SnackAvailable() const {
        return chosen_snack_<0||snack_stock_[chosen_snack_]>0;
    }
    bool ConsumeDrink() {
        if(chosen_drink_==0)return true; // house lager deducted by Simulation
        if(drink_stock_[chosen_drink_]<=0)return false;
        --drink_stock_[chosen_drink_];return true;
    }
    void ConsumeSnack() {
        if(chosen_snack_>=0&&snack_stock_[chosen_snack_]>0)
            --snack_stock_[chosen_snack_];
    }
    bool AcceptStock() {
        if(offer_==0)return false; // house lager uses legacy beer crate
        if(offer_<kDrinks) {
            if(drink_stock_[offer_]>99990)return false;
            drink_stock_[offer_]+=kDrinkCatalog[offer_].units;
        } else {
            const int id=offer_-kDrinks;
            if(snack_stock_[id]>99990)return false;
            snack_stock_[id]+=kSnackCatalog[id].units;
        }
        return true;
    }
    void RecordService(PatronKind patron) {
        const int faction=FactionFor(patron);
        const ServiceBonus bonus=RateService(patron,chosen_drink_,chosen_snack_);
        const int64_t next=static_cast<int64_t>(favor_[faction])+bonus.favor;
        favor_[faction]=static_cast<int32_t>(
            next<-100000?-100000:(next>100000?100000:next));
        chosen_drink_=0;chosen_snack_=-1;
    }
    bool Serialize(uint8_t *out,size_t capacity) const {
        if(!out||capacity<kSerializedSize)return false;
        std::array<uint32_t,kSerializedSize/4u> words{};
        words[0]=0x31545252u;words[1]=2u;
        size_t at=2u;
        for(int v:drink_stock_)words[at++]=static_cast<uint32_t>(v);
        for(int v:snack_stock_)words[at++]=static_cast<uint32_t>(v);
        for(int v:favor_)words[at++]=static_cast<uint32_t>(v);
        words[at++]=static_cast<uint32_t>(chosen_drink_);
        words[at++]=static_cast<uint32_t>(chosen_snack_);
        words[at++]=static_cast<uint32_t>(offer_);
        words[at++]=static_cast<uint32_t>(room_);
        if(at!=words.size()-1u)return false;
        uint32_t digest=2166136261u;
        for(size_t i=0;i+1u<words.size();++i){
            const uint32_t v=words[i];
            for(int b=0;b<4;++b) {
                const uint8_t byte=static_cast<uint8_t>(v>>(8*b));
                out[i*4u+static_cast<size_t>(b)]=byte;
                digest=(digest^byte)*16777619u;
            }
        }
        for(int b=0;b<4;++b)
            out[(words.size()-1u)*4u+static_cast<size_t>(b)]=static_cast<uint8_t>(digest>>(8*b));
        return true;
    }
    bool Deserialize(const uint8_t *in,size_t size){
        if(!in||size!=kSerializedSize)return false;
        std::array<uint32_t,kSerializedSize/4u> words{};
        uint32_t digest=2166136261u;
        for(size_t i=0;i<words.size();++i){
            for(int b=0;b<4;++b){
                const uint8_t v=in[i*4u+static_cast<size_t>(b)];
                words[i]|=static_cast<uint32_t>(v)<<(8*b);
                if(i+1u<words.size())digest=(digest^v)*16777619u;
            }
        }
        if(words[0]!=0x31545252u||words[1]!=2u||words.back()!=digest)
            return false;
        Trade copy;
        size_t at=2u;
        for(int &v:copy.drink_stock_){v=static_cast<int32_t>(words[at++]);if(v<0||v>100000)return false;}
        for(int &v:copy.snack_stock_){v=static_cast<int32_t>(words[at++]);if(v<0||v>100000)return false;}
        for(int &v:copy.favor_){v=static_cast<int32_t>(words[at++]);if(v < -100000||v>100000)return false;}
        copy.chosen_drink_=static_cast<int32_t>(words[at++]);
        copy.chosen_snack_=static_cast<int32_t>(words[at++]);
        copy.offer_=static_cast<int32_t>(words[at++]);
        const int32_t room=static_cast<int32_t>(words[at++]);
        if(at!=words.size()-1u||copy.drink_stock_[0]!=0||
            copy.chosen_drink_<0||copy.chosen_drink_>=kDrinks||
            copy.chosen_snack_<-1||copy.chosen_snack_>=kSnacks||
            copy.offer_<0||copy.offer_>=kOffers||room<0||room>2)return false;
        copy.room_=static_cast<Room>(room);
        *this=copy;return true;
    }
private:
    std::array<int32_t,kDrinks> drink_stock_{};
    std::array<int32_t,kSnacks> snack_stock_{};
    std::array<int32_t,kFactions> favor_{};
    int32_t chosen_drink_=0,chosen_snack_=-1,offer_=0;
    Room room_=Room::Bar;
};
}}}
#endif
