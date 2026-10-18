#include "breakwater_campaign.h"
#include "breakwater_armor.h"
#include "breakwater_silhouettes.h"
#include "breakwater_infantry_tactics.h"
#include "breakwater_sprite_atlas.h"
#include "breakwater_terrain.h"
#include "breakwater_sky.h"
#include "breakwater_damage_feedback.h"
#include "breakwater_aircraft_feedback.h"
#include "breakwater_surface_aim.h"
#include "breakwater_combat_input.h"
#include "breakwater_pointer_focus.h"
#include "breakwater_impacts.h"
#include "breakwater_wrecks.h"
#include "breakwater_cover.h"
#include "breakwater_weapons.h"
#include "breakwater_support_turret.h"
#include "breakwater_checkpoint.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
#include <iostream>
using namespace reverie::breakwater;

int main() {
    // Real shoreline cover must share geometry with physics, block only
    // lower/parabolic shots, take actual ammunition and disappear on death.
    {
        const auto shingle=MakeShoreCover(0);
        for(const ShoreCover &wall:shingle)assert(wall.hp==0);
        const auto iron=MakeShoreCover(1);
        const auto basalt=MakeShoreCover(2);
        for(size_t i=0;i<4;++i) {
            assert(iron[i].hp>0 || i==3);
            assert(basalt[i].hp>0);
            if(iron[i].hp>0) {
                assert(iron[i].hp==iron[i].max_hp);
                assert(CoverHeight(iron[i])>.43f);
            }
        }
        float distance=0.0f;
        const float origin[]={0.0f,-.70f,0.0f};
        const float straight[]={0.0f,0.0f,-1.0f};
        const int wall=CoverTargetFromArray(iron,origin,straight,.014f,&distance);
        assert(wall==1 && distance>1.0f && distance<1.2f);
        const float behind[]={0.0f,-.66f,-1.65f};
        float soldierDepth=0;
        assert(ProjectileHitsSphere(origin,straight,.014f,
                                   behind,.30f,&soldierDepth));
        assert(distance<soldierDepth);
        // Enter through the TOP while moving through the wall's Z
        // thickness. A front-face-only ray test would wrongly miss it.
        const float topOrigin[]={0.0f,-.40f,0.0f};
        const float descending[]={0.0f,-.16f,-1.0f};
        float topHit=0;
        assert(ProjectileHitsCover(topOrigin,descending,.014f,
                                   iron[1],&topHit));
        assert(topHit>1.05f && topHit<1.29f);
        float topPoint[3]={};
        assert(ProjectilePosition(topOrigin,descending,topHit,
                                  .014f,topPoint));
        assert(topPoint[1]<=kCoverBaseY+CoverHeight(iron[1])+.001f);
        const float above[]={0.0f,-.18f,0.0f};
        const float aside[]={1.0f,-.70f,0.0f};
        assert(!ProjectileHitsCover(above,straight,.014f,
                                   iron[1],&distance));
        assert(!ProjectileHitsCover(aside,straight,.014f,
                                   iron[1],&distance));
        const float noTravel[]={0.0f,0.0f,0.0f};
        assert(!ProjectileHitsCover(origin,noTravel,.014f,
                                   iron[1],&distance));
        const float wrongWay[]={0.0f,0.0f,1.0f};
        assert(!ProjectileHitsCover(origin,wrongWay,.014f,
                                   iron[1],&distance));
        const float nan[]={std::numeric_limits<float>::quiet_NaN(),
                           0.0f,0.0f};
        assert(!ProjectileHitsCover(nan,straight,.014f,
                                   iron[1],&distance));
        const auto inspectRender=[](const ShoreCover &part,uint8_t location) {
            size_t boxes=0;
            EmitShoreCover(location,part,false,[&](auto... args) {
                const float values[]={args...};
                static_assert(sizeof...(args)==9,
                    "Cover box consumes x,y,z,w,h,d,r,g,b");
                for(float val:values)assert(std::isfinite(val));
                assert(values[3]>0.0f && values[4]>0.0f);
                ++boxes;
            });
            return boxes;
        };
        assert(inspectRender(iron[1],1)==4);
        assert(inspectRender(shingle[0],0)==0);

        Campaign fighting(Difficulty::Casual,1234);
        assert(fighting.Start());
        uint8_t shop[Campaign::kSerializedSize]={};
        assert(fighting.SerializeShop(shop,sizeof(shop)));
        shop[16]=1; // Iron Quay, same documented checkpoint field
        assert(fighting.DeserializeShop(shop,sizeof(shop)));
        assert(fighting.BeginWave());
        assert(fighting.shore_cover()[1].hp==105);
        assert(!fighting.FireAtCover(3)); // slot does not exist at Iron Quay
        assert(!fighting.FireAtCover(4)); // out of fixed pool
        const uint8_t originalAmmo=fighting.magazine_left();
        assert(fighting.FireAtCover(1));
        assert(fighting.shore_cover()[1].hp==85);
        assert(CoverHeight(fighting.shore_cover()[1])<
               CoverHeight(iron[1]));
        assert(fighting.magazine_left()==originalAmmo-1);
        assert(!fighting.FireAtCover(1)); // weapon cooldown
        size_t shots=1;
        while(fighting.shore_cover()[1].hp>0 && shots<12) {
            // Campaign::Tick caps input delta to 0.10 seconds; advance
            // through real frames rather than expecting .35s in one call.
            for(int frame=0;frame<4;++frame)fighting.Tick(.10f);
            assert(fighting.FireAtCover(1));
            ++shots;
        }
        assert(shots==6 && fighting.shore_cover()[1].hp==0);
        assert(CoverHeight(fighting.shore_cover()[1])==0.0f);
        float blocked=0.0f;
        assert(CoverTarget(fighting,origin,straight,.014f,&blocked)==-1);
        assert(!fighting.FireAtCover(1));
        assert(inspectRender(fighting.shore_cover()[1],1)==0);
        Campaign shingleCampaign(Difficulty::Casual,44);
        assert(shingleCampaign.Start() && shingleCampaign.BeginWave());
        assert(CoverTarget(shingleCampaign,origin,straight,.014f,
                           &blocked)==-1);
    }

    // Cover-seeking landing-party riflemen truly maneuver behind live
    // obstacles and fall back to their open lane if cover is destroyed.
    {
        Infantry rifle{};
        rifle.active=true;
        rifle.tactic=InfantryTactic::Rifle;
        rifle.x=.24f;rifle.z=-2.28f;rifle.lane_x=rifle.x;
        const auto iron=MakeShoreCover(1);
        const auto none=MakeShoreCover(0);
        const float originalLine=InfantryLineZ(rifle);
        assert(originalLine==-1.34f);
        AssignInfantryCover(rifle,none,0);
        assert(rifle.cover_slot==-1 && rifle.lane_x==.24f);
        assert(InfantryLineZ(rifle)==originalLine);
        AssignInfantryCover(rifle,iron,0);
        assert(rifle.cover_slot==1);
        assert(InfantryCoverOrdered(rifle));
        assert(InfantryLineZ(rifle)<-1.34f);
        assert(std::abs(InfantryLineZ(rifle)-(-1.46f))<.00001f);
        assert(rifle.open_lane_x==.24f);
        assert(std::abs(rifle.lane_x-.15f)<.00001f);
        const float coveredEta=InfantrySecondsToReady(rifle);
        assert(coveredEta>5.0f && coveredEta<7.0f);
        assert(!InfantryReady(rifle));
        AdvanceInfantry(rifle,2.0f);
        assert(rifle.x<.24f && rifle.z>-2.28f);
        AdvanceInfantry(rifle,30.0f);
        assert(InfantryReady(rifle) && rifle.x==rifle.lane_x);
        assert(std::abs(rifle.z+1.46f)<.00001f);
        assert(InfantrySecondsToReady(rifle)<.00001f);
        auto destroyed=iron;
        destroyed[1].hp=0;
        RefreshInfantryCover(rifle,destroyed);
        assert(rifle.cover_slot==-1);
        assert(rifle.regrouping); // lateral return must not be skipped
        assert(rifle.lane_x==rifle.open_lane_x);
        assert(!InfantryReady(rifle)); // must regroup to exposed line
        const float retreatEta=InfantrySecondsToReady(rifle);
        assert(retreatEta>.0f && retreatEta<2.0f);
        AdvanceInfantry(rifle,retreatEta+1.0f);
        assert(InfantryReady(rifle));
        assert(!rifle.regrouping);
        assert(std::abs(rifle.z+1.34f)<.00001f);
        assert(std::abs(rifle.x-.24f)<.00001f);
        assert(InfantryLineZ(rifle)==originalLine);
        RefreshInfantryCover(rifle,destroyed);
        assert(rifle.cover_slot==-1); // never reacquire destroyed cover

        // Actual occupied cover can pin a soldier for a finite time;
        // remote soldiers and flanking units are not magically pinned.
        Infantry pinned=rifle;
        assert(!CoverSheltersInfantry(pinned,iron[1],1));
        pinned.x=.15f;pinned.z=-1.46f;
        pinned.lane_x=.15f;pinned.cover_slot=1;
        pinned.cover_line_z=-1.46f;
        assert(CoverSheltersInfantry(pinned,iron[1],1));
        assert(!CoverSheltersInfantry(pinned,iron[0],0));
        pinned.pinned_seconds=kInfantryPinSeconds;
        assert(InfantryPinned(pinned) && !InfantryReady(pinned));
        assert(std::abs(InfantrySecondsToReady(pinned)-1.20f)<.00001f);
        assert(InfantrySprite(pinned)==SpriteId::InfantryPinned);
        assert(InfantrySprite(pinned,true)==SpriteId::InfantryPinned);
        assert(!InfantryFiringPose(pinned,1u,0));
        AdvanceInfantry(pinned,.45f);
        assert(InfantryPinned(pinned) && !InfantryReady(pinned));
        AdvanceInfantry(pinned,.80f);
        assert(!InfantryPinned(pinned) && InfantryReady(pinned));
        assert(InfantrySprite(pinned)==SpriteId::InfantryRifleReady);
        // A pinned soldier ordered to evacuate destroyed cover stops
        // while flinching, then travels for the remainder of the frame.
        // The HUD readiness estimate must include BOTH phases.
        Infantry displaced=pinned;
        displaced.cover_slot=-1;
        displaced.regrouping=true;
        displaced.x=.15f;displaced.z=-1.46f;
        displaced.lane_x=.95f;
        displaced.open_lane_x=.95f;
        displaced.pinned_seconds=kInfantryPinSeconds;
        const float delayEta=InfantrySecondsToReady(displaced);
        assert(delayEta>5.40f && delayEta<5.43f);
        Infantry split=displaced;
        AdvanceInfantry(displaced,.75f);
        assert(std::abs(displaced.x-.15f)<.00001f);
        assert(std::abs(displaced.z+1.46f)<.00001f);
        assert(InfantryPinned(displaced));
        AdvanceInfantry(displaced,.60f);
        assert(!InfantryPinned(displaced));
        assert(displaced.x>.15f && displaced.z>-1.46f);
        assert(std::abs(InfantrySecondsToReady(displaced)-
                        (delayEta-1.35f))<.001f);
        // The same elapsed time split into native 0.05s substeps
        // cannot advance a rifleman during suppression.
        for(int part=0;part<27;++part)AdvanceInfantry(split,.05f);
        assert(std::abs(split.x-displaced.x)<.0002f);
        assert(std::abs(split.z-displaced.z)<.0002f);
        assert(std::abs(split.pinned_seconds-displaced.pinned_seconds)<.0002f);
        AdvanceInfantry(displaced,InfantrySecondsToReady(displaced)+.15f);
        assert(InfantryReady(displaced));
        assert(!displaced.regrouping);
        assert(std::abs(displaced.x-.95f)<.00002f);

        Infantry remote=pinned;remote.z=-2.20f;
        assert(!CoverSheltersInfantry(remote,iron[1],1));
        Infantry flanking=pinned;flanking.tactic=InfantryTactic::FlankLeft;
        assert(!CoverSheltersInfantry(flanking,iron[1],1));

        // A troop starting between Iron Quay defenses may be within
        // 1.90 of another barricade but the structures themselves are
        // not neighbors. Losing center cover must still trigger fallback.
        Infantry between{};
        between.active=true;
        between.tactic=InfantryTactic::Rifle;
        between.x=1.0f;
        between.z=-2.28f;
        between.lane_x=between.x;
        AssignInfantryCover(between,iron,0);
        assert(between.cover_slot==1);
        RefreshInfantryCover(between,destroyed);
        assert(between.cover_slot==-1 && between.regrouping);
        assert(between.lane_x==between.open_lane_x);

        Infantry distant=rifle;
        distant.x=3.1f;distant.lane_x=distant.x;
        AssignInfantryCover(distant,iron,0);
        // Outer Iron Quay wall at 2.35 remains close enough for a
        // local fallback, but the unit must never teleport there.
        assert(distant.cover_slot==2);
        assert(distant.x==3.1f);
        Infantry flanker=rifle;
        flanker.tactic=InfantryTactic::FlankLeft;
        flanker.cover_slot=-1;
        flanker.lane_x=flanker.x-.85f;
        AssignInfantryCover(flanker,iron,2);
        assert(flanker.cover_slot==-1);
        assert(InfantryLineZ(flanker)==-1.34f);
        Infantry support=rifle;
        support.tactic=InfantryTactic::Support;
        AssignInfantryCover(support,iron,3);
        assert(support.cover_slot==-1);
        assert(InfantryLineZ(support)==-1.68f);

        Infantry invalid=rifle;
        invalid.x=std::numeric_limits<float>::quiet_NaN();
        AssignInfantryCover(invalid,iron,0);
        assert(invalid.cover_slot==-1);
        invalid=rifle;invalid.cover_slot=127;
        RefreshInfantryCover(invalid,iron);
        assert(invalid.cover_slot==-1);
        assert(invalid.lane_x==invalid.open_lane_x);
    }

    // A close Blackcap wall is a genuine alternative defensive position.
    // Relocation changes the ordered lane, never teleports the soldier or
    // silently resumes shooting while the soldier crosses exposed ground.
    {
        auto walls=MakeShoreCover(2);
        Infantry soldier{};
        soldier.active=true;
        soldier.tactic=InfantryTactic::Rifle;
        soldier.x=-.63f;
        soldier.z=-2.28f;
        soldier.lane_x=soldier.x;
        AssignInfantryCover(soldier,walls,0);
        assert(soldier.cover_slot==1);
        AdvanceInfantry(soldier,10.0f);
        assert(InfantryReady(soldier));
        const float originalX=soldier.open_lane_x;
        const float physicalX=soldier.x;
        walls[1].hp=0;
        RefreshInfantryCover(soldier,walls);
        assert(soldier.cover_slot==2);
        assert(soldier.open_lane_x==originalX);
        assert(std::abs(soldier.lane_x-.63f)<.00001f);
        assert(soldier.x==physicalX);
        assert(!InfantryReady(soldier));
        const float flankEta=InfantrySecondsToReady(soldier);
        assert(flankEta>6.0f && flankEta<7.0f);
        AdvanceInfantry(soldier,flankEta+1.0f);
        assert(InfantryReady(soldier));
        assert(std::abs(soldier.z+1.48f)<.00001f);
        walls[2].hp=0;
        RefreshInfantryCover(soldier,walls);
        assert(soldier.cover_slot==3); // another intact near neighbor
        walls[3].hp=0;
        RefreshInfantryCover(soldier,walls);
        assert(soldier.cover_slot==-1);
        assert(soldier.regrouping);
        assert(soldier.lane_x==originalX);
        assert(!InfantryReady(soldier));
        AdvanceInfantry(soldier,30.0f);
        assert(InfantryReady(soldier));
        assert(!soldier.regrouping);
        assert(soldier.x==originalX);
        assert(std::abs(soldier.z+1.34f)<.00001f);
    }

    // Landing-craft tactics are real simulation positions, not a separate
    // animation timer or fake HUD threats. Early rowboats remain riflemen.
    {
        assert(AssignInfantryTactic(UnitType::Rowboat,0)==InfantryTactic::Rifle);
        assert(AssignInfantryTactic(UnitType::LandingCraft,0)==InfantryTactic::Rifle);
        assert(AssignInfantryTactic(UnitType::LandingCraft,1)==InfantryTactic::FlankLeft);
        assert(AssignInfantryTactic(UnitType::LandingCraft,2)==InfantryTactic::FlankRight);
        assert(AssignInfantryTactic(UnitType::LandingCraft,3)==InfantryTactic::Support);
        assert(AssignInfantryTactic(UnitType::LandingCraft,5)==InfantryTactic::FlankLeft);
        assert(InfantryShotDamage(InfantryTactic::Rifle)==2);
        assert(InfantryShotDamage(InfantryTactic::FlankLeft)==2);
        assert(InfantryShotDamage(InfantryTactic::Support)==1);
        assert(InfantryLineZ(InfantryTactic::Support)<InfantryLineZ(InfantryTactic::Rifle));
        assert(InfantryLaneX(-3.0f,InfantryTactic::FlankLeft)==-3.1f);
        assert(InfantryLaneX(3.0f,InfantryTactic::FlankRight)==3.1f);
        assert(InfantryLaneX(.5f,InfantryTactic::Rifle)==.5f);
        Infantry rifle{};
        rifle.active=true;rifle.x=.35f;rifle.z=-2.28f;
        rifle.tactic=InfantryTactic::Rifle;rifle.lane_x=rifle.x;
        const float rifleSeconds=InfantrySecondsToReady(rifle);
        assert(rifleSeconds>7.0f && rifleSeconds<8.0f);
        assert(!InfantryReady(rifle));
        AdvanceInfantry(rifle,rifleSeconds);
        assert(InfantryReady(rifle));
        assert(std::abs(rifle.z+1.34f)<.00002f);
        const float stoppedZ=rifle.z;
        AdvanceInfantry(rifle,1.0f);
        assert(rifle.z==stoppedZ);
        assert(InfantrySecondsToReady(rifle)<.0001f);

        Infantry flank{};
        flank.active=true;flank.x=.4f;flank.z=-2.28f;
        flank.tactic=InfantryTactic::FlankLeft;
        flank.lane_x=InfantryLaneX(flank.x,flank.tactic);
        assert(flank.lane_x<flank.x);
        const float flankArrival=InfantrySecondsToReady(flank);
        assert(flankArrival>0.0f);
        const float oldX=flank.x;
        AdvanceInfantry(flank,1.0f);
        assert(flank.x<oldX && flank.z>-2.28f);
        assert(!InfantryReady(flank));
        AdvanceInfantry(flank,100.0f);
        assert(InfantryReady(flank));
        assert(flank.x==flank.lane_x);
        assert(InfantryMarchStride(flank)==0.0f);
        assert(InfantrySecondsToReady(flank)<.0001f);
        Infantry right=flank;
        right.tactic=InfantryTactic::FlankRight;
        right.x=0.0f;right.z=-2.28f;right.lane_x=.85f;
        const float arrivalBefore=InfantrySecondsToReady(right);
        AdvanceInfantry(right,.05f);
        assert(InfantrySecondsToReady(right)<arrivalBefore);
        assert(right.x>0.0f);
        AdvanceInfantry(right,100.0f);
        assert(InfantryReady(right));
        assert(right.x==right.lane_x);

        Infantry support{};
        support.active=true;support.x=-.1f;support.z=-2.38f;
        support.tactic=InfantryTactic::Support;
        support.lane_x=support.x;
        assert(InfantrySprite(support)==SpriteId::InfantryStrideA ||
               InfantrySprite(support)==SpriteId::InfantryStrideB);
        assert(InfantrySprite(rifle)==SpriteId::InfantryRifleReady);
        assert(InfantrySprite(rifle,true)==SpriteId::InfantryRifleFire);
        assert(InfantrySprite(flank)==SpriteId::InfantryFlankReady);
        assert(InfantrySprite(flank,true)==SpriteId::InfantryFlankFire);
        assert(InfantrySprite(support,true)==SpriteId::InfantryStrideA ||
               InfantrySprite(support,true)==SpriteId::InfantryStrideB);
        assert(!InfantryFiringPose(rifle,0u,0));
        assert(!InfantryFiringPose(rifle,1u,1));
        assert(InfantryFiringPose(rifle,1u,0));
        assert(!InfantryFiringPose(rifle,~uint64_t{0},Campaign::kMaxInfantry));
        assert(InfantrySprite(right,true)==SpriteId::InfantryFlankFire);
        const float supportTime=InfantrySecondsToReady(support);
        assert(supportTime>5.0f && supportTime<8.0f);
        AdvanceInfantry(support,100.0f);
        assert(InfantryReady(support));
        assert(std::abs(support.z+1.68f)<.00002f);
        assert(InfantryMarchStride(support)==0.0f);
        assert(InfantrySprite(support)==SpriteId::InfantrySupport);
        assert(InfantrySprite(support,true)==SpriteId::InfantrySupportFire);
        assert(InfantrySprite(rifle)==SpriteId::InfantryRifleReady);
        // An advancing soldier cannot show a firearm muzzle flash even
        // if a stale bitmap bit was accidentally carried over.
        Infantry approaching=rifle;
        approaching.z=-2.05f;
        assert(InfantrySprite(approaching,true)==
               SpriteId::InfantryStrideA ||
               InfantrySprite(approaching,true)==
               SpriteId::InfantryStrideB);
        assert(!InfantryFiringPose(approaching,1u,0));
        Infantry malformed=support;
        malformed.x=std::numeric_limits<float>::quiet_NaN();
        assert(InfantrySecondsToReady(malformed)<0.0f);
        assert(!InfantryReady(malformed));
        AdvanceInfantry(malformed,.05f);
        malformed=support;
        malformed.active=false;
        assert(InfantrySecondsToReady(malformed)<0.0f);
        assert(!InfantryReady(malformed));
        malformed=support;
        malformed.tactic=InfantryTactic::FlankLeft;
        malformed.lane_x=std::numeric_limits<float>::infinity();
        assert(InfantrySecondsToReady(malformed)<0.0f);
        assert(!InfantryReady(malformed));

        // The extra support silhouette occupies one already-empty cell;
        // no atlas expansion or new GL texture.
        assert(sizeof(SpritePixels)==128u*1024u);
        const SpritePixels art=BuildSpriteAtlas();
        const int supportCell=static_cast<int>(SpriteId::InfantrySupport);
        const int col=supportCell%4,row=supportCell/4;
        uint64_t supportHash=14695981039346656037ull;
        uint64_t rifleHash=14695981039346656037ull;
        int ink=0;
        for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
            const size_t supportOffset=static_cast<size_t>(
                ((row*32+y)*kSpriteAtlasWidth+col*32+x)*4);
            const size_t rifleOffset=static_cast<size_t>(
                (y*kSpriteAtlasWidth+x)*4);
            for(int k=0;k<4;++k) {
                supportHash=(supportHash^art[supportOffset+k])*1099511628211ull;
                rifleHash=(rifleHash^art[rifleOffset+k])*1099511628211ull;
            }
            if(art[supportOffset+3]==255)++ink;
            else assert(art[supportOffset+3]==0);
        }
        assert(ink>25 && supportHash!=rifleHash);
        const std::array<SpriteId,6> poses={{
            SpriteId::InfantryRifleReady,SpriteId::InfantryRifleFire,
            SpriteId::InfantryFlankReady,SpriteId::InfantryFlankFire,
            SpriteId::InfantrySupportFire,SpriteId::InfantryPinned
        }};
        std::array<uint64_t,6> hashes{};
        for(size_t i=0;i<poses.size();++i) {
            const int cell=static_cast<int>(poses[i]);
            const int px=(cell%4)*32,py=(cell/4)*32;
            uint64_t h=14695981039346656037ull;
            int visible=0,transparent=0;
            for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
                const size_t offset=static_cast<size_t>(
                    ((py+y)*kSpriteAtlasWidth+px+x)*4);
                for(int ch=0;ch<4;++ch) {
                    h^=art[offset+ch];
                    h*=1099511628211ull;
                }
                if(art[offset+3]==255)++visible;
                else {assert(art[offset+3]==0);++transparent;}
            }
            assert(visible>25 && transparent>50);
            hashes[i]=h;
            std::vector<float> card;
            assert(AppendSprite(card,poses[i],-.5f,-.67f,-1.34f,
                                .40f,.70f));
            assert(card.size()==30);
            for(size_t n=0;n<card.size();n+=5) {
                assert(card[n+3]>=static_cast<float>(px)/kSpriteAtlasWidth);
                assert(card[n+3]<=static_cast<float>(px+32)/kSpriteAtlasWidth);
                assert(card[n+4]>=static_cast<float>(py)/kSpriteAtlasHeight);
                assert(card[n+4]<=static_cast<float>(py+32)/kSpriteAtlasHeight);
            }
        }
        for(size_t a=0;a<hashes.size();++a)
            for(size_t b=a+1;b<hashes.size();++b)
                assert(hashes[a]!=hashes[b]);
        assert(sizeof(SpritePixels)==128u*1024u);
    }

    // A wreck is produced only by an authoritative death transition.
    // It survives enemy deactivation visually, without becoming a target.
    {
        WreckEffects wrecks;
        Enemy before{};before.active=true;
        before.type=UnitType::LandingCraft;
        before.x=1.25f;before.z=-5.5f;before.hp=30;
        Enemy after=before;
        after.hp=10;
        assert(!wrecks.RecordDestroyed(before,after)); // damaged, alive
        after.active=false;after.hp=0;
        assert(wrecks.RecordDestroyed(before,after));
        assert(!wrecks.RecordDestroyed(after,after));
        assert(wrecks.pulses()[0].remaining==WreckEffects::kDuration);
        assert(wrecks.pulses()[0].x==before.x);
        assert(wrecks.pulses()[0].z==before.z);
        assert(wrecks.pulses()[0].type==UnitType::LandingCraft);
        assert(!wrecks.RecordDestroyed(before,before));
        after.hp=10; // transport reached shore / retreated, not killed
        assert(!wrecks.RecordDestroyed(before,after));
        after.hp=0;before.active=false;
        assert(!wrecks.RecordDestroyed(before,after));
        before.active=true;before.hp=30;
        before.x=std::numeric_limits<float>::quiet_NaN();
        assert(!wrecks.RecordDestroyed(before,after));
        before.x=1.25f;before.z=std::numeric_limits<float>::infinity();
        assert(!wrecks.RecordDestroyed(before,after));

        int triangles=0;
        auto countTriangles=[&](auto... values) {
            static_assert(sizeof...(values)==12,"xyz + rgb triangle contract");
            const float components[]={values...};
            for(float component:components)assert(std::isfinite(component));
            ++triangles;
        };
        EmitWreckFragments(wrecks.pulses()[0],countTriangles);
        assert(triangles==9);
        WreckPulse airplane{-.7f,-6.1f,WreckEffects::kDuration,
                            UnitType::Aircraft,2};
        triangles=0;
        EmitWreckFragments(airplane,countTriangles);
        assert(triangles==9);
        WreckPulse invalid=airplane;
        invalid.remaining=std::numeric_limits<float>::quiet_NaN();
        triangles=0;
        EmitWreckFragments(invalid,countTriangles);
        assert(triangles==0);
        invalid=airplane;invalid.type=static_cast<UnitType>(255);
        EmitWreckFragments(invalid,countTriangles);
        assert(triangles==0);
        wrecks.Tick(std::numeric_limits<float>::quiet_NaN());
        assert(wrecks.pulses()[0].remaining==WreckEffects::kDuration);
        wrecks.Tick(.60f);
        assert(wrecks.pulses()[0].remaining<WreckEffects::kDuration);
        assert(wrecks.pulses()[0].remaining>0.0f);
        triangles=0;EmitWreckFragments(wrecks.pulses()[0],countTriangles);
        assert(triangles==9);
        wrecks.Tick(2.0f);
        triangles=0;EmitWreckFragments(wrecks.pulses()[0],countTriangles);
        assert(triangles==0);

        // Heavy wrecks have an extended, separately sinking plated hull.
        // Both old vessels and aircraft retain their original 1.8s pulse.
        WreckEffects heavy_wreck;
        Enemy heavy_before{};heavy_before.active=true;
        heavy_before.type=UnitType::ArmoredTransport;
        heavy_before.hp=320;heavy_before.x=.3f;heavy_before.z=-4.1f;
        Enemy heavy_after=heavy_before;
        heavy_after.active=false;heavy_after.hp=-1;
        assert(heavy_wreck.RecordDestroyed(heavy_before,heavy_after));
        assert(heavy_wreck.pulses()[0].remaining==
               WreckEffects::kHeavyDuration);
        triangles=0;
        EmitWreckFragments(heavy_wreck.pulses()[0],countTriangles);
        assert(triangles==15); // shards + deck + foam + smoke
        heavy_wreck.Tick(2.30f);
        triangles=0;
        EmitWreckFragments(heavy_wreck.pulses()[0],countTriangles);
        assert(triangles==12); // armored deck still sinking
        heavy_wreck.Tick(.80f);
        triangles=0;
        EmitWreckFragments(heavy_wreck.pulses()[0],countTriangles);
        assert(triangles==0);
        heavy_after.hp=320; // survived beaching / disembarked
        assert(!heavy_wreck.RecordDestroyed(heavy_before,heavy_after));

        WreckEffects batch;
        std::array<Enemy,Campaign::kMaxEnemies> start{},finish{};
        for(size_t i=0;i<10;++i) {
            start[i].active=true;start[i].hp=20;
            start[i].type=i%3==0?UnitType::Aircraft:UnitType::Rowboat;
            start[i].x=static_cast<float>(i)*.24f;
            start[i].z=-8.0f;
            finish[i]=start[i];finish[i].active=false;finish[i].hp=0;
        }
        // Fixed capacity, never overwrites a still-fading wreck in
        // mass simultaneous artillery/airstrike destruction.
        assert(batch.RecordTransitions(start,finish)==WreckEffects::kCapacity);
        assert(batch.RecordTransitions(start,finish)==0);
        for(const WreckPulse& pulse:batch.pulses())
            assert(pulse.remaining==WreckEffects::kDuration);
        batch.Tick(2.0f);
        assert(batch.RecordTransitions(start,finish)==WreckEffects::kCapacity);
        batch.Clear();
        for(const WreckPulse& pulse:batch.pulses())
            assert(pulse.remaining==0.0f);
    }

    // A precise target reticle is informational only: all ship and
    // soldier tests below leave hit spheres and Campaign state untouched.
    {
        assert(EnemyStartingHp(UnitType::Rowboat)==20);
        assert(EnemyStartingHp(UnitType::LandingCraft)==85);
        assert(EnemyStartingHp(UnitType::ArmoredTransport)==320);
        assert(EnemyStartingHp(UnitType::Gunboat)==125);
        assert(EnemyStartingHp(UnitType::Aircraft)==100);
        assert(EnemyStartingHp(static_cast<UnitType>(255))==0);
        assert(SurfaceBaseHp(UnitType::Rowboat)==20);
        assert(SurfaceBaseHp(UnitType::LandingCraft)==85);
        assert(SurfaceBaseHp(UnitType::ArmoredTransport)==320);
        assert(SurfaceBaseHp(UnitType::Gunboat)==125);
        assert(SurfaceBaseHp(UnitType::Aircraft)==0);
        assert(SurfaceBaseHp(static_cast<UnitType>(255))==0);
        Enemy e{};
        e.active=true;e.type=UnitType::Rowboat;
        e.x=1.20f;e.z=-5.0f;e.hp=15;
        const auto row=InspectSurfaceEnemy(e,WeaponKind::MachineGun);
        assert(row.visible && !row.infantry && row.vulnerable);
        assert(row.max_hp==20 && row.hp==15);
        assert(std::abs(SurfaceHullFraction(row)-.75f)<.00001f);
        assert(row.x==e.x && row.z==e.z);
        assert(row.half_width>.3f && row.half_width<.80f);
        assert(!InspectSurfaceEnemy(e,WeaponKind::Flak).infantry);
        e.type=UnitType::LandingCraft;e.hp=42;
        auto craft=InspectSurfaceEnemy(e,WeaponKind::DirectCannon);
        assert(craft.visible && craft.max_hp==85 && craft.hp==42);
        assert(SurfaceHullFraction(craft)>.49f &&
               SurfaceHullFraction(craft)<.50f);
        e.type=UnitType::Gunboat;e.hp=115;
        const auto gunboat=InspectSurfaceEnemy(e,WeaponKind::HeavyGun);
        assert(gunboat.visible && gunboat.max_hp==125);
        assert(SurfaceHullFraction(gunboat)>.90f);
        e.hp=500;
        assert(InspectSurfaceEnemy(e,WeaponKind::MissileBattery).hp==125);
        assert(SurfaceHullFraction(InspectSurfaceEnemy(
                   e,WeaponKind::MissileBattery))==1.0f);
        e.hp=0;
        assert(!InspectSurfaceEnemy(e,WeaponKind::MachineGun).visible);
        e.hp=20;e.active=false;
        assert(!InspectSurfaceEnemy(e,WeaponKind::MachineGun).visible);
        e.active=true;e.type=UnitType::Aircraft;
        assert(!InspectSurfaceEnemy(e,WeaponKind::Flak).visible);
        e.type=UnitType::Rowboat;
        e.x=std::numeric_limits<float>::quiet_NaN();
        assert(!InspectSurfaceEnemy(e,WeaponKind::Flak).visible);
        e.x=0;e.z=std::numeric_limits<float>::infinity();
        assert(!InspectSurfaceEnemy(e,WeaponKind::Flak).visible);

        Infantry troop{};
        troop.active=true;troop.x=.22f;troop.z=-1.7f;
        const auto rifle=InspectSurfaceInfantry(troop,WeaponKind::MachineGun);
        const auto flak=InspectSurfaceInfantry(troop,WeaponKind::Flak);
        assert(rifle.visible && rifle.infantry && rifle.vulnerable);
        assert(flak.visible && flak.infantry && !flak.vulnerable);
        assert(rifle.x==troop.x && rifle.z==troop.z);
        assert(SurfaceHullFraction(rifle)==0.0f);
        troop.active=false;
        assert(!InspectSurfaceInfantry(troop,WeaponKind::MachineGun).visible);
        troop.active=true;
        troop.z=std::numeric_limits<float>::quiet_NaN();
        assert(!InspectSurfaceInfantry(troop,WeaponKind::HeavyGun).visible);
        Campaign game(Difficulty::Regular,1);
        assert(!InspectSurfaceAim(game,0).visible);
        assert(!InspectSurfaceAim(game,-1).visible);
        assert(!InspectSurfaceAim(game,255).visible);
        assert(game.Start());
        assert(!InspectSurfaceAim(game,0).visible);
        assert(game.BeginWave());
        assert(!InspectSurfaceAim(game,0).visible); // not yet spawned
        game.Tick(.10f);
        int activeSlot=-1;
        for(size_t i=0;i<game.enemies().size();++i)
            if(game.enemies()[i].active) {
                activeSlot=static_cast<int>(i);break;
            }
        assert(activeSlot>=0);
        const auto selected=InspectSurfaceAim(game,activeSlot);
        assert(selected.visible && !selected.infantry);
        assert(selected.x==game.enemies()[static_cast<size_t>(activeSlot)].x);
        assert(selected.z==game.enemies()[static_cast<size_t>(activeSlot)].z);
        assert(selected.hp==game.enemies()[static_cast<size_t>(activeSlot)].hp);
        assert(!InspectSurfaceAim(game,-1).visible);
        assert(!InspectSurfaceAim(game,
            static_cast<int>(Campaign::kMaxEnemies+
                             Campaign::kMaxInfantry)).visible);
        assert(SurfaceHullFraction(SurfaceAimFeedback{})==0.0f);
    }

    // All three coastlines have a closed 360-degree layered sky in both
    // day/night modes. The renderer consumes these triangles only once per
    // shared stereo scene build; the recipe does not own gameplay state.
    {
        auto sky=[](uint8_t location,bool night) {
            std::vector<float> mesh;
            EmitSkyPanorama(location,night,[&](float ax,float ay,float az,
                float bx,float by,float bz,float cx,float cy,float cz,
                float r,float g,float b) {
                const float values[]={ax,ay,az,bx,by,bz,cx,cy,cz};
                for(float value:values)mesh.push_back(value);
                // Reject invalid or overbright source art.
                assert(std::isfinite(r) && std::isfinite(g) &&
                       std::isfinite(b));
                assert(r>=0 && r<=1 && g>=0 && g<=1 && b>=0 && b<=1);
            });
            return mesh;
        };
        const SkyPoint front=SkyAt(0,kSkyRadius,0,0);
        const SkyPoint back=SkyAt(kSkyPi,kSkyRadius,0,0);
        assert(std::abs(front.x)<.0001f && front.z< -25.5f);
        assert(std::abs(back.x)<.0001f && back.z>25.5f);
        for(uint8_t location=0;location<Campaign::kLocations;++location) {
            for(bool night : {false,true}) {
                const auto mesh=sky(location,night);
                const int plumes=location==1?3:location==2?1:0;
                const size_t expected=static_cast<size_t>(
                    (kSkySegments*kSkyBands*2+kSkySegments+
                     (night?8:12)*3+10+
                     (night?48:0)+plumes*2)*9);
                assert(mesh.size()==expected);
                assert(mesh==sky(location,night));
                bool left=false,right=false,frontSeen=false,backSeen=false;
                size_t apexVertices=0;
                for(size_t i=0;i<mesh.size();i+=3) {
                    const float x=mesh[i],y=mesh[i+1],z=mesh[i+2];
                    assert(std::isfinite(x) && std::isfinite(y) &&
                           std::isfinite(z));
                    assert(y>=-4.001f && y<=40.001f);
                    const float radius=std::sqrt(x*x+z*z);
                    if(std::abs(y-40.0f)<.001f) {
                        // The closed zenith cone uses one exact center
                        // vertex per sky sector, above the prior 17m ring.
                        assert(radius<.001f);
                        ++apexVertices;
                    } else {
                        assert(y<=17.001f);
                        assert(radius>24.0f && radius<27.5f);
                    }
                    left|=x< -20.0f;right|=x>20.0f;
                    frontSeen|=z< -20.0f;backSeen|=z>20.0f;
                }
                assert(apexVertices==static_cast<size_t>(kSkySegments));
                assert(left && right && frontSeen && backSeen);
            }
        }
        assert(sky(255,false).empty());
        assert(sky(255,true).empty());
        assert(sky(0,false)!=sky(0,true));
        assert(sky(0,false)!=sky(1,false));
        assert(sky(1,false)!=sky(2,false));
    }
    // Every authored RGBA sprite has opaque military-colored pixels and
    // transparent edges. World cards are constant-six-vertex impostors.
    {
        const SpritePixels pixels=BuildSpriteAtlas();
        for(int id=0;id<static_cast<int>(SpriteId::Count);++id) {
            int visible=0,transparent=0;
            const int ox=(id%4)*kSpriteTile,oy=(id/4)*kSpriteTile;
            for(int y=0;y<kSpriteTile;++y)
                for(int x=0;x<kSpriteTile;++x) {
                    const size_t a=static_cast<size_t>(
                        ((oy+y)*kSpriteAtlasWidth+ox+x)*4+3);
                    if(pixels[a]==255)++visible;
                    else {assert(pixels[a]==0);++transparent;}
                }
            assert(visible>20 && transparent>50);
        }
        std::vector<float> sprites;
        for(int id=0;id<static_cast<int>(SpriteId::Count);++id)
            assert(AppendSprite(sprites,static_cast<SpriteId>(id),
                0.0f,-.6f,-5.0f,1.0f,.75f));
        assert(sprites.size()==static_cast<size_t>(
            static_cast<int>(SpriteId::Count)*30));
        for(size_t i=0;i<sprites.size();i+=5) {
            assert(std::isfinite(sprites[i]));
            assert(std::isfinite(sprites[i+1]));
            assert(std::isfinite(sprites[i+2]));
            assert(sprites[i+3]>=0.0f && sprites[i+3]<=1.0f);
            assert(sprites[i+4]>=0.0f && sprites[i+4]<=1.0f);
        }
        // These are five distinct visual identities, mapped directly from
        // the same saved weapon enum that owns combat ballistics and selection.
        const std::array<SpriteId,kWeaponCount> expected={{
            SpriteId::MachineGun,SpriteId::HeavyGun,SpriteId::FlakGun,
            SpriteId::MissilePod,SpriteId::DirectCannon}};
        std::array<uint64_t,kWeaponCount> signatures{};
        for(uint8_t id=0;id<kWeaponCount;++id) {
            const SpriteId visual=WeaponSprite(static_cast<WeaponKind>(id));
            assert(visual==expected[id]);
            const int slot=static_cast<int>(visual);
            const int ox=(slot%4)*kSpriteTile,oy=(slot/4)*kSpriteTile;
            uint64_t signature=14695981039346656037ull;
            int opaque=0,transparent=0;
            for(int y=0;y<kSpriteTile;++y)
                for(int x=0;x<kSpriteTile;++x) {
                    const size_t pixel=static_cast<size_t>(
                        ((oy+y)*kSpriteAtlasWidth+ox+x)*4);
                    for(int ch=0;ch<4;++ch) {
                        signature^=pixels[pixel+ch];
                        signature*=1099511628211ull;
                    }
                    if(pixels[pixel+3]==255)++opaque;
                    else {assert(pixels[pixel+3]==0);++transparent;}
                }
            assert(opaque>40 && transparent>100);
            signatures[id]=signature;
            std::vector<float> card;
            assert(AppendSprite(card,visual,0,-.77f,-.53f,1.10f,.70f));
            assert(card.size()==30);
            for(size_t vertex=0;vertex<card.size();vertex+=5) {
                const float u=card[vertex+3],v=card[vertex+4];
                assert(u>=static_cast<float>(ox)/kSpriteAtlasWidth);
                assert(u<=static_cast<float>(ox+32)/kSpriteAtlasWidth);
                assert(v>=static_cast<float>(oy)/kSpriteAtlasHeight);
                assert(v<=static_cast<float>(oy+32)/kSpriteAtlasHeight);
            }
        }
        for(size_t a=0;a<signatures.size();++a)
            for(size_t b=a+1;b<signatures.size();++b)
                assert(signatures[a]!=signatures[b]);
        // The remaining atlas cells provide four different operational
        // support identities with no second texture or new gameplay control.
        const std::array<SpriteId,4> supportIds={{
            SpriteId::ArtillerySupport,SpriteId::AirstrikeSupport,
            SpriteId::SamSupport,SpriteId::RepairSupport}};
        assert(static_cast<int>(SpriteId::Count)==31);
        assert(sizeof(SpritePixels)==128u*1024u);
        std::array<uint64_t,4> supportHashes{};
        for(size_t id=0;id<supportIds.size();++id) {
            const int slot=static_cast<int>(supportIds[id]);
            const int ox=(slot%4)*kSpriteTile,oy=(slot/4)*kSpriteTile;
            uint64_t signature=14695981039346656037ull;
            int ink=0,clear=0;
            for(int y=0;y<kSpriteTile;++y)
                for(int x=0;x<kSpriteTile;++x) {
                    const size_t offset=static_cast<size_t>(
                        ((oy+y)*kSpriteAtlasWidth+ox+x)*4);
                    for(int ch=0;ch<4;++ch) {
                        signature^=pixels[offset+ch];
                        signature*=1099511628211ull;
                    }
                    if(pixels[offset+3]==255)++ink;
                    else { assert(pixels[offset+3]==0);++clear; }
                }
            assert(ink>20 && clear>50);
            supportHashes[id]=signature;
            std::vector<float> card;
            assert(AppendSprite(card,supportIds[id],-2.0f,.97f,-1.76f,
                                .26f,.26f));
            assert(card.size()==30);
            for(size_t i=0;i<card.size();i+=5) {
                assert(card[i+3]>=static_cast<float>(ox)/kSpriteAtlasWidth);
                assert(card[i+3]<=static_cast<float>(ox+32)/kSpriteAtlasWidth);
                assert(card[i+4]>=static_cast<float>(oy)/kSpriteAtlasHeight);
                assert(card[i+4]<=static_cast<float>(oy+32)/kSpriteAtlasHeight);
            }
        }
        for(size_t i=0;i<supportHashes.size();++i)
            for(size_t j=i+1;j<supportHashes.size();++j)
                assert(supportHashes[i]!=supportHashes[j]);
        assert(ShopSupportSprite(false,4)==SpriteId::ArtillerySupport);
        assert(ShopSupportSprite(false,5)==SpriteId::AirstrikeSupport);
        assert(ShopSupportSprite(false,7)==SpriteId::RepairSupport);
        assert(ShopSupportSprite(true,5)==SpriteId::SamSupport);
        for(uint8_t item=0;item<8;++item) {
            if(item!=5)assert(ShopSupportSprite(true,item)==SpriteId::Count);
            if(item!=4 && item!=5 && item!=7)
                assert(ShopSupportSprite(false,item)==SpriteId::Count);
        }
        assert(ShopSupportSprite(true,255)==SpriteId::Count);
        assert(ShopSupportSprite(false,255)==SpriteId::Count);
        // Eight original animation tiles extend the same atlas without a
        // second texture or draw call. Each variant has independently
        // rendered pixel content and legal per-cell UV coordinates.
        const std::array<SpriteId,8> animated={{
            SpriteId::RowboatOars,SpriteId::LandingRampHalf,
            SpriteId::LandingRampOpen,SpriteId::GunboatSalvo,
            SpriteId::StraferBankLeft,SpriteId::StraferBankRight,
            SpriteId::BomberBankLeft,SpriteId::BomberBankRight
        }};
        std::array<uint64_t,8> variantHashes{};
        for(size_t id=0;id<animated.size();++id) {
            const int cell=static_cast<int>(animated[id]);
            const int col=cell%4,row=cell/4;
            int opaque=0,clear=0;
            uint64_t hash=14695981039346656037ull;
            for(int y=0;y<kSpriteTile;++y)
                for(int x=0;x<kSpriteTile;++x) {
                    const size_t pixel=static_cast<size_t>(
                        ((row*32+y)*kSpriteAtlasWidth+col*32+x)*4);
                    for(int ch=0;ch<4;++ch) {
                        hash^=pixels[pixel+ch];
                        hash*=1099511628211ull;
                    }
                    if(pixels[pixel+3]==255)++opaque;
                    else { assert(pixels[pixel+3]==0);++clear; }
                }
            assert(opaque>20 && clear>50);
            variantHashes[id]=hash;
            std::vector<float> card;
            assert(AppendSprite(card,animated[id],.5f,-.6f,-6.0f,
                                1.0f,.65f,.15f));
            assert(card.size()==30);
            for(size_t i=0;i<card.size();i+=5) {
                assert(card[i+3]>=static_cast<float>(col*32)/kSpriteAtlasWidth);
                assert(card[i+3]<=static_cast<float>(col*32+32)/kSpriteAtlasWidth);
                assert(card[i+4]>=static_cast<float>(row*32)/kSpriteAtlasHeight);
                assert(card[i+4]<=static_cast<float>(row*32+32)/kSpriteAtlasHeight);
            }
        }
        for(size_t a=0;a<variantHashes.size();++a)
            for(size_t b=a+1;b<variantHashes.size();++b)
                assert(variantHashes[a]!=variantHashes[b]);

        // Pose transitions follow real movement, landing and attack
        // state; replaying a given state must never depend on render time.
        Enemy e{};
        e.active=true;e.type=UnitType::Rowboat;e.z=-10.9f;
        assert(EnemySprite(e)==SpriteId::Rowboat);
        e.z=-10.6f;
        assert(EnemySprite(e)==SpriteId::RowboatOars);
        assert(EnemySprite(e)==EnemySprite(e));
        e.type=UnitType::LandingCraft;e.z=-3.8f;
        assert(EnemySprite(e)==SpriteId::LandingCraft);
        e.z=-3.0f;
        assert(EnemySprite(e)==SpriteId::LandingRampHalf);
        e.z=-2.4f;
        assert(EnemySprite(e)==SpriteId::LandingRampOpen);
        e.type=UnitType::Gunboat;e.attack_timer=2.0f;
        assert(EnemySprite(e)==SpriteId::Gunboat);
        e.attack_timer=2.65f;
        assert(EnemySprite(e)==SpriteId::GunboatSalvo);
        e.attack_timer=std::numeric_limits<float>::quiet_NaN();
        assert(EnemySprite(e)==SpriteId::Gunboat);
        e.type=UnitType::Aircraft;e.aircraft_pattern=AircraftPattern::Strafer;
        e.bank=0.0f;
        assert(EnemySprite(e)==SpriteId::Strafer);
        e.bank=-.15f;
        assert(EnemySprite(e)==SpriteId::StraferBankLeft);
        e.bank=.15f;
        assert(EnemySprite(e)==SpriteId::StraferBankRight);
        e.aircraft_pattern=AircraftPattern::Bomber;
        e.bank=0.0f;
        assert(EnemySprite(e)==SpriteId::Bomber);
        e.bank=-.023f;
        assert(EnemySprite(e)==SpriteId::BomberBankLeft);
        e.bank=.023f;
        assert(EnemySprite(e)==SpriteId::BomberBankRight);
        e.bank=std::numeric_limits<float>::quiet_NaN();
        assert(EnemySprite(e)==SpriteId::Bomber);
        e.type=static_cast<UnitType>(255);
        assert(EnemySprite(e)==SpriteId::Count);
        // The remaining atlas cell stays completely transparent.
        for(int cell=31;cell<32;++cell) {
            const int col=cell%4,row=cell/4;
            for(int y=0;y<32;++y)
                for(int x=0;x<32;++x) {
                    const size_t alpha=static_cast<size_t>(
                        ((row*32+y)*kSpriteAtlasWidth+col*32+x)*4+3);
                    assert(pixels[alpha]==0);
                }
        }
        // Unknown weapon enum values never index beyond the atlas.
        assert(WeaponSprite(static_cast<WeaponKind>(255))==
            SpriteId::MachineGun);
        const size_t before=sprites.size();
        assert(!AppendSprite(sprites,SpriteId::Count,0,0,0,1,1));
        assert(!AppendSprite(sprites,SpriteId::InfantryStrideA,
            0,0,0,0,1));
        assert(!AppendSprite(sprites,SpriteId::Bomber,
            0,0,0,1,std::numeric_limits<float>::quiet_NaN()));
        assert(sprites.size()==before);
        assert(AppendSprite(sprites,SpriteId::Bomber,
            0,1.6f,-9.0f,2.4f,1.3f,.22f));
        assert(sprites.size()==before+30);
    }
    // Quick Menu takes away the pointer: enemy waves, shop and held buttons
    // cannot act again until a valid pointer returns and buttons release.
    {
        PointerFocusGate focus;
        bool primary=true, secondary=true;
        assert(focus.Filter(true,primary,secondary) && primary && secondary);
        assert(!focus.Filter(false,primary,secondary));
        assert(!primary && !secondary);
        primary=secondary=true;
        assert(focus.Filter(true,primary,secondary));
        assert(!primary && !secondary);
        primary=false;secondary=true;
        assert(focus.Filter(true,primary,secondary));
        assert(!primary && !secondary);
        primary=secondary=true;
        assert(focus.Filter(true,primary,secondary));
        assert(primary && !secondary);
        primary=secondary=false;
        assert(focus.Filter(true,primary,secondary));
        primary=secondary=true;
        assert(focus.Filter(true,primary,secondary));
        assert(primary && secondary);
        focus.RequireRelease(); // Android lifecycle pause
        primary=secondary=true;
        assert(focus.Filter(true,primary,secondary));
        assert(!primary && !secondary);
        primary=secondary=false;
        assert(focus.Filter(true,primary,secondary));
        primary=secondary=true;
        assert(focus.Filter(true,primary,secondary));
        assert(primary && secondary);
        assert(!focus.Filter(false,primary,secondary));
        primary=secondary=false;
        assert(!focus.Filter(false,primary,secondary));
        assert(focus.Filter(true,primary,secondary));
        primary=secondary=true;
        assert(focus.Filter(true,primary,secondary));
        assert(primary && secondary);
    }
    // Pressing FIRE TO RESTART must atomically invalidate the previous
    // inter-wave save. This is a game-private zero-byte ABI-v1 slot, not
    // a change to campaign serialization or a deletion API.
    {
        static unsigned clear_writes=0;
        ReverieNativeHostV1 host{};
        host.struct_size=sizeof(host);
        host.abi_version=REVERIE_NATIVE_MODULE_ABI_VERSION;
        host.read_save=[](const char*,void*,uint32_t,uint32_t*)->int32_t {
            return REVERIE_NATIVE_SAVE_NOT_FOUND;
        };
        host.write_save=[](const char* slot,const void* data,uint32_t size)->int32_t {
            assert(std::strcmp(slot,CheckpointSlot(0u))==0);
            assert(data==nullptr && size==0u);
            ++clear_writes;
            return REVERIE_NATIVE_SAVE_OK;
        };
        assert(ClearCheckpoint(&host,0u));
        assert(clear_writes==1);
        assert(CheckpointSlot(1u)!=nullptr && CheckpointSlot(2u)!=nullptr);
        assert(std::strcmp(CheckpointSlot(0u),CheckpointSlot(1u))!=0);
        assert(CheckpointSlot(3u)==nullptr);
        assert(!ClearCheckpoint(&host,3u));
        assert(IsClearedCheckpoint(REVERIE_NATIVE_SAVE_OK,0u));
        assert(!IsClearedCheckpoint(REVERIE_NATIVE_SAVE_NOT_FOUND,0u));
        assert(!IsClearedCheckpoint(REVERIE_NATIVE_SAVE_ERROR,0u));
        assert(!IsClearedCheckpoint(REVERIE_NATIVE_SAVE_OK,80u));
        host.write_save=[](const char*,const void*,uint32_t)->int32_t {
            return REVERIE_NATIVE_SAVE_ERROR;
        };
        assert(!ClearCheckpoint(&host));
        host.abi_version=REVERIE_NATIVE_MODULE_ABI_VERSION+1u;
        assert(!ClearCheckpoint(&host));
        host.abi_version=REVERIE_NATIVE_MODULE_ABI_VERSION;
        host.struct_size=REVERIE_NATIVE_HOST_V1_SAVE_MIN_SIZE-1u;
        assert(!ClearCheckpoint(&host));
        host.struct_size=sizeof(host);
        host.read_save=nullptr;
        assert(!ClearCheckpoint(&host));
        assert(!ClearCheckpoint(nullptr));
    }
    // The three battlefields render real shore, not an endless sea under
    // walking infantry. Validate budget, location identity and bounds.
    for (uint8_t location=0;location<Campaign::kLocations;location++) {
        int boxes=0, triangles=0;
        auto box=[&](float x,float y,float z,float w,float h,float d,
                     float r,float g,float b) {
            ++boxes;
            assert(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z));
            assert(w>0&&h>0&&d>0);
            // Only scenery extends past the combat lane: piers, side
            // mountains and seawalls stay within the 20-unit panorama.
            assert(std::abs(x)<=20.001f&&z>=-2.50f&&z<=1.50f);
            assert(r>=0&&g>=0&&b>=0&&r<=1&&g<=1&&b<=1);
        };
        auto triangle=[&](float x0,float y0,float z0,
                          float x1,float y1,float z1,
                          float x2,float y2,float z2,
                          float r,float g,float b) {
            ++triangles;
            assert(std::isfinite(x0)&&std::isfinite(y0)&&std::isfinite(z0));
            assert(std::isfinite(x1)&&std::isfinite(y1)&&std::isfinite(z1));
            assert(std::isfinite(x2)&&std::isfinite(y2)&&std::isfinite(z2));
            // The first 20 triangles are the gameplay beach and surf.
            // The following four intentionally widen into each location's
            // scenic shoulders, outside the traversable infantry lane.
            const bool scenicShoulder=triangles>20&&triangles<=24;
            const bool flankExtension=location!=1u &&
                triangles>24&&triangles<=28;
            const float reach=flankExtension?20.01f:
                scenicShoulder?(location==0?5.35f:
                    location==1?4.85f:5.80f):4.0f;
            const float zNear=flankExtension?1.451f:
                scenicShoulder?-.71f:-.80f;
            // Lateral slopes begin 5 cm farther seaward than the former
            // center-only beach; no other triangle may exceed that bound.
            const float zFar=flankExtension?-2.301f:-2.25f;
            assert(x0>=-reach-.001f&&x0<=reach+.001f);
            assert(x1>=-reach-.001f&&x1<=reach+.001f);
            assert(x2>=-reach-.001f&&x2<=reach+.001f);
            assert(y0>=-1.10f&&y1>=-1.10f&&y2>=-1.10f);
            assert(z0>=zFar&&z0<=zNear);
            assert(z1>=zFar&&z1<=zNear);
            assert(z2>=zFar&&z2<=zNear);
            assert(r>=0&&g>=0&&b>=0&&r<=1&&g<=1&&b<=1);
        };
        EmitCoastalTerrain(location,false,box,triangle);
        const int expectedTriangles[]={46,29,36};
        const int expectedBoxes[]={20,46,26};
        assert(triangles==expectedTriangles[location]);
        assert(boxes==expectedBoxes[location]);
        const CoastPalette day=CoastColors(location,false);
        const CoastPalette night=CoastColors(location,true);
        assert(day.sand_r>night.sand_r);
        assert(day.sand_g>night.sand_g);
        assert(day.sand_b>night.sand_b);
        boxes=triangles=0;
        EmitCoastalTerrain(location,true,box,triangle);
        assert(triangles==expectedTriangles[location]);
        assert(boxes==expectedBoxes[location]);
    }
    {
        int boxes=0,triangles=0;
        EmitCoastalTerrain(Campaign::kLocations,false,
            [&](auto...){++boxes;},[&](auto...){++triangles;});
        assert(boxes==0&&triangles==0);
    }

    // Inspect the exact low-poly vessel recipes used by the headset mesh.
    const auto vesselShape = [](UnitType kind, uint8_t passengers, bool active) {
        Enemy craft{};
        craft.type=kind;craft.passengers=passengers;
        craft.active=active;craft.x=.6f;craft.z=-5.0f;
        int boxes=0, triangles=0;
        EmitVesselSilhouette(craft,
            [&](float x,float y,float z,float w,float h,float d,
                float r,float g,float b) {
                ++boxes;
                assert(w>0.0f&&h>0.0f&&d>0.0f);
                assert(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z));
                assert(std::isfinite(r)&&std::isfinite(g)&&std::isfinite(b));
            },
            [&](float x0,float y0,float z0,float x1,float y1,float z1,
                float x2,float y2,float z2,float r,float g,float b) {
                ++triangles;
                assert(std::isfinite(x0)&&std::isfinite(y0)&&std::isfinite(z0));
                assert(std::isfinite(x1)&&std::isfinite(y1)&&std::isfinite(z1));
                assert(std::isfinite(x2)&&std::isfinite(y2)&&std::isfinite(z2));
                assert(std::isfinite(r)&&std::isfinite(g)&&std::isfinite(b));
            });
        return std::array<int,2>{{boxes,triangles}};
    };
    assert((vesselShape(UnitType::Rowboat,1,true)==std::array<int,2>{{5,4}}));
    assert((vesselShape(UnitType::LandingCraft,3,true)==std::array<int,2>{{10,4}}));
    assert((vesselShape(UnitType::LandingCraft,6,true)==std::array<int,2>{{16,4}}));
    assert((vesselShape(UnitType::LandingCraft,255,true)==std::array<int,2>{{16,4}}));
    assert((vesselShape(UnitType::Gunboat,0,true)==std::array<int,2>{{7,4}}));
    assert((vesselShape(UnitType::Aircraft,0,true)==std::array<int,2>{{0,0}}));
    assert((vesselShape(UnitType::Rowboat,1,false)==std::array<int,2>{{0,0}}));
    const Infantry activeInfantry{.3f,-2.0f,true};
    int soldierBoxes=0,soldierTriangles=0;
    EmitInfantrySilhouette(activeInfantry,
        [&](float x,float y,float z,float w,float h,float d,
            float, float, float) {
            ++soldierBoxes;
            assert(std::abs(x-activeInfantry.x)<.15f);
            assert(std::abs(z-activeInfantry.z)<.15f);
            assert(w<=.20f&&h<=.36f&&d<=.16f&&y>-.94f);
        },
        [&](float x0,float y0,float z0,float x1,float y1,float z1,
            float x2,float y2,float z2,float, float, float) {
            ++soldierTriangles;
            assert(std::isfinite(x0)&&std::isfinite(y0)&&std::isfinite(z0));
            assert(std::isfinite(x1)&&std::isfinite(y1)&&std::isfinite(z1));
            assert(std::isfinite(x2)&&std::isfinite(y2)&&std::isfinite(z2));
            assert(std::abs(x0-activeInfantry.x)<.15f);
            assert(std::abs(x1-activeInfantry.x)<.15f);
            assert(std::abs(x2-activeInfantry.x)<.15f);
        });
    assert(soldierBoxes==2&&soldierTriangles==5);
    int inactiveGeometry=0;
    EmitInfantrySilhouette(Infantry{},
        [&](auto...){++inactiveGeometry;},[&](auto...){++inactiveGeometry;});
    assert(inactiveGeometry==0);

    // The animation poses are spatially driven, deterministic, and never
    // advance state separately when the right eye renders after the left.
    const float invalidPose=std::numeric_limits<float>::quiet_NaN();
    assert(LandingRampProgress(-11.0f)==0.0f);
    assert(LandingRampProgress(-3.7f)<.0001f);
    assert(LandingRampProgress(-2.2f)>.9999f);
    // The longer heavy hull's center remains offshore while its bow
    // finishes lowering ahead of its two real car spawn positions.
    const float heavy_beach=invasion::BeachingZ(UnitType::ArmoredTransport);
    assert(heavy_beach==-3.40f);
    assert(LandingRampProgress(invasion::HeavyRampReferenceZ(
               heavy_beach))>.9999f);
    const float heavy_bow=heavy_beach+1.28f+.18f;
    assert(heavy_bow>-2.23f && heavy_bow<-1.80f);
    assert(LandingRampProgress(invalidPose)==0.0f);
    assert(VesselWakePhase(-11.0f)==0.0f);
    assert(VesselWakePhase(-10.75f)==.5f);
    assert(VesselWakePhase(-10.5f)==0.0f);
    assert(VesselWakePhase(invalidPose)==0.0f);
    for (int i=0;i<=150;i++) {
        const float z=-3.7f+i*.01f;
        const float progress=LandingRampProgress(z);
        assert(progress>=0.0f && progress<=1.0f);
        if(i) assert(progress>=LandingRampProgress(z-.01f));
    }
    Enemy rampCraft{};
    rampCraft.active=true;
    rampCraft.type=UnitType::LandingCraft;
    rampCraft.passengers=4;
    rampCraft.z=-4.0f;
    const auto rampTip = [](const Enemy &craft) {
        float tip=100.0f;
        int triangleCount=0;
        EmitVesselSilhouette(craft,[](auto...){},
            [&](float,float,float,float,float,float,
                float,float y2,float,float,float,float) {
                if (triangleCount++==0) tip=y2;
            });
        assert(triangleCount==4);
        return tip;
    };
    const float upright=rampTip(rampCraft);
    rampCraft.z=-2.2f;
    const float lowered=rampTip(rampCraft);
    assert(upright>-.47f && lowered< -1.02f);
    assert(rampTip(rampCraft)==lowered);
    const auto movingWake = [](const Enemy &craft) {
        float zRelative=0.0f;
        int triangles=0;
        EmitVesselSilhouette(craft,[](auto...){},
            [&](float,float,float,float,float,float z1,
                float,float,float,float,float,float) {
                if(triangles++==2)zRelative=z1-craft.z;
            });
        assert(triangles==4);
        return zRelative;
    };
    rampCraft.type=UnitType::Rowboat;
    rampCraft.passengers=1;
    rampCraft.z=-10.75f;
    const float wakeA=movingWake(rampCraft);
    rampCraft.z=-10.5f;
    assert(std::abs(wakeA-movingWake(rampCraft))>.03f);
    Infantry marching{.3f,-2.2f,true};
    const auto footPosition = [](const Infantry &trooper) {
        float relativeZ=0.0f;
        int triangles=0;
        EmitInfantrySilhouette(trooper,[](auto...){},
            [&](float,float,float,float,float,float z1,
                float,float,float,float,float,float) {
                if(triangles++==2)relativeZ=z1-trooper.z;
            });
        assert(triangles==5);
        return relativeZ;
    };
    const float firstStride=InfantryMarchStride(marching);
    const float firstFoot=footPosition(marching);
    marching.z=-2.08f;
    assert(InfantryMarchStride(marching)*firstStride<0.0f);
    assert(std::abs(footPosition(marching)-firstFoot)>.05f);
    marching.z=-1.34f;
    assert(InfantryMarchStride(marching)==0.0f);
    marching.active=false;
    assert(InfantryMarchStride(marching)==0.0f);
    marching.active=true;
    marching.z=invalidPose;
    assert(InfantryMarchStride(marching)==0.0f);

    // Ballistic trajectory, ray intersection and ghost aim are pure C++.
    // In ordinary MG mode, pointing a straight ray at a far vessel must
    // miss low; raising it compensates for the simulated trajectory drop.
    const float gun_origin[]={0.0f,-.70f,0.0f};
    const float direct[]={0.0f,0.0f,-1.0f};
    const float ship[]={0.0f,-.70f,-9.0f};
    const float flat_gravity=WeaponFor(WeaponKind::MachineGun).gravity;
    assert(flat_gravity>0.0f);
    float distance=0.0f,point[3]={};
    assert(ProjectilePosition(gun_origin,direct,9.0f,flat_gravity,point));
    assert(std::abs(point[1]+.70f+flat_gravity*81.0f)<.00001f);
    assert(!ProjectileHitsSphere(gun_origin,direct,flat_gravity,ship,
                                 .65f,&distance));
    const float raised[]={0.0f,.125f,-1.0f};
    assert(ProjectileHitsSphere(gun_origin,raised,flat_gravity,ship,
                                .65f,&distance));
    assert(distance>8.0f&&distance<10.0f);
    assert(ProjectileHitsSphere(gun_origin,direct,0.0f,ship,
                                .65f,&distance));
    assert(!ProjectileHitsSphere(gun_origin,direct,flat_gravity,ship,
                                -1.0f,&distance));
    assert(!ProjectileHitsSphere(gun_origin,direct,-.1f,ship,
                                .65f,&distance));
    const float high[]={0.0f,2.0f,-.4f};
    assert(!WeaponWithinTraverse(high,WeaponKind::DirectCannon));
    assert(!WeaponWithinTraverse(high,WeaponKind::MachineGun));
    assert(WeaponWithinTraverse(raised,WeaponKind::DirectCannon));
    const float modest[]={0.0f,.75f,-1.0f};
    assert(WeaponWithinTraverse(modest,WeaponKind::Flak));
    assert(!WeaponWithinTraverse(modest,WeaponKind::DirectCannon));
    assert(WeaponFor(WeaponKind::MissileBattery).uses_ghost);
    assert(WeaponFor(WeaponKind::Flak).air_capable);
    float ghost[3]={};
    assert(GhostAimPoint(gun_origin,high,ghost));
    assert(ghost[1]>gun_origin[1]+4.0f);
    const float bad[]={0.0f,std::numeric_limits<float>::quiet_NaN(),-1.0f};
    assert(!GhostAimPoint(gun_origin,bad,ghost));

    // Support labels consume held Select after the initial action edge.
    // They must not fire the gun through the HUD or spend ammunition.
    assert(HitCombatSupportHud(-2.0f,.95f)==CombatHudTarget::Artillery);
    assert(HitCombatSupportHud(2.0f,.95f)==CombatHudTarget::Airstrike);
    assert(HitCombatSupportHud(0.0f,.95f)==CombatHudTarget::Sam);
    assert(HitCombatSupportHud(-2.4f,-1.44f)==CombatHudTarget::Weapon0);
    assert(HitCombatSupportHud(2.4f,-1.44f)==CombatHudTarget::Weapon4);
    assert(RouteCombatInput(CombatHudTarget::Sam,true)==CombatInputRoute::Sam);
    assert(RouteCombatInput(CombatHudTarget::Sam,false)==CombatInputRoute::NoAction);
    assert(RouteCombatInput(CombatHudTarget::Weapon2,true)==CombatInputRoute::Weapon2);
    assert(HitCombatSupportHud(-2.0f,.65f)==CombatHudTarget::None);
    assert(HitCombatSupportHud(2.0f,1.25f)==CombatHudTarget::None);
    assert(HitCombatSupportHud(std::numeric_limits<float>::quiet_NaN(),.95f)
           ==CombatHudTarget::None);
    assert(RouteCombatInput(CombatHudTarget::Artillery,true)
           ==CombatInputRoute::Artillery);
    assert(RouteCombatInput(CombatHudTarget::Airstrike,true)
           ==CombatInputRoute::Airstrike);
    assert(RouteCombatInput(CombatHudTarget::Artillery,false)
           ==CombatInputRoute::NoAction);
    assert(RouteCombatInput(CombatHudTarget::Airstrike,false)
           ==CombatInputRoute::NoAction);
    assert(RouteCombatInput(CombatHudTarget::None,false)
           ==CombatInputRoute::Gun);
    assert(RouteCombatInput(CombatHudTarget::None,true)
           ==CombatInputRoute::Gun);

    // UI presses remain captured across aim drift and phase transitions.
    // Releasing Select restores normal held-fire ownership.
    CombatInputLatch latch;
    assert(latch.Route(CombatHudTarget::Artillery,true)
           ==CombatInputRoute::Artillery);
    assert(latch.Route(CombatHudTarget::Artillery,false)
           ==CombatInputRoute::NoAction);
    assert(latch.Route(CombatHudTarget::None,false)
           ==CombatInputRoute::NoAction); // drag off ARTY while held
    assert(latch.Route(CombatHudTarget::Airstrike,false)
           ==CombatInputRoute::NoAction); // cannot activate AIR by dragging
    latch.Release();
    assert(latch.Route(CombatHudTarget::None,true)
           ==CombatInputRoute::Gun);
    assert(latch.Route(CombatHudTarget::None,false)
           ==CombatInputRoute::Gun); // held gunfire still works
    assert(latch.Route(CombatHudTarget::Airstrike,false)
           ==CombatInputRoute::NoAction); // hovering over HUD blocks gun
    latch.Release();
    latch.CaptureUiPress(); // shop START or secondary wave-start with Select held
    assert(latch.Route(CombatHudTarget::None,false)
           ==CombatInputRoute::NoAction); // no accidental first-wave shot
    latch.Release();
    assert(latch.Route(CombatHudTarget::None,true)
           ==CombatInputRoute::Gun);
    latch.Release();
    assert(latch.Route(CombatHudTarget::Airstrike,true)
           ==CombatInputRoute::Airstrike);
    assert(latch.Route(CombatHudTarget::None,false)
           ==CombatInputRoute::NoAction);

    // Procedural hit feedback uses finite positions and a bounded pulse pool.
    ImpactEffects effects;
    const float muzzle[]={0,0,0}, downward[]={0,-.2f,-1};
    assert(effects.RecordHit(muzzle,downward,4.0f,ImpactKind::Metal));
    assert(std::abs(effects.pulses()[0].y+.8f)<.0001f);
    assert(effects.pulses()[0].kind==ImpactKind::Metal);
    assert(effects.RecordWaterMiss(muzzle,downward));
    assert(std::abs(effects.pulses()[1].y+1.10f)<.0001f);
    assert(effects.pulses()[1].kind==ImpactKind::Water);
    const float horizontal[]={0,0,-1}, zeroVector[]={0,0,0};
    assert(!effects.RecordWaterMiss(muzzle,horizontal));
    assert(!effects.RecordHit(muzzle,zeroVector,2,ImpactKind::Sand));
    assert(!effects.RecordHit(muzzle,horizontal,41,ImpactKind::Metal));
    const float nanVector[]={0,std::numeric_limits<float>::quiet_NaN(),-1};
    assert(!effects.RecordHit(muzzle,nanVector,2,ImpactKind::Metal));
    effects.Tick(.15f);
    assert(effects.pulses()[0].remaining>0.14f);
    effects.Tick(.15f);
    assert(effects.pulses()[0].remaining==0.0f);
    for(size_t i=0;i<ImpactEffects::kCapacity+2;i++)
        assert(effects.RecordHit(muzzle,horizontal,static_cast<float>(i+1),
                                 ImpactKind::Sand));
    size_t liveEffects=0;
    for(const ImpactPulse &pulse:effects.pulses())
        if(pulse.remaining>0) ++liveEffects;
    assert(liveEffects==ImpactEffects::kCapacity);

    // Minor impact saturation cannot erase a live explosion.
    ImpactEffects priority;
    assert(priority.RecordHit(muzzle,horizontal,1,ImpactKind::Explosion));
    for(size_t i=0;i<ImpactEffects::kCapacity+6;i++)
        assert(priority.RecordHit(muzzle,horizontal,2,ImpactKind::Sand));
    size_t retained=0;
    for(const auto &pulse:priority.pulses())
        if(pulse.remaining>0 && pulse.kind==ImpactKind::Explosion) ++retained;
    assert(retained==1);
    ImpactEffects allBlasts;
    for(size_t i=0;i<ImpactEffects::kCapacity;i++)
        assert(allBlasts.RecordHit(muzzle,horizontal,1,ImpactKind::Explosion));
    assert(!allBlasts.RecordHit(muzzle,horizontal,2,ImpactKind::Water));
    assert(allBlasts.RecordHit(muzzle,horizontal,2,ImpactKind::AirstrikeBlast));
    allBlasts.Tick(ImpactEffects::kAirstrikeLifetime);
    assert(allBlasts.RecordHit(muzzle,horizontal,2,ImpactKind::Water));

    // Battery warnings come from actual integrity loss, not predicted threats.
    BatteryDamageFeedback damage_feedback;
    assert(!damage_feedback.active());
    assert(!damage_feedback.Record(100,100));
    assert(!damage_feedback.Record(100,101));
    assert(!damage_feedback.Record(101,90));
    assert(!damage_feedback.Record(100,-1));
    assert(damage_feedback.Record(100,97));
    assert(damage_feedback.damage()==3);
    damage_feedback.Tick(.30f);
    assert(damage_feedback.active());
    assert(damage_feedback.Record(97,85));
    assert(damage_feedback.damage()==12);
    assert(damage_feedback.remaining()==BatteryDamageFeedback::kDuration);
    damage_feedback.Tick(std::numeric_limits<float>::quiet_NaN());
    assert(damage_feedback.active());
    damage_feedback.Tick(1.0f);
    assert(!damage_feedback.active());
    assert(damage_feedback.Record(2,0) && damage_feedback.damage()==2);

    // A hit spark requires real damage; lethal hits have longer blast pulses.
    // The unupgraded gun consumes ammo against aircraft without dealing damage.
    ImpactEffects blasts;
    assert(!blasts.RecordDamageHit(muzzle,horizontal,5.0f,100,100,true));
    assert(blasts.RecordDamageHit(muzzle,horizontal,5.0f,100,80,true));
    assert(blasts.pulses()[0].kind==ImpactKind::Metal);
    assert(blasts.RecordDamageHit(muzzle,horizontal,5.0f,20,-10,false));
    assert(blasts.pulses()[1].kind==ImpactKind::Explosion);
    assert(std::abs(blasts.pulses()[1].remaining-
                    ImpactEffects::kExplosionLifetime)<.0001f);
    assert(!blasts.RecordDamageHit(muzzle,zeroVector,5.0f,20,0,false));
    assert(!blasts.RecordDamageHit(muzzle,horizontal,41.0f,20,0,false));
    blasts.Tick(ImpactEffects::kLifetime);
    assert(blasts.pulses()[0].remaining==0.0f);
    assert(blasts.pulses()[1].remaining>0.0f);
    blasts.Tick(ImpactEffects::kExplosionLifetime);
    assert(blasts.pulses()[1].remaining==0.0f);

    // Support strikes produce visible blasts only for enemies whose HP
    // actually changed; lethal targets retain their pre-destruction position.
    Campaign artillery_game(Difficulty::Casual,41);
    assert(artillery_game.Start());
    assert(artillery_game.Purchase(Upgrade::Artillery));
    assert(artillery_game.BeginWave());
    artillery_game.Tick(.05f);
    const auto artillery_before=artillery_game.enemies();
    assert(artillery_before[0].active);
    ImpactEffects support_fx;
    assert(artillery_game.CallArtillery());
    assert(support_fx.RecordSupportDamage(artillery_before,
                                        artillery_game.enemies(),false)==1);
    assert(support_fx.pulses()[0].kind==ImpactKind::ArtilleryBlast);
    assert(std::abs(support_fx.pulses()[0].x-artillery_before[0].x)<.0001f);
    assert(std::abs(support_fx.pulses()[0].z-artillery_before[0].z)<.0001f);
    assert(!artillery_game.enemies()[0].active);
    assert(std::abs(support_fx.pulses()[0].remaining-
                    ImpactEffects::kArtilleryLifetime)<.0001f);
    assert(!artillery_game.CallArtillery());
    assert(support_fx.RecordSupportDamage(artillery_game.enemies(),
                                         artillery_game.enemies(),false)==0);
    support_fx.Tick(ImpactEffects::kArtilleryLifetime);
    assert(support_fx.pulses()[0].remaining==0.0f);

    // An actual purchased turret produces a position-accurate shot event,
    // and a killing shot becomes an explosion rather than a generic spark.
    Campaign without_turret(Difficulty::Casual,41);
    assert(without_turret.Start() && without_turret.BeginWave());
    without_turret.Tick(.05f);
    assert(without_turret.turret_shot().sequence==0);

    Campaign turret_game(Difficulty::Casual,41);
    assert(turret_game.Start());
    assert(turret_game.Purchase(Upgrade::SupportTurret));
    assert(turret_game.BeginWave());
    assert(turret_game.turret_shot().sequence==0);
    assert(turret_game.infantry_volley().sequence==0);
    const auto initial_undamaged=turret_game.enemies();
    for(int frame=0;frame<100;frame++)turret_game.Tick(.05f);
    // Automatic defenses must not silently shoot spawn-point rowboats
    // beyond their actual visible gun emplacement and firing sector.
    assert(turret_game.turret_shot().sequence==0);
    assert(turret_game.enemies()[0].hp==
           initial_undamaged[0].hp || initial_undamaged[0].hp==0);
    for(int frame=0;frame<200 &&
        turret_game.turret_shot().sequence==0;frame++)
        turret_game.Tick(.05f);
    const TurretShotEvent first_turret_shot=turret_game.turret_shot();
    assert(first_turret_shot.sequence==1 && !first_turret_shot.destroyed);
    assert(first_turret_shot.z>=support::kMinimumTargetZ);
    assert(turret_game.enemies()[0].active && turret_game.enemies()[0].hp==5);
    ImpactEffects turret_fx;
    assert(turret_fx.RecordTurretImpact(first_turret_shot.x,
                                        first_turret_shot.z,false));
    assert(turret_fx.pulses()[0].kind==ImpactKind::Metal);
    assert(std::abs(turret_fx.pulses()[0].z-first_turret_shot.z)<.0001f);
    for(int frame=0;frame<35 && turret_game.turret_shot().sequence<2;frame++)
        turret_game.Tick(.05f);
    const TurretShotEvent lethal_turret_shot=turret_game.turret_shot();
    assert(lethal_turret_shot.sequence==2 && lethal_turret_shot.destroyed);
    assert(!turret_game.enemies()[0].active);
    assert(turret_game.wave_status().boats_sunk==1);
    assert(turret_fx.RecordTurretImpact(lethal_turret_shot.x,
                                        lethal_turret_shot.z,true));
    assert(turret_fx.pulses()[1].kind==ImpactKind::Explosion);
    assert(std::abs(turret_fx.pulses()[1].remaining-
                    ImpactEffects::kExplosionLifetime)<.0001f);
    assert(!turret_fx.RecordTurretImpact(
        std::numeric_limits<float>::quiet_NaN(),-11.0f,false));

    // The allied gun physically exists only once bought. Its emitter
    // has many separate physical bodies, a directed muzzle, and a flash
    // dependent on a real authored shot rather than a permanent spark.
    int turretBoxes=0,turretFaces=0;
    support::EmitTurret(0.f,-4.1f,.16f,
        [&](auto...){++turretBoxes;},
        [&](auto...){++turretFaces;});
    assert(turretBoxes>=12 && turretFaces>=6);
    float turretX=0.f,turretY=0.f,turretZ=0.f;
    support::MuzzleToward(0.f,-4.1f,&turretX,&turretY,&turretZ);
    assert(turretX<support::kTurretX);
    assert(turretZ<support::kTurretZ);
    assert(turretY==support::kTurretMuzzleY);
    Enemy offshore{};
    offshore.active=true;offshore.hp=40;offshore.type=UnitType::Rowboat;
    offshore.z=-11.f;
    assert(!support::InRange(offshore));
    offshore.z=-4.f;
    assert(support::InRange(offshore));
    offshore.type=UnitType::Aircraft;
    assert(!support::InRange(offshore));
    // Surface intersection ordering must beat center-distance ordering.
    // The farther center's larger hull can be the first struck surface.
    const float rayOrigin[]={0,0,5}, rayForward[]={0,0,-1};
    const float smallNear[]={0,0,2}, largeFar[]={0,0,1};
    float smallDepth=0, largeDepth=0;
    assert(RaySphereSurfaceDistance(rayOrigin,rayForward,smallNear,.2f,&smallDepth));
    assert(RaySphereSurfaceDistance(rayOrigin,rayForward,largeFar,1.4f,&largeDepth));
    assert(largeDepth<smallDepth);
    const float center[]={0,0,0}, tangent[]={1,0,0};
    float hit=0;
    assert(RaySphereSurfaceDistance(rayOrigin,rayForward,center,1,&hit));
    assert(std::abs(hit-4.0f)<0.0001f);
    assert(RaySphereSurfaceDistance(rayOrigin,rayForward,tangent,1,&hit));
    assert(std::abs(hit-5.0f)<0.0001f);
    const float inside[]={0,0,0}, exitRay[]={0,0,1};
    assert(RaySphereSurfaceDistance(inside,exitRay,center,1,&hit));
    assert(std::abs(hit-1.0f)<0.0001f);
    const float scaledRay[]={0,0,-2};
    assert(RaySphereSurfaceDistance(rayOrigin,scaledRay,center,1,&hit));
    assert(std::abs(hit-2.0f)<0.0001f);
    const float behind[]={0,0,10}, zero[]={0,0,0};
    const float invalid[]={std::numeric_limits<float>::quiet_NaN(),0,5};
    assert(!RaySphereSurfaceDistance(rayOrigin,rayForward,behind,1,&hit));
    assert(!RaySphereSurfaceDistance(rayOrigin,zero,center,1,&hit));
    assert(!RaySphereSurfaceDistance(invalid,rayForward,center,1,&hit));
    assert(!RaySphereSurfaceDistance(rayOrigin,rayForward,center,-1,&hit));
    assert(!RaySphereSurfaceDistance(rayOrigin,rayForward,center,1,nullptr));

    Campaign c(Difficulty::Casual, 41);
    assert(c.Start() && c.phase() == Phase::Shop);
    assert(c.waves_this_period() == 12);
    assert(c.Purchase(Upgrade::FireRate));
    assert(c.stats().fire_delay_ms == 265);
    assert(c.Purchase(Upgrade::Magazine));
    assert(c.stats().magazine == 16);
    assert(c.BeginWave());
    for (int i=0;i<50;i++) c.Tick(0.05f);
    bool saw_rowboat=false;
    for (const Enemy &e:c.enemies()) {
        if (e.active) {
            assert(e.type == UnitType::Rowboat && e.passengers == 1);
            saw_rowboat=true;
        }
    }
    assert(saw_rowboat);
    assert(c.incoming_threats().aircraft_seconds<0.0f);
    assert(c.incoming_threats().gunboat_seconds<0.0f);
    // The SDK permits a 0.10-second frame delta. One slow frame must
    // simulate the same world as two 0.05-second frames, without letting
    // an arbitrarily long resume pause skip an entire battle.
    Campaign slow_frame(Difficulty::Casual,41);
    Campaign split_frame(Difficulty::Casual,41);
    Campaign long_pause(Difficulty::Casual,41);
    for(Campaign *game : {&slow_frame,&split_frame,&long_pause}) {
        assert(game->Start() && game->BeginWave());
    }
    for(int frame=0;frame<81;frame++) {
        slow_frame.Tick(0.10f);
        split_frame.Tick(0.05f);
        split_frame.Tick(0.05f);
        long_pause.Tick(5.0f);
        assert(slow_frame.phase()==split_frame.phase());
        assert(slow_frame.integrity()==split_frame.integrity());
        assert(slow_frame.infantry()==split_frame.infantry());
        assert(slow_frame.wave_status().boats_sunk==split_frame.wave_status().boats_sunk);
        for(size_t i=0;i<Campaign::kMaxEnemies;i++) {
            const Enemy &a=slow_frame.enemies()[i], &b=split_frame.enemies()[i];
            const Enemy &c=long_pause.enemies()[i];
            assert(a.active==b.active && a.active==c.active);
            if(a.active) {
                assert(a.type==b.type && a.hp==b.hp);
                assert(a.z==b.z && a.z==c.z);
            }
        }
    }
    assert(slow_frame.enemies()[0].active);
    assert(slow_frame.enemies()[0].z>-11.0f);
    Campaign sunk(Difficulty::Casual,41);
    assert(sunk.Start() && sunk.BeginWave());
    for (int i=0;i<180;i++) {
        sunk.Tick(0.05f);
        for (uint8_t n=0;n<sunk.enemies().size();++n) {
            if (sunk.enemies()[n].active) sunk.FireAt(n);
        }
        if (sunk.phase()!=Phase::Combat) break;
    }
    assert(sunk.wave_status().boats_sunk>=1);
    assert(sunk.wave_status().troops_landed==0);
    assert(sunk.infantry()==0);

    // Trigger pulls are physical actions, not free hit-probes: misses spend
    // ammunition, respect fire cadence and enter the same timed reload cycle.
    Campaign firing(Difficulty::Casual,19);
    assert(firing.Start() && firing.BeginWave());
    const uint8_t full_mag=firing.magazine_left();
    assert(full_mag==firing.stats().magazine);
    assert(!firing.reloading());
    assert(firing.reload_fraction()==0.0f);
    assert(firing.FireMiss());
    assert(firing.magazine_left()==full_mag-1);
    assert(!firing.reloading());
    assert(firing.reload_fraction()==0.0f);
    assert(!firing.FireMiss());
    while(firing.magazine_left()>0) {
        for(int i=0;i<7;i++) firing.Tick(0.05f);
        assert(firing.FireMiss());
    }
    assert(firing.reloading());
    assert(firing.reload_fraction()==0.0f);
    assert(!firing.FireMiss());
    firing.Tick(0.05f);
    const float early_progress=firing.reload_fraction();
    assert(early_progress>0.0f && early_progress<1.0f);
    assert(firing.magazine_left()==0);
    assert(!firing.FireMiss());
    for(int i=0;i<9;i++) firing.Tick(0.05f);
    assert(firing.reloading());
    assert(firing.reload_fraction()>early_progress);
    assert(firing.reload_fraction()<1.0f);
    assert(!firing.FireMiss());
    for(int i=0;i<14;i++) firing.Tick(0.05f);
    assert(!firing.reloading());
    assert(firing.reload_fraction()==0.0f);
    assert(firing.magazine_left()==firing.stats().magazine);
    assert(firing.FireMiss());

    // The combat secondary action allows a tactical partial-magazine reload,
    // but never an instant refill, a second concurrent reload, or a full-mag
    // exploit. The existing timed reload path remains authoritative.
    Campaign tactical(Difficulty::Casual,19);
    assert(!tactical.RequestReload()); // briefing
    assert(tactical.Start());
    assert(!tactical.RequestReload()); // shop
    assert(tactical.BeginWave());
    const uint8_t tactical_full=tactical.magazine_left();
    assert(!tactical.RequestReload()); // full magazine
    assert(tactical.FireMiss());
    assert(tactical.magazine_left()==tactical_full-1);
    assert(tactical.RequestReload());
    assert(tactical.reloading());
    assert(tactical.magazine_left()==tactical_full-1);
    assert(!tactical.RequestReload());
    assert(!tactical.FireMiss());
    tactical.Tick(.05f);
    assert(tactical.reload_fraction()>0.0f);
    for(int i=0;i<24;i++) tactical.Tick(.05f);
    assert(!tactical.reloading());
    assert(tactical.magazine_left()==tactical_full);
    assert(!tactical.RequestReload());
    assert(tactical.FireMiss());

    Campaign landed(Difficulty::Casual,41);
    assert(landed.Start());
    assert(landed.incoming_threats().infantry_seconds<0.0f); // shop
    assert(landed.BeginWave());
    assert(landed.incoming_threats().infantry_seconds<0.0f); // offshore
    for(int i=0;i<4500 && landed.wave_status().troops_landed==0;i++)
        landed.Tick(0.05f);
    assert(landed.wave_status().troops_landed>0);
    assert(landed.infantry()>0);
    // Earliest live landing party still needs to traverse the beach and
    // wait a full salvo period; boats have no imaginary infantry timer.
    const IncomingThreats approaching_infantry=landed.incoming_threats();
    assert(approaching_infantry.infantry_seconds>10.0f);
    assert(approaching_infantry.aircraft_seconds<0.0f);
    // Disembarking is not an instant firing position: troops must cross the
    // exposed beach, then complete a full four-second attack countdown.
    const int32_t integrity=landed.integrity();
    for(int i=0;i<120;i++) landed.Tick(0.05f); // six seconds after first landing
    assert(landed.integrity()==integrity);
    assert(landed.infantry_volley().sequence==0);
    assert(landed.infantry_volley().shooters==0);
    assert(landed.infantry_volley().damage==0);
    // A living shore party approaches the firing line: the estimate enters
    // the visible six-second window before any damage occurs.
    const float shore_countdown=landed.incoming_threats().infantry_seconds;
    assert(shore_countdown>4.0f && shore_countdown<6.0f);
    for(const Infantry &trooper:landed.infantry_units())
        if(trooper.active) assert(trooper.z < -1.34f);
    for(int i=0;i<160;i++) landed.Tick(0.05f);
    assert(landed.integrity()<integrity);
    const InfantryVolleyEvent first_volley=landed.infantry_volley();
    // The same authoritative active/ready roster is used for the firing
    // animation snapshot. No dead or still-advancing soldier flashes.
    const uint64_t firingMask=first_volley.firing_mask;
    size_t displayed=0;
    for(size_t slot=0;slot<Campaign::kMaxInfantry;++slot) {
        const Infantry &unit=landed.infantry_units()[slot];
        const bool shown=InfantryFiringPose(unit,firingMask,slot);
        if(shown) {
            assert(unit.active && InfantryReady(unit));
            assert(InfantrySprite(unit,true)!=InfantrySprite(unit,false));
            ++displayed;
        } else if(!unit.active || !InfantryReady(unit)) {
            assert(!InfantryFiringPose(unit,~uint64_t{0},slot));
        }
    }
    assert(displayed>0 && displayed==first_volley.shooters);
    assert(first_volley.sequence>=1);
    assert(landed.incoming_threats().infantry_seconds>=0.0f);
    assert(landed.incoming_threats().infantry_seconds<=4.0f);
    assert(first_volley.shooters>0 && first_volley.shooters<=landed.infantry());
    assert(first_volley.damage>0 && first_volley.damage<=first_volley.shooters*2);
    assert(std::isfinite(first_volley.x) && std::isfinite(first_volley.z));
    bool matched_shooter=false;
    for(const Infantry &trooper:landed.infantry_units())
        if(trooper.active && std::abs(trooper.x-first_volley.x)<.001f &&
           std::abs(trooper.z-first_volley.z)<.001f) matched_shooter=true;
    assert(matched_shooter);
    assert(first_volley.z>=-1.34f);
    while(landed.infantry()) {
        landed.Tick(0.05f);
        for(uint8_t i=0;i<landed.infantry_units().size();++i)
            if(landed.infantry_units()[i].active && landed.FireAtInfantry(i)) break;
    }
    assert(landed.wave_status().troops_stopped>=1);
    assert(landed.incoming_threats().infantry_seconds<0.0f);
    assert(landed.waves_this_period()==12);

    // Infantry volleys must report the real damage against the final hull
    // point, rather than the nominal damage from every ready shooter.
    Campaign last_infantry_hull(Difficulty::Casual,41);
    assert(last_infantry_hull.Start());
    uint8_t last_infantry_save[Campaign::kSerializedSize]={};
    assert(last_infantry_hull.SerializeShop(last_infantry_save,
                                           sizeof(last_infantry_save)));
    last_infantry_save[36]=1; // integrity word, little-endian
    last_infantry_save[37]=0;
    last_infantry_save[38]=0;
    last_infantry_save[39]=0;
    assert(last_infantry_hull.DeserializeShop(last_infantry_save,
                                              sizeof(last_infantry_save)));
    assert(last_infantry_hull.BeginWave());
    for(int i=0;i<1600 && last_infantry_hull.phase()==Phase::Combat;i++)
        last_infantry_hull.Tick(.05f);
    assert(last_infantry_hull.phase()==Phase::Defeat);
    assert(last_infantry_hull.integrity()==0);
    const InfantryVolleyEvent fatal_volley=last_infantry_hull.infantry_volley();
    assert(fatal_volley.sequence==1 && fatal_volley.damage==1);
    assert(fatal_volley.shooters>0);
    assert(std::isfinite(fatal_volley.x) && std::isfinite(fatal_volley.z));

    // The campaign allocates independently moving soldiers with real targets.
    Campaign beach(Difficulty::Casual,41);
    assert(beach.Start() && beach.BeginWave());
    for(int i=0;i<360 && beach.infantry()==0;i++) beach.Tick(0.05f);
    assert(beach.infantry()>0);
    uint8_t first=255, active=0;
    for(uint8_t i=0;i<beach.infantry_units().size();i++) {
        if(!beach.infantry_units()[i].active) continue;
        if(first==255) first=i;
        ++active;
    }
    assert(active==beach.infantry() && first!=255);
    const Infantry soldier=beach.infantry_units()[first];
    assert(beach.incoming_threats().infantry_seconds>6.0f);
    const float origin[]={soldier.x,-0.66f,soldier.z+1.0f};
    const float missOrigin[]={soldier.x+0.6f,-0.66f,soldier.z+1.0f};
    const float ray[]={0,0,-1};
    float hitDepth=0;
    assert(RayHitsInfantry(soldier,origin,ray,&hitDepth));
    assert(hitDepth>0.69f && hitDepth<0.71f);
    assert(!RayHitsInfantry(soldier,missOrigin,ray,&hitDepth));
    assert(!RayHitsInfantry(soldier,origin,nullptr,&hitDepth));
    for(int i=0;i<10;i++) beach.Tick(0.05f);
    assert(beach.infantry_units()[first].z>soldier.z);
    const uint8_t ammoBefore=beach.magazine_left();
    assert(!beach.FireAtInfantry(static_cast<uint8_t>(Campaign::kMaxInfantry-1)));
    assert(beach.magazine_left()==ammoBefore);
    assert(beach.FireAtInfantry(first));
    assert(!beach.infantry_units()[first].active);
    assert(beach.infantry()==active-1);
    assert(beach.magazine_left()==ammoBefore-1);
    // The only landed soldier is gone: active offshore transports are not
    // counted as shooters until they really disembark.
    if (beach.infantry()==0)
        assert(beach.incoming_threats().infantry_seconds<0.0f);
    assert(!beach.FireAtInfantry(first));
    assert(beach.magazine_left()==ammoBefore-1);

    // Higher difficulties add more boats and waves, never skip the
    // opening single-passenger rowboat introduction.
    for(Difficulty d : {Difficulty::Casual, Difficulty::Regular,
                        Difficulty::Veteran, Difficulty::Siege}) {
        Campaign opening(d,17);
        assert(opening.Start() && opening.BeginWave());
        opening.Tick(0.05f);
        assert(opening.enemies()[0].active);
        assert(opening.enemies()[0].type==UnitType::Rowboat);
        assert(opening.enemies()[0].passengers==1);
    }

    Campaign veteran(Difficulty::Siege,7);
    assert(veteran.Start());
    assert(veteran.waves_this_period()==18);
    assert(veteran.price(Upgrade::AntiAir)>0);
    assert(!veteran.FireAt(0));
    assert(veteran.Purchase(Upgrade::AntiAir));
    assert(veteran.stats().anti_air);
    assert(veteran.stats().anti_air_damage_bonus==0);
    assert(veteran.price(Upgrade::AntiAir)==1050);
    assert(!veteran.Purchase(Upgrade::AntiAir)); // insufficient credits
    assert(veteran.Purchase(Upgrade::Artillery));
    assert(veteran.Purchase(Upgrade::Airstrike)==false); // insufficient credits
    assert(veteran.BeginWave());
    assert(!veteran.CallArtillery()); // no vessel spawned yet
    assert(veteran.artillery_charges()==1);
    veteran.Tick(0.05f);
    assert(veteran.enemies()[0].active);
    assert(veteran.CallArtillery());
    assert(veteran.artillery_charges()==0);
    assert(!veteran.CallArtillery());
    assert(!veteran.Purchase(Upgrade::Damage));

    // Support charges are never consumed by a call with no eligible target.
    Campaign empty_support(Difficulty::Casual,41);
    assert(empty_support.Start());
    assert(empty_support.Purchase(Upgrade::Artillery));
    assert(empty_support.Purchase(Upgrade::Airstrike));
    assert(!empty_support.CallArtillery()); // shop phase
    assert(!empty_support.CallAirstrike());
    assert(empty_support.BeginWave());
    assert(!empty_support.CallArtillery()); // no active enemies
    assert(!empty_support.CallAirstrike());
    assert(empty_support.artillery_charges()==1);
    assert(empty_support.airstrike_charges()==1);
    empty_support.Tick(0.05f);
    assert(empty_support.enemies()[0].active);
    assert(empty_support.CallArtillery());
    assert(empty_support.artillery_charges()==0);
    assert(!empty_support.CallAirstrike()); // artillery cleared the only target
    assert(empty_support.airstrike_charges()==1);
    for(int i=0;i<33;i++) empty_support.Tick(0.05f);
    bool next_target=false;
    for(const Enemy &e:empty_support.enemies()) if(e.active) next_target=true;
    assert(next_target);
    assert(empty_support.CallAirstrike());
    assert(empty_support.airstrike_charges()==0);
    assert(!empty_support.CallAirstrike());

    Campaign turret(Difficulty::Casual,41);
    assert(turret.Start());
    assert(turret.Purchase(Upgrade::SupportTurret));
    assert(turret.stats().turret);
    assert(turret.BeginWave());
    for(int i=0;i<160;i++) turret.Tick(0.05f);
    assert(turret.wave_status().boats_sunk>=1);
    assert(turret.wave_status().troops_landed==0);

    // Snapshot only stable shop state. Corrupt/mismatched snapshots must
    // leave the live campaign untouched, including purchased upgrades.
    Campaign writer(Difficulty::Veteran,1234);
    assert(writer.Start());
    assert(writer.Purchase(Upgrade::Damage));
    assert(writer.Purchase(Upgrade::Artillery));
    uint8_t encoded[Campaign::kSerializedSize]={};
    assert(writer.SerializeShop(encoded,sizeof(encoded)));
    Campaign restored;
    assert(restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.phase()==Phase::Shop);
    assert(restored.credits()==writer.credits());
    assert(restored.level(Upgrade::Damage)==1);
    assert(restored.artillery_charges()==1);
    assert(restored.waves_this_period()==16);
    const int credit_before=restored.credits();
    encoded[0]^=0xffu;
    assert(!restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.credits()==credit_before);
    encoded[0]^=0xffu;
    // High wave indices must survive the original 80-byte v2 save format.
    encoded[28]=16u; // last legal opening Veteran daytime wave
    assert(restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.wave_index()==16u);
    encoded[28]=17u; // valid uint8_t, invalid period progress
    assert(!restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.wave_index()==16u);
    // The nine-day / 244-352-wave campaign previously accepted wave
    // indices up to 32 in simulation but rejected shop saves above eight.
    // Verify the late actual location/period maximum, *not* just uint8_t
    // representability; never silently wrap the serialized 32-bit word.
    uint8_t late[Campaign::kSerializedSize]={};
    assert(writer.SerializeShop(late,sizeof(late)));
    late[12]=3u; // Siege difficulty
    late[16]=2u; // Blackcap Island (third map)
    late[20]=4u; // day 4
    late[24]=0u; // daylight
    late[28]=25u; // 12+2*3+3+2*2, final day limit
    assert(restored.DeserializeShop(late,sizeof(late)));
    assert(restored.location()==2u && restored.day()==4u);
    assert(restored.difficulty()==Difficulty::Siege);
    assert(restored.waves_this_period()==25u);
    assert(restored.wave_index()==25u);
    late[28]=26u;
    assert(!restored.DeserializeShop(late,sizeof(late)));
    assert(restored.wave_index()==25u);
    late[24]=1u; // night now has only 21 waves
    late[28]=21u;
    assert(restored.DeserializeShop(late,sizeof(late)));
    assert(restored.period()==Period::Night);
    assert(restored.waves_this_period()==21u);
    late[28]=22u;
    assert(!restored.DeserializeShop(late,sizeof(late)));
    assert(restored.wave_index()==21u);
    // Resume the original Veteran fixture for the subsequent corrupted
    // save atomicity checks, which assert original credits/upgrade state.
    encoded[28]=16u;
    assert(restored.DeserializeShop(encoded,sizeof(encoded)));
    encoded[28]=0xffu; // invalid wave index, at word 7
    assert(!restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.credits()==credit_before);
    // Corrupt high bytes used to wrap a 32-bit wave index to a valid
    // uint8_t wave 1. Reject both cases without changing the live shop.
    encoded[28]=1u;
    encoded[29]=1u; // 257: low byte is a legal wave 1
    assert(!restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.credits()==credit_before);
    assert(restored.level(Upgrade::Damage)==1);
    encoded[29]=0u;
    encoded[30]=1u; // 65537: still narrows to wave 1
    assert(!restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.credits()==credit_before);
    encoded[30]=0u;
    assert(restored.DeserializeShop(encoded,sizeof(encoded)));
    assert(restored.wave_index()==1);
    assert(writer.BeginWave());
    assert(!writer.SerializeShop(encoded,sizeof(encoded)));

    // A location-stage fixture proves the military landing craft is an
    // actual transport containing 3–6 soldiers, not a decorative rowboat.
    Campaign stage(Difficulty::Casual,6);
    assert(stage.Start());
    uint8_t stage_bytes[Campaign::kSerializedSize]={};
    assert(stage.SerializeShop(stage_bytes,sizeof(stage_bytes)));
    stage_bytes[16]=1; // advance location from Shingle Bay to Iron Quay
    Campaign landing_stage;
    assert(landing_stage.DeserializeShop(stage_bytes,sizeof(stage_bytes)));
    assert(landing_stage.location()==1);
    assert(landing_stage.BeginWave());
    landing_stage.Tick(0.05f);
    const Enemy first_craft=landing_stage.enemies()[0];
    assert(first_craft.active && first_craft.type==UnitType::LandingCraft);
    assert(first_craft.passengers>=3 && first_craft.passengers<=6);
    for(int i=0;i<35;i++) {
        landing_stage.FireAt(0);
        landing_stage.Tick(0.05f);
    }
    assert(landing_stage.wave_status().boats_sunk>=1);
    assert(landing_stage.wave_status().troops_landed==0);

    // Identical seeded landing-craft progression, but allow the troops to
    // disembark. Real Campaign::Land must actually assign maneuver roles.
    Campaign tactic_landing;
    assert(tactic_landing.DeserializeShop(stage_bytes,sizeof(stage_bytes)));
    assert(tactic_landing.BeginWave());
    bool found_flank=false;
    bool found_cover=false;
    int covered_slot=-1;
    int covered_wall=-1;
    for(int tick=0;tick<480 && tactic_landing.phase()==Phase::Combat;
        ++tick) {
        tactic_landing.Tick(.05f);
        for(const Infantry &unit:tactic_landing.infantry_units()) {
            if(!unit.active)continue;
            if(unit.tactic==InfantryTactic::Rifle &&
               unit.cover_slot>=0) {
                found_cover=true;
                covered_slot=static_cast<int>(
                    &unit-&tactic_landing.infantry_units()[0]);
                covered_wall=unit.cover_slot;
                assert(unit.open_lane_x>=-3.1f &&
                       unit.open_lane_x<=3.1f);
                assert(unit.cover_line_z<-1.34f);
                assert(tactic_landing.shore_cover()[
                    static_cast<size_t>(covered_wall)].hp>0);
            }
            if(unit.tactic==InfantryTactic::FlankLeft ||
               unit.tactic==InfantryTactic::FlankRight) {
                found_flank=true;
                assert(unit.lane_x>=-3.1f && unit.lane_x<=3.1f);
                assert(std::abs(unit.lane_x-unit.x)<=.85f+.0001f);
            }
        }
        if(found_flank && found_cover)break;
    }
    assert(found_flank && found_cover);
    assert(covered_slot>=0 && covered_wall>=0);
    assert(tactic_landing.wave_status().troops_landed>=3);
    // Shoot the exact barricade claimed by a real landed rifleman.
    // Confirm its instantaneous warning stops treating the dead wall
    // as a usable firing station and a later tick repositions the unit.
    // Let this real rifleman physically reach the barricade before
    // testing a nonlethal shot against occupied shelter.
    for(int step=0;step<180 &&
        tactic_landing.phase()==Phase::Combat &&
        !InfantryReady(tactic_landing.infantry_units()[
            static_cast<size_t>(covered_slot)]);++step)
        tactic_landing.Tick(.05f);
    assert(tactic_landing.phase()==Phase::Combat);
    const Infantry protectedBefore=tactic_landing.infantry_units()[
        static_cast<size_t>(covered_slot)];
    assert(InfantryReady(protectedBefore));
    assert(CoverSheltersInfantry(protectedBefore,
        tactic_landing.shore_cover()[static_cast<size_t>(covered_wall)],
        covered_wall));
    const int16_t beforePinHp=tactic_landing.shore_cover()[
        static_cast<size_t>(covered_wall)].hp;
    const uint8_t beforePinAmmo=tactic_landing.magazine_left();
    assert(tactic_landing.FireAtCover(static_cast<uint8_t>(covered_wall)));
    const Infantry &pinnedByShot=tactic_landing.infantry_units()[
        static_cast<size_t>(covered_slot)];
    assert(pinnedByShot.pinned_seconds==kInfantryPinSeconds);
    assert(!InfantryReady(pinnedByShot));
    assert(InfantrySprite(pinnedByShot)==SpriteId::InfantryPinned);
    assert(tactic_landing.magazine_left()+1==beforePinAmmo);
    assert(tactic_landing.shore_cover()[
        static_cast<size_t>(covered_wall)].hp<beforePinHp);
    tactic_landing.Tick(.05f);
    assert(tactic_landing.infantry_units()[
        static_cast<size_t>(covered_slot)].pinned_seconds<
        kInfantryPinSeconds);
    const int16_t startingCover=tactic_landing.shore_cover()[
        static_cast<size_t>(covered_wall)].hp;
    int coverShots=0;
    while(tactic_landing.shore_cover()[
          static_cast<size_t>(covered_wall)].hp>0 &&
          tactic_landing.phase()==Phase::Combat && coverShots<12) {
        for(int step=0;step<4;++step)tactic_landing.Tick(.10f);
        assert(tactic_landing.FireAtCover(
            static_cast<uint8_t>(covered_wall)));
        ++coverShots;
    }
    assert(coverShots>=4 && coverShots<=9);
    assert(startingCover>0);
    assert(tactic_landing.shore_cover()[
        static_cast<size_t>(covered_wall)].hp==0);
    const Infantry& beforeRegroup=tactic_landing.infantry_units()[
        static_cast<size_t>(covered_slot)];
    // Cover destruction resolves this order during the actual gun hit,
    // not one simulation frame later.
    assert(beforeRegroup.cover_slot==-1);
    assert(beforeRegroup.regrouping ||
           beforeRegroup.x==protectedBefore.open_lane_x);
    assert(beforeRegroup.lane_x==protectedBefore.open_lane_x);
    const float regroupX=beforeRegroup.x,regroupZ=beforeRegroup.z;
    const IncomingThreats immediateWarning=tactic_landing.incoming_threats();
    assert(std::isfinite(immediateWarning.infantry_seconds));
    assert(beforeRegroup.active);
    tactic_landing.Tick(.05f);
    const Infantry &afterRegroup=tactic_landing.infantry_units()[
        static_cast<size_t>(covered_slot)];
    // The lethal barricade shot also provokes a real brief halt:
    // the soldier remains targetable at the old physical coordinates.
    assert(InfantryPinned(afterRegroup));
    assert(std::abs(afterRegroup.x-regroupX)<.00001f);
    assert(std::abs(afterRegroup.z-regroupZ)<.00001f);
    assert(afterRegroup.cover_slot==-1);
    assert(afterRegroup.regrouping ||
           afterRegroup.x==protectedBefore.open_lane_x);
    assert(afterRegroup.lane_x==protectedBefore.open_lane_x);
    assert(InfantryLineZ(afterRegroup)==-1.34f);

    // Iron Quay day two seed 8 starts with a gunboat. It must be able to
    // fire from offshore, and early interception must prevent those salvos.
    auto prepare_gunboat=[](Campaign &game) {
        uint8_t bytes[Campaign::kSerializedSize]={};
        assert(game.SerializeShop(bytes,sizeof(bytes)));
        bytes[16]=1;
        bytes[20]=2;
        assert(game.DeserializeShop(bytes,sizeof(bytes)));
        assert(game.BeginWave());
        game.Tick(0.05f);
        assert(game.enemies()[0].type==UnitType::Gunboat);
    };
    Campaign firing_ship(Difficulty::Casual,8);
    assert(firing_ship.Start());
    prepare_gunboat(firing_ship);
    const IncomingThreats approaching_gunboat=firing_ship.incoming_threats();
    assert(approaching_gunboat.aircraft_seconds<0.0f);
    assert(approaching_gunboat.gunboat_seconds>3.0f &&
           approaching_gunboat.gunboat_seconds<5.0f);
    for(int i=0;i<70;i++) firing_ship.Tick(0.05f);
    assert(firing_ship.wave_status().gunboat_attacks==0);
    assert(firing_ship.integrity()==100);
    assert(firing_ship.gunboat_salvo().sequence==0);
    for(int i=0;i<25 && firing_ship.wave_status().gunboat_attacks==0;i++)
        firing_ship.Tick(0.05f);
    assert(firing_ship.wave_status().gunboat_attacks==1);
    assert(firing_ship.integrity()==97);
    const GunboatSalvoEvent first_salvo=firing_ship.gunboat_salvo();
    assert(first_salvo.sequence==1 && first_salvo.damage==3);
    assert(std::isfinite(first_salvo.x) && std::isfinite(first_salvo.z));
    assert(first_salvo.x==firing_ship.enemies()[0].x);
    assert(first_salvo.z==firing_ship.enemies()[0].z);
    BatteryDamageFeedback salvo_feedback;
    assert(salvo_feedback.Record(100,firing_ship.integrity()));
    assert(salvo_feedback.damage()==3);
    for(int i=0;i<65 && firing_ship.wave_status().gunboat_attacks<2;i++)
        firing_ship.Tick(0.05f);
    assert(firing_ship.wave_status().gunboat_attacks==2);
    assert(firing_ship.integrity()==94);
    assert(firing_ship.gunboat_salvo().sequence==2);
    assert(firing_ship.gunboat_salvo().damage==3);
    assert(salvo_feedback.Record(97,firing_ship.integrity()));
    assert(salvo_feedback.damage()==3);

    // An armed escort must never suicide-ram, beach, disappear or
    // damage friendly vessels through its own gunfire. It holds the
    // offshore standoff position, continues firing at only the battery,
    // and remains shootable until it is actually destroyed by a defender.
    assert(firing_ship.gunboat_collision().sequence==0);
    const int16_t gunboat_hp=firing_ship.enemies()[0].hp;
    const uint8_t sunk_before=firing_ship.wave_status().boats_sunk;
    const uint8_t stopped_before=firing_ship.wave_status().troops_stopped;
    for(int frame=0;frame<160;frame++) {
        const auto ships_before=firing_ship.enemies();
        firing_ship.Tick(.05f);
        assert(firing_ship.phase()==Phase::Combat);
        const Enemy &gunboat=firing_ship.enemies()[0];
        assert(gunboat.active && gunboat.type==UnitType::Gunboat);
        assert(gunboat.hp==gunboat_hp);
        assert(gunboat.z<=invasion::kGunboatStandOffZ);
        assert(firing_ship.gunboat_collision().sequence==0);
        assert(firing_ship.wave_status().boats_sunk==sunk_before);
        assert(firing_ship.wave_status().troops_stopped==stopped_before);
        for(size_t index=0;index<ships_before.size();++index) {
            const Enemy &before=ships_before[index];
            const Enemy &after=firing_ship.enemies()[index];
            if(before.active && after.active && before.type==after.type)
                assert(before.hp==after.hp); // no hostile friendly fire
        }
    }
    assert(firing_ship.enemies()[0].z==invasion::kGunboatStandOffZ);
    assert(firing_ship.wave_status().gunboat_attacks>=3);
    assert(firing_ship.incoming_threats().gunboat_seconds>=0.0f);
    // The reserved collision effect validates its input, but ordinary
    // gunboats never emit such an event on a live offshore attack run.
    ImpactEffects ram_fx;
    assert(!ram_fx.RecordGunboatCollision(0,-2.2f,0));
    assert(!ram_fx.RecordGunboatCollision(0,-2.2f,9));

    Campaign stopped_ship(Difficulty::Casual,8);
    assert(stopped_ship.Start());
    prepare_gunboat(stopped_ship);
    for(int shot=0;shot<7;shot++) {
        if(shot>0) for(int i=0;i<7;i++) stopped_ship.Tick(0.05f);
        assert(stopped_ship.FireAt(0));
    }
    assert(!stopped_ship.enemies()[0].active);
    assert(stopped_ship.incoming_threats().gunboat_seconds<0.0f);
    for(int i=0;i<140;i++) stopped_ship.Tick(0.05f);
    assert(stopped_ship.wave_status().gunboat_attacks==0);
    assert(stopped_ship.gunboat_salvo().sequence==0);
    assert(stopped_ship.gunboat_collision().sequence==0);
    assert(stopped_ship.integrity()==100);

    // Offshore gunboat salvos also retain their timing across 10 FPS frames.
    Campaign slow_gunboat(Difficulty::Casual,8), split_gunboat(Difficulty::Casual,8);
    assert(slow_gunboat.Start() && split_gunboat.Start());
    prepare_gunboat(slow_gunboat);
    prepare_gunboat(split_gunboat);
    for(int frame=0;frame<100;frame++) {
        slow_gunboat.Tick(0.10f);
        split_gunboat.Tick(0.05f);
        split_gunboat.Tick(0.05f);
    }
    assert(slow_gunboat.wave_status().gunboat_attacks>=1);
    assert(slow_gunboat.wave_status().gunboat_attacks==
           split_gunboat.wave_status().gunboat_attacks);
    assert(slow_gunboat.integrity()==split_gunboat.integrity());
    assert(slow_gunboat.enemies()[0].z==split_gunboat.enemies()[0].z);
    assert(slow_gunboat.gunboat_salvo().sequence==
           split_gunboat.gunboat_salvo().sequence);
    assert(slow_gunboat.gunboat_salvo().z==
           split_gunboat.gunboat_salvo().z);

    // Fatal offshore salvos report the actual remaining hull integrity,
    // not the nominal three-point weapon damage.
    Campaign last_hull(Difficulty::Casual,8);
    assert(last_hull.Start());
    uint8_t low_hull_save[Campaign::kSerializedSize]={};
    assert(last_hull.SerializeShop(low_hull_save,sizeof(low_hull_save)));
    low_hull_save[16]=1; low_hull_save[20]=2;
    low_hull_save[36]=1; // Iron Quay D2, 1 HP
    assert(last_hull.DeserializeShop(low_hull_save,sizeof(low_hull_save)));
    assert(last_hull.BeginWave());
    // A fatal salvo stops the same substep before the next attacker moves.
    // Previously the rest of the enemy loop continued after lethal damage.
    bool stopped_at_fatal_salvo=false;
    for(int i=0;i<150 && last_hull.phase()==Phase::Combat;i++) {
        const bool second_alive=last_hull.enemies()[1].active;
        const float second_z=last_hull.enemies()[1].z;
        last_hull.Tick(.05f);
        if(last_hull.phase()==Phase::Defeat) {
            assert(second_alive);
            assert(last_hull.enemies()[1].active);
            assert(last_hull.enemies()[1].z==second_z);
            stopped_at_fatal_salvo=true;
        }
    }
    assert(stopped_at_fatal_salvo);
    assert(last_hull.phase()==Phase::Defeat);
    assert(last_hull.gunboat_salvo().sequence==1);
    assert(last_hull.gunboat_salvo().damage==1);

    auto stage_blackcap_night=[](Campaign &game) {
        uint8_t bytes[Campaign::kSerializedSize]={};
        assert(game.SerializeShop(bytes,sizeof(bytes)));
        bytes[16]=2; // Blackcap Island
        bytes[20]=4; // final day
        bytes[24]=1; // night
        bytes[28]=1; // first wave in period
        assert(game.DeserializeShop(bytes,sizeof(bytes)));
    };

    // An aircraft-only wave is ineligible for artillery, but air support
    // may destroy it without purchasing the ordinary anti-air upgrade.
    Campaign air_only(Difficulty::Casual,8);
    assert(air_only.Start());
    assert(air_only.Purchase(Upgrade::Artillery));
    assert(air_only.Purchase(Upgrade::Airstrike));
    stage_blackcap_night(air_only);
    assert(air_only.BeginWave());
    air_only.Tick(0.05f);
    assert(air_only.enemies()[0].type==UnitType::Aircraft);
    const auto aircraft_before=air_only.enemies();
    ImpactEffects air_fx;
    assert(!air_only.CallArtillery());
    assert(air_only.artillery_charges()==1);
    assert(air_fx.RecordSupportDamage(aircraft_before,air_only.enemies(),false)==0);
    assert(air_only.CallAirstrike());
    assert(air_fx.RecordSupportDamage(aircraft_before,air_only.enemies(),true)==1);
    assert(air_fx.pulses()[0].kind==ImpactKind::AirstrikeBlast);
    assert(std::abs(air_fx.pulses()[0].y-1.6f)<.0001f);
    assert(std::abs(air_fx.pulses()[0].z-aircraft_before[0].z)<.0001f);
    assert(std::abs(air_fx.pulses()[0].remaining-
                    ImpactEffects::kAirstrikeLifetime)<.0001f);
    assert(air_only.airstrike_charges()==0);
    assert(air_only.wave_status().aircraft_destroyed==1);

    // Aircraft steering is deterministic, bounded by the seeded center line,
    // and does not depend on frame splitting or the rendering eye.
    for(AircraftPattern pattern :
        {AircraftPattern::Bomber,AircraftPattern::Strafer}) {
        const float amplitude = pattern==AircraftPattern::Bomber?.24f:.76f;
        const AircraftFlightPose zero=AircraftFlightAt(pattern,1.2f,1.0f,-11.0f);
        assert(std::abs(zero.x-1.2f)<.0001f);
        assert(std::isfinite(zero.bank) && std::abs(zero.bank)<=.28f);
        for(float z=-11.0f;z<=2.4f;z+=.17f) {
            const AircraftFlightPose left=AircraftFlightAt(pattern,-.8f,1.0f,z);
            const AircraftFlightPose right=AircraftFlightAt(pattern,-.8f,-1.0f,z);
            assert(std::isfinite(left.x) && std::isfinite(left.bank));
            assert(std::abs(left.x+.8f)<=amplitude+.00001f);
            assert(std::abs(left.bank)<=.28f);
            assert(std::abs(left.x+right.x+1.6f)<.00001f);
            assert(std::abs(left.bank+right.bank)<.00001f);
        }
    }
    assert(AircraftFlightAt(AircraftPattern::None,1.2f,1.0f,-5.0f).x==1.2f);
    assert(AircraftFlightAt(AircraftPattern::Strafer,1.2f,0.0f,-5.0f).x==0);
    AircraftHitFeedback hit_feedback;
    Enemy no_aircraft;
    assert(!hit_feedback.Record(no_aircraft,no_aircraft,true));
    hit_feedback.Tick(std::numeric_limits<float>::infinity());
    assert(!hit_feedback.active());

    // Seed 8 deterministically opens late Blackcap with a bomber. Without AA
    // the gun still spends its round, but cannot damage the aircraft.
    Campaign bomber(Difficulty::Casual,8);
    assert(bomber.Start());
    stage_blackcap_night(bomber);
    assert(bomber.BeginWave());
    bomber.Tick(0.05f);
    assert(bomber.enemies()[0].active);
    assert(bomber.enemies()[0].type==UnitType::Aircraft);
    assert(bomber.enemies()[0].aircraft_pattern==AircraftPattern::Bomber);
    const AircraftAimFeedback unarmed_aim=InspectAircraftAim(bomber,0);
    assert(unarmed_aim.visible && !unarmed_aim.equipped);
    assert(unarmed_aim.hp==100 && unarmed_aim.pattern==AircraftPattern::Bomber);
    assert(!InspectAircraftAim(bomber,-1).visible);
    assert(!InspectAircraftAim(bomber,static_cast<int>(Campaign::kMaxEnemies)).visible);
    assert(!InspectAircraftAim(bomber,1).visible); // unspawned slot
    const Enemy &bomber_approach=bomber.enemies()[0];
    const AircraftFlightPose bomber_pose=AircraftFlightAt(
        bomber_approach.aircraft_pattern,bomber_approach.flight_center_x,
        bomber_approach.flight_sign,bomber_approach.z);
    assert(std::abs(bomber_approach.x-bomber_pose.x)<.000001f);
    assert(std::abs(bomber_approach.bank-bomber_pose.bank)<.000001f);
    const IncomingThreats approaching_bomber=bomber.incoming_threats();
    assert(approaching_bomber.aircraft_seconds>4.0f &&
           approaching_bomber.aircraft_seconds<6.0f);
    assert(approaching_bomber.gunboat_seconds<0.0f);
    assert(bomber.aircraft_attack().sequence==0);
    const int16_t bomber_hp=bomber.enemies()[0].hp;
    const uint8_t bomber_mag=bomber.magazine_left();
    const Enemy unarmed_before=bomber.enemies()[0];
    assert(bomber.FireAt(0));
    assert(!hit_feedback.Record(unarmed_before,bomber.enemies()[0],true));
    assert(!hit_feedback.active());
    assert(bomber.magazine_left()==bomber_mag-1);
    assert(bomber.enemies()[0].hp==bomber_hp);
    for(int i=0;i<140 && bomber.wave_status().aircraft_attacks==0;i++)
        bomber.Tick(0.05f);
    assert(bomber.wave_status().aircraft_attacks==1);
    const AircraftAttackEvent bomb=bomber.aircraft_attack();
    assert(bomb.sequence==1 && bomb.pattern==AircraftPattern::Bomber);
    assert(bomb.damage==12);
    assert(std::isfinite(bomb.x) && std::isfinite(bomb.z));
    assert(std::abs(bomb.x-bomber.enemies()[0].x)<.0001f);
    assert(std::abs(bomb.z-bomber.enemies()[0].z)<.0001f);
    ImpactEffects air_impact;
    const float bomb_origin[]={bomb.x,-.77f,-.93f};
    const float down[]={0.0f,-1.0f,0.0f};
    assert(air_impact.RecordHit(bomb_origin,down,.10f,ImpactKind::Explosion));
    assert(air_impact.pulses()[0].kind==ImpactKind::Explosion);
    assert(bomber.incoming_threats().aircraft_seconds<0.0f);
    assert(bomber.integrity()==88);
    // A lethal bomber hit reports only the integrity actually lost.
    Campaign fragile(Difficulty::Casual,8);
    assert(fragile.Start());
    stage_blackcap_night(fragile);
    uint8_t fragile_save[Campaign::kSerializedSize]={};
    assert(fragile.SerializeShop(fragile_save,sizeof(fragile_save)));
    fragile_save[36]=5; // little-endian integrity field (word 9)
    assert(fragile.DeserializeShop(fragile_save,sizeof(fragile_save)));
    assert(fragile.BeginWave());
    fragile.Tick(.05f);
    for(int i=0;i<140 && fragile.wave_status().aircraft_attacks==0;i++)
        fragile.Tick(.05f);
    assert(fragile.wave_status().aircraft_attacks==1);
    assert(fragile.integrity()==0 && fragile.phase()==Phase::Defeat);
    assert(fragile.aircraft_attack().sequence==1);
    assert(fragile.aircraft_attack().damage==5);

    BatteryDamageFeedback bombing_feedback;
    assert(bombing_feedback.Record(100,bomber.integrity()));
    assert(bombing_feedback.damage()==12);

    // Seed 22 opens the same stage with a strafer that peppers the position
    // repeatedly during one overhead pass instead of behaving like a sky-boat.
    Campaign strafer(Difficulty::Casual,22);
    assert(strafer.Start());
    stage_blackcap_night(strafer);
    assert(strafer.BeginWave());
    strafer.Tick(0.05f);
    assert(strafer.enemies()[0].active);
    assert(strafer.enemies()[0].aircraft_pattern==AircraftPattern::Strafer);
    const Enemy first_strafer=strafer.enemies()[0];
    const float first_x=first_strafer.x;
    for(int i=0;i<110 && strafer.wave_status().aircraft_attacks<4;i++)
        strafer.Tick(0.05f);
    assert(strafer.wave_status().aircraft_attacks==4);
    const Enemy late_strafer=strafer.enemies()[0];
    assert(std::abs(late_strafer.x-first_x)>.10f);
    assert(std::abs(late_strafer.x-late_strafer.flight_center_x)<=.76001f);
    const AircraftFlightPose late_pose=AircraftFlightAt(
        late_strafer.aircraft_pattern,late_strafer.flight_center_x,
        late_strafer.flight_sign,late_strafer.z);
    assert(std::abs(late_pose.x-late_strafer.x)<.000001f);
    assert(std::abs(late_pose.bank-late_strafer.bank)<.000001f);
    const AircraftAttackEvent strafe=strafer.aircraft_attack();
    assert(strafe.sequence==4 && strafe.pattern==AircraftPattern::Strafer);
    assert(strafe.damage==3);
    assert(std::isfinite(strafe.x) && std::isfinite(strafe.z));
    assert(strafer.integrity()==88);

    // A single 0.10-second headset frame and two 0.05-second updates
    // arrive at the same visual/target pose, including the aircraft bank.
    Campaign slow_flight(Difficulty::Casual,22);
    Campaign split_flight(Difficulty::Casual,22);
    assert(slow_flight.Start() && split_flight.Start());
    stage_blackcap_night(slow_flight);
    stage_blackcap_night(split_flight);
    assert(slow_flight.BeginWave() && split_flight.BeginWave());
    for(int step=0;step<24;step++) {
        slow_flight.Tick(.10f);
        split_flight.Tick(.05f);
        split_flight.Tick(.05f);
        const Enemy &slow=slow_flight.enemies()[0];
        const Enemy &split=split_flight.enemies()[0];
        assert(slow.type==UnitType::Aircraft && split.type==UnitType::Aircraft);
        assert(slow.x==split.x && slow.z==split.z && slow.bank==split.bank);
        assert(slow.hp==split.hp);
    }

    // Purchasing AA turns those otherwise ineffective shots into damage and
    // can destroy the bomber before its attack window begins.
    Campaign defended(Difficulty::Casual,8);
    assert(defended.Start());
    assert(defended.Purchase(Upgrade::AntiAir));
    stage_blackcap_night(defended);
    assert(defended.BeginWave());
    defended.Tick(0.05f);
    assert(defended.enemies()[0].type==UnitType::Aircraft);
    const AircraftAimFeedback armed_aim=InspectAircraftAim(defended,0);
    assert(armed_aim.visible && armed_aim.equipped && armed_aim.hp==100);
    for(int shot=0;shot<5;shot++) {
        if(shot>0) for(int i=0;i<7;i++) defended.Tick(0.05f);
        const Enemy before=defended.enemies()[0];
        assert(defended.FireAt(0));
        assert(hit_feedback.Record(before,defended.enemies()[0],true));
        assert(hit_feedback.damage()==20);
        assert(std::abs(hit_feedback.x()-before.x)<.000001f);
        assert(std::abs(hit_feedback.z()-before.z)<.000001f);
        assert(hit_feedback.destroyed()==(shot==4));
        assert(hit_feedback.active());
    }
    assert(!InspectAircraftAim(defended,0).visible);
    hit_feedback.Tick(.54f);
    assert(hit_feedback.active());
    hit_feedback.Tick(.02f);
    assert(!hit_feedback.active());
    assert(!defended.enemies()[0].active);
    assert(defended.incoming_threats().aircraft_seconds<0.0f);
    assert(defended.wave_status().aircraft_destroyed==1);
    assert(defended.wave_status().aircraft_attacks==0);
    assert(defended.aircraft_attack().sequence==0);
    assert(defended.integrity()==100);

    // Non-aircraft targets cannot generate false aircraft brackets or hits.
    Campaign no_target(Difficulty::Casual,41);
    assert(no_target.Start() && no_target.BeginWave());
    no_target.Tick(.05f);
    assert(no_target.enemies()[0].type==UnitType::Rowboat);
    assert(!InspectAircraftAim(no_target,0).visible);
    assert(!InspectAircraftAim(no_target,255).visible);
    // Recorded weapon damage must remain tied to a fired, real aircraft hit.
    AircraftHitFeedback ignored_hit;
    const Enemy already_down=defended.enemies()[0];
    assert(!ignored_hit.Record(already_down,already_down,true));
    assert(!ignored_hit.Record(unarmed_before,defended.enemies()[0],false));
    assert(!ignored_hit.active());

    // AA is a real three-tier investment: a basic engagement kit, then
    // +15 / +30 aircraft-only damage. The shop shows prices 750/1050/1350
    // and saved campaigns retain the full tier without changing save size.
    auto funded_aa=[&](uint8_t tiers) {
        Campaign game(Difficulty::Casual,8);
        assert(game.Start());
        uint8_t save[Campaign::kSerializedSize]={};
        assert(game.SerializeShop(save,sizeof(save)));
        // Supply earned campaign credits in this deterministic shop fixture.
        save[32]=0x10; save[33]=0x27; // 10000, little-endian word 8
        assert(game.DeserializeShop(save,sizeof(save)));
        assert(game.price(Upgrade::AntiAir)==750);
        assert(game.stats().anti_air_damage_bonus==0);
        assert(game.stats().gun_damage==20);
        assert(!game.stats().anti_air);
        for(uint8_t tier=0;tier<tiers;++tier) {
            assert(game.price(Upgrade::AntiAir)==750+300*tier);
            assert(game.Purchase(Upgrade::AntiAir));
        }
        assert(game.level(Upgrade::AntiAir)==tiers);
        assert(game.stats().anti_air==(tiers>0));
        assert(game.stats().anti_air_damage_bonus==
               (tiers>1?15*(tiers-1):0));
        assert(game.stats().gun_damage==20); // no surface-weapon buff
        assert(game.credits()==10000-(tiers==0?0:
            tiers==1?750:tiers==2?1800:3150));
        if(tiers==3) {
            assert(game.price(Upgrade::AntiAir)==-1);
            assert(!game.Purchase(Upgrade::AntiAir));
        }
        assert(game.SerializeShop(save,sizeof(save)));
        Campaign loaded;
        assert(loaded.DeserializeShop(save,sizeof(save)));
        assert(loaded.level(Upgrade::AntiAir)==tiers);
        assert(loaded.stats().anti_air_damage_bonus==
               game.stats().anti_air_damage_bonus);
        if(tiers==3) {
            // Invalid tier 4 is rejected transactionally; the live shop stays
            // maxed and retains its credits and legal upgraded damage.
            save[60]=4; // upgrade[3] at serialized word 15
            assert(!loaded.DeserializeShop(save,sizeof(save)));
            assert(loaded.level(Upgrade::AntiAir)==3);
            assert(loaded.credits()==game.credits());
        }
        return game;
    };
    Campaign aa_tier1=funded_aa(1);
    Campaign aa_tier2=funded_aa(2);
    Campaign aa_tier3=funded_aa(3);
    // All three start the identical deterministic bomber encounter.
    // Higher AA tiers intercept with fewer shots, without consuming airstrikes
    // or changing wave attack timing and without changing shell controls.
    auto engage_bomber=[&](Campaign &game,uint8_t shots) {
        stage_blackcap_night(game);
        assert(game.BeginWave());
        game.Tick(.05f);
        assert(game.enemies()[0].type==UnitType::Aircraft);
        assert(game.enemies()[0].hp==100);
        for(uint8_t shot=0;shot<shots;shot++) {
            if(shot>0) for(int i=0;i<7;i++) game.Tick(.05f);
            assert(game.FireAt(0));
            assert(game.enemies()[0].active==(shot+1<shots));
        }
        assert(game.wave_status().aircraft_destroyed==1);
        assert(game.incoming_threats().aircraft_seconds<0.0f);
        assert(game.integrity()==100);
    };
    engage_bomber(aa_tier1,5); // 20 damage
    engage_bomber(aa_tier2,3); // 35 damage
    engage_bomber(aa_tier3,2); // 50 damage

    // One Select press remains captured across UI drift for both the switch
    // strip and SAM launcher, never accidentally firing the basic gun.
    CombatInputLatch weapon_latch;
    assert(weapon_latch.Route(CombatHudTarget::Weapon3,true)==
           CombatInputRoute::Weapon3);
    assert(weapon_latch.Route(CombatHudTarget::None,false)==
           CombatInputRoute::NoAction);
    weapon_latch.Release();
    assert(weapon_latch.Route(CombatHudTarget::Sam,true)==CombatInputRoute::Sam);
    assert(weapon_latch.Route(CombatHudTarget::None,false)==
           CombatInputRoute::NoAction);
    weapon_latch.Release();
    assert(weapon_latch.Route(CombatHudTarget::None,true)==CombatInputRoute::Gun);

    // Armed SAM consumes exactly one Select edge, including an invalid lock.
    // Neither pointer drift nor continued holding may fire the main gun.
    CombatInputLatch sam_fire_latch;
    assert(!sam_fire_latch.CaptureOneShotPress(false));
    assert(sam_fire_latch.Route(CombatHudTarget::None,false)==CombatInputRoute::Gun);
    assert(sam_fire_latch.CaptureOneShotPress(true));
    assert(!sam_fire_latch.CaptureOneShotPress(true));
    assert(sam_fire_latch.Route(CombatHudTarget::None,false)==CombatInputRoute::NoAction);
    assert(sam_fire_latch.Route(CombatHudTarget::Artillery,false)==CombatInputRoute::NoAction);
    sam_fire_latch.Release();
    assert(sam_fire_latch.CaptureOneShotPress(true)); // failed lock is still spent
    assert(sam_fire_latch.Route(CombatHudTarget::None,false)==CombatInputRoute::NoAction);
    sam_fire_latch.Release();
    assert(sam_fire_latch.Route(CombatHudTarget::None,true)==CombatInputRoute::Gun);

    // Weapon ownership, selection and SAM supplies survive the compact
    // 80-byte v2 shop save, while v1 saves still load as a basic machine gun.
    Campaign arsenal(Difficulty::Casual,8);
    assert(arsenal.Start());
    uint8_t arsenal_save[Campaign::kSerializedSize]={};
    assert(arsenal.SerializeShop(arsenal_save,sizeof(arsenal_save)));
    arsenal_save[32]=0x10; arsenal_save[33]=0x27; // 10000 shop credits
    assert(arsenal.DeserializeShop(arsenal_save,sizeof(arsenal_save)));
    assert(arsenal.owns_weapon(WeaponKind::MachineGun));
    assert(!arsenal.owns_weapon(WeaponKind::Flak));
    assert(!arsenal.SelectWeapon(WeaponKind::MissileBattery));
    assert(arsenal.PurchaseWeapon(WeaponKind::Flak));
    assert(arsenal.selected_weapon()==WeaponKind::Flak);
    assert(arsenal.stats().anti_air);
    assert(arsenal.stats().projectile_gravity==.001f);
    assert(arsenal.PurchaseWeapon(WeaponKind::MissileBattery));
    assert(arsenal.selected_weapon()==WeaponKind::MissileBattery);
    assert(arsenal.stats().magazine==2);
    assert(arsenal.PurchaseWeapon(WeaponKind::DirectCannon));
    assert(arsenal.stats().projectile_gravity==.026f);
    assert(arsenal.PurchaseSam());
    assert(arsenal.sam_charges()==1);
    assert(arsenal.SerializeShop(arsenal_save,sizeof(arsenal_save)));
    Campaign loaded_arsenal;
    assert(loaded_arsenal.DeserializeShop(arsenal_save,sizeof(arsenal_save)));
    assert(loaded_arsenal.owns_weapon(WeaponKind::MissileBattery));
    assert(loaded_arsenal.selected_weapon()==WeaponKind::DirectCannon);
    assert(loaded_arsenal.sam_charges()==1);
    arsenal_save[64]=0; // zero owned mask invalid in v2
    assert(!loaded_arsenal.DeserializeShop(arsenal_save,sizeof(arsenal_save)));
    assert(loaded_arsenal.owns_weapon(WeaponKind::MissileBattery));
    arsenal_save[64]=static_cast<uint8_t>(31);
    arsenal_save[68]=9; // invalid selected weapon
    assert(!loaded_arsenal.DeserializeShop(arsenal_save,sizeof(arsenal_save)));
    assert(loaded_arsenal.sam_charges()==1);
    // The original v1 binary format must still restore the default MG.
    Campaign legacy(Difficulty::Casual,9);
    assert(legacy.Start());
    uint8_t legacy_save[Campaign::kSerializedSize]={};
    assert(legacy.SerializeShop(legacy_save,sizeof(legacy_save)));
    legacy_save[4]=1; legacy_save[64]=0; legacy_save[68]=0;
    legacy_save[76]=0;
    Campaign old_loaded;
    assert(old_loaded.DeserializeShop(legacy_save,sizeof(legacy_save)));
    assert(old_loaded.selected_weapon()==WeaponKind::MachineGun);
    assert(!old_loaded.owns_weapon(WeaponKind::Flak));
    assert(old_loaded.sam_charges()==0);
    // A rejected SAM target does not consume a charge; a valid aircraft
    // interception consumes one charge and destroys exactly one aircraft.
    Campaign sam_game(Difficulty::Casual,8);
    assert(sam_game.Start() && sam_game.PurchaseSam());
    stage_blackcap_night(sam_game);
    assert(sam_game.BeginWave());
    sam_game.Tick(.05f);
    assert(sam_game.enemies()[0].active &&
           sam_game.enemies()[0].type==UnitType::Aircraft);
    assert(!sam_game.CallSam(1)); // unoccupied slot
    assert(sam_game.sam_charges()==1);
    assert(sam_game.CallSam(0));
    assert(sam_game.sam_charges()==0);
    assert(!sam_game.enemies()[0].active);
    assert(sam_game.wave_status().aircraft_destroyed==1);
    assert(!sam_game.CallSam(0)); // cannot double-launch
    assert(loaded_arsenal.BeginWave());
    loaded_arsenal.Tick(.05f);
    assert(!loaded_arsenal.SelectWeapon(WeaponKind::HeavyGun));
    assert(loaded_arsenal.SelectWeapon(WeaponKind::MachineGun));
    assert(loaded_arsenal.selected_weapon()==WeaponKind::MachineGun);
    // Switch to the owned flak while the default gun is on cooldown.
    assert(loaded_arsenal.SelectWeapon(WeaponKind::Flak));
    assert(loaded_arsenal.stats().anti_air);
    assert(loaded_arsenal.SelectWeapon(WeaponKind::MissileBattery));
    assert(!loaded_arsenal.FireMiss()); // missiles require genuine lock
    assert(loaded_arsenal.SelectWeapon(WeaponKind::MachineGun));

    // An actual later-location landing craft carries an independently
    // simulated armored car. Only a *surviving* transport can unload it,
    // and the wave cannot complete while any armor survives ashore.
    uint32_t armored_seed=0u;
    for(uint32_t seed=1;seed<256;seed++) {
        Campaign probe(Difficulty::Regular,seed);
        assert(probe.Start());
        uint8_t shop[Campaign::kSerializedSize]={};
        assert(probe.SerializeShop(shop,sizeof(shop)));
        shop[16]=1u; // Iron Quay day 1: armored transports enter the roster
        assert(probe.DeserializeShop(shop,sizeof(shop)));
        assert(probe.BeginWave());
        probe.Tick(.05f);
        if(probe.enemies()[0].active &&
           probe.enemies()[0].type==UnitType::LandingCraft &&
           probe.enemies()[0].carries_armor) {
            armored_seed=seed;
            break;
        }
    }
    assert(armored_seed!=0u);
    auto armorGame=[&](uint32_t seed) {
        Campaign game(Difficulty::Regular,seed);
        assert(game.Start());
        uint8_t save[Campaign::kSerializedSize]={};
        assert(game.SerializeShop(save,sizeof(save)));
        save[16]=1u;
        assert(game.DeserializeShop(save,sizeof(save)));
        assert(game.PurchaseWeapon(WeaponKind::HeavyGun));
        assert(game.BeginWave());
        game.Tick(.05f);
        assert(game.enemies()[0].carries_armor);
        return game;
    };
    Campaign arrived=armorGame(armored_seed);
    Campaign intercepted=armorGame(armored_seed);
    assert(!arrived.FireAtArmor(0)); // no imaginary shore vehicles
    // Destroy cargo transport before its ramp opens. Three heavy rounds
    // are sufficient; the original wave still spawns other real craft.
    for(int shot=0;shot<3;++shot) {
        assert(intercepted.FireAt(0));
        if(shot<2)for(int t=0;t<9;++t)intercepted.Tick(.05f);
    }
    assert(!intercepted.enemies()[0].active);
    assert(!intercepted.enemies()[0].landed);
    for(int frame=0;frame<380;++frame) {
        arrived.Tick(.05f);
        intercepted.Tick(.05f);
    }
    assert(arrived.wave_status().armor_landed>0);
    assert(arrived.wave_status().armor_landed==
           intercepted.wave_status().armor_landed+1u);
    uint8_t car_index=static_cast<uint8_t>(Campaign::kMaxArmor);
    for(uint8_t i=0;i<Campaign::kMaxArmor;++i)
        if(arrived.armored_vehicles()[i].active) {car_index=i;break;}
    assert(car_index<Campaign::kMaxArmor);
    assert(arrived.armored_vehicles()[car_index].hp==armor::kVehicleHp);
    assert(arrived.armored_vehicles()[car_index].z<=armor::kArmorLineZ);
    // Native HUD warning is computed from the actual deployed combat
    // actor and is absent before any armored car reaches shore.
    assert(arrived.incoming_threats().armor_seconds>=0.0f);
    assert(arrived.incoming_threats().armor_seconds<=armor::kArmorFireInterval);
    assert(!intercepted.FireAtArmor(static_cast<uint8_t>(Campaign::kMaxArmor)));
    const int original_armor_hp=arrived.armored_vehicles()[car_index].hp;
    assert(arrived.FireAtArmor(car_index));
    assert(arrived.armored_vehicles()[car_index].hp==
           original_armor_hp-armor::ArmorDamage(WeaponKind::HeavyGun,
                                                arrived.stats().gun_damage));
    // A real surviving armored car still blocks wave completion; a
    // destroyed one stops firing and pays its kill reward once.
    for(int shot=0;shot<5;++shot) {
        for(int t=0;t<9;++t)arrived.Tick(.05f);
        assert(arrived.FireAtArmor(car_index));
    }
    assert(!arrived.armored_vehicles()[car_index].active);
    assert(arrived.wave_status().armor_destroyed>=1);
    assert(!arrived.FireAtArmor(car_index));

    // Actual final-day flagship, not a decorative mesh or early-game
    // substitution. Two vehicles remain cargo until a true live landfall.
    Campaign flagship(Difficulty::Casual,2026u);
    assert(flagship.Start());
    uint8_t late_save[Campaign::kSerializedSize]={};
    assert(flagship.SerializeShop(late_save,sizeof(late_save)));
    late_save[16]=2u; // Blackcap Island
    late_save[20]=4u; // final day
    late_save[28]=19u; // final daytime wave on Cadet
    late_save[32]=0x10u;late_save[33]=0x27u; // 10,000 credits
    assert(flagship.DeserializeShop(late_save,sizeof(late_save)));
    assert(flagship.waves_this_period()==19u);
    assert(flagship.PurchaseWeapon(WeaponKind::DirectCannon));
    assert(flagship.BeginWave());
    flagship.Tick(.05f);
    assert(flagship.enemies()[0].active);
    assert(flagship.enemies()[0].type==UnitType::ArmoredTransport);
    assert(flagship.enemies()[0].hp==320);
    assert(flagship.enemies()[0].passengers==4u);
    assert(flagship.enemies()[0].carries_armor);
    assert(flagship.wave_status().armor_landed==0);
    assert(flagship.armored_vehicles()[0].active==false);
    // The second actual spawned attacker is a fast gunboat escort in a
    // separate lane, not merely a scripted label or visual decoration.
    for(int n=0;n<35;++n)flagship.Tick(.05f);
    assert(flagship.phase()==Phase::Combat);
    assert(flagship.enemies()[0].active);
    assert(flagship.enemies()[1].active);
    assert(flagship.enemies()[1].type==UnitType::Gunboat);
    assert(std::abs(flagship.enemies()[1].x-
                    invasion::FlagshipEscortLane(
                        flagship.enemies()[0].x))<.00001f);
    Campaign flagshipSunk=flagship;
    // Four correctly timed direct cannon hits defeat the actual armored
    // ship before landfall. Killing it prevents both cars and its troop
    // complement from being spawned by Land().
    for(int shot=0;shot<4;++shot) {
        assert(flagshipSunk.FireAt(0));
        if(shot<3)for(int ticks=0;ticks<10;++ticks)
            flagshipSunk.Tick(.10f);
    }
    assert(!flagshipSunk.enemies()[0].active);
    assert(flagshipSunk.wave_status().boats_sunk==1u);
    // Compare deterministic copies through the same elapsed invasion;
    // both other spawn streams are unchanged. The sunk heavy transport
    // suppresses precisely its two vehicle cargo slots.
    bool observed_ramp_hold=false;
    for(int n=0;n<235;++n) {
        flagship.Tick(.10f);
        flagshipSunk.Tick(.10f);
        const Enemy &ship=flagship.enemies()[0];
        if(flagship.phase()==Phase::Combat && ship.active &&
           ship.type==UnitType::ArmoredTransport && ship.landed) {
            observed_ramp_hold=true;
            assert(ship.z==invasion::BeachingZ(ship.type));
            assert(ship.attack_timer>0.0f);
            assert(ship.z+1.28f+.18f>-2.23f);
            assert(ship.passengers==4u);
        }
        if(flagship.phase()==Phase::Defeat ||
           flagshipSunk.phase()==Phase::Defeat)break;
    }
    // A lost battery can end the wave before the heavy reaches shore.
    (void)observed_ramp_hold;
    if(flagship.phase()!=Phase::Defeat &&
       flagshipSunk.phase()!=Phase::Defeat) {
        assert(flagship.wave_status().armor_landed>=2u);
        assert(flagship.wave_status().armor_landed==
               flagshipSunk.wave_status().armor_landed+2u);
    }

    // Day and night each contain their own shop-separated waves.
    Campaign clock(Difficulty::Casual,41);
    assert(clock.Start());
    assert(clock.waves_this_period()==12);
    uint8_t day_end_save[Campaign::kSerializedSize]={};
    assert(clock.SerializeShop(day_end_save,sizeof(day_end_save)));
    day_end_save[28]=clock.waves_this_period();
    // Start at the last (12th) day wave to exercise the real end-of-period
    // transition without spending minutes replaying eleven prior assaults.
    assert(clock.DeserializeShop(day_end_save,sizeof(day_end_save)));
    assert(clock.wave_index()==12);
    for(int i=0;i<7200 && clock.period()!=Period::Night &&
                    clock.phase()!=Phase::Defeat;i++) {
        if(clock.phase()==Phase::Shop) assert(clock.BeginWave());
        clock.Tick(0.05f);
        for(uint8_t n=0;n<clock.enemies().size();n++)
            if(clock.enemies()[n].active) clock.FireAt(n);
        if(clock.infantry()>0)
            for(uint8_t i=0;i<clock.infantry_units().size();++i)
                if(clock.infantry_units()[i].active && clock.FireAtInfantry(i)) break;
    }
    assert(clock.period()==Period::Night && clock.day()==1);
    assert(clock.wave_index()==1);
    assert(clock.waves_this_period()==8);

    // Every shipped difficulty gets a distinct long campaign without
    // exceeding the uint8_t wave index or the original save envelope.
    assert(Campaign::Locations()[0].days==2);
    assert(Campaign::Locations()[1].days==3);
    assert(Campaign::Locations()[2].days==4);
    std::cout << "Breakwater boat / landing / upgrade smoke passed\n";
}
