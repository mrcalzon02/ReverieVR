#ifndef REVERIE_RECON_PROJECTS_SAVE_H
#define REVERIE_RECON_PROJECTS_SAVE_H
#include "lantern_desk_simulation.h"
#include "forward_detachment_simulation.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace reverie { namespace recon {
struct LanternState { lantern::Simulation simulation; uint8_t choice=0; };
struct ForwardState { forward::Simulation simulation; uint8_t choice=0; bool confirm_refit=false; };
// Game-private checkpoint for the existing two decision-board prototypes.
// Explicit fields, separate kind tags, full integrity and transactional reads.
class Checkpoint {
public:
    static constexpr size_t kBytes=128;
    using Bytes=std::array<uint8_t,kBytes>;
    static bool Encode(const LanternState& state,Bytes* output){
        if(!output||!Valid(state))return false;
        Bytes b=Start(0);const auto& s=state.simulation;
        b[12]=static_cast<uint8_t>(s.phase_);b[13]=static_cast<uint8_t>(s.selected_);
        b[14]=static_cast<uint8_t>(s.photo_.sector);b[15]=s.photo_.clarity;
        for(size_t i=0;i<16;++i)Put16(&b[16+2*i],s.photo_.pixels[i]);
        PutI(&b[48],s.credibility_);PutI(&b[52],s.operational_effect_);
        PutI(&b[56],s.last_report_.credibility_delta);PutI(&b[60],s.last_report_.operational_effect);
        b[64]=state.choice;Finish(b);*output=b;return true;
    }
    static bool Decode(LanternState* output,const uint8_t* b,size_t size){
        if(!output||!Header(b,size,0)||!Padding(b,65))return false;
        LanternState next;auto& s=next.simulation;
        s.phase_=static_cast<lantern::Phase>(b[12]);s.selected_=static_cast<lantern::Sector>(b[13]);
        s.photo_.sector=static_cast<lantern::Sector>(b[14]);s.photo_.clarity=b[15];
        for(size_t i=0;i<16;++i)s.photo_.pixels[i]=Get16(&b[16+2*i]);
        s.credibility_=GetI(&b[48]);s.operational_effect_=GetI(&b[52]);
        s.last_report_.credibility_delta=GetI(&b[56]);s.last_report_.operational_effect=GetI(&b[60]);
        next.choice=b[64];
        if(!Valid(next))return false;
        *output=next;return true;
    }
    static bool Encode(const ForwardState& state,Bytes* output){
        if(!output||!Valid(state))return false;
        Bytes b=Start(1);const auto& s=state.simulation;
        b[12]=static_cast<uint8_t>(s.phase_);b[13]=static_cast<uint8_t>(s.route_);
        b[14]=static_cast<uint8_t>(s.decision_);b[15]=s.unnecessary_force_?1:0;
        PutI(&b[16],s.vehicle_condition_);PutI(&b[20],s.fuel_remaining_);PutI(&b[24],s.information_value_);
        b[28]=state.choice;b[29]=state.confirm_refit?1:0;
        Finish(b);*output=b;return true;
    }
    static bool Decode(ForwardState* output,const uint8_t* b,size_t size){
        if(!output||!Header(b,size,1)||!Padding(b,30)||b[15]>1||b[29]>1)return false;
        ForwardState next;auto& s=next.simulation;
        s.phase_=static_cast<forward::Phase>(b[12]);s.route_=static_cast<forward::Route>(b[13]);
        s.decision_=static_cast<forward::Decision>(b[14]);s.unnecessary_force_=b[15]!=0;
        s.vehicle_condition_=GetI(&b[16]);s.fuel_remaining_=GetI(&b[20]);s.information_value_=GetI(&b[24]);
        next.choice=b[28];next.confirm_refit=b[29]!=0;
        if(!Valid(next))return false;
        *output=next;return true;
    }
private:
    static bool SamePhoto(const lantern::Photograph& a,const lantern::Photograph& b){
        return a.sector==b.sector&&a.clarity==b.clarity&&a.pixels==b.pixels;
    }
    static bool Valid(const LanternState& state){
        const auto& s=state.simulation;
        if(static_cast<unsigned>(s.phase_)>3||static_cast<unsigned>(s.selected_)>2)return false;
        const int choices=s.phase_==lantern::Phase::Briefing?3:s.phase_==lantern::Phase::PhotoReady?4:1;
        if(state.choice>=choices)return false;
        if(s.phase_==lantern::Phase::Briefing||s.phase_==lantern::Phase::InFlight){
            if(!SamePhoto(s.photo_,lantern::Photograph{})||s.last_report_.credibility_delta
                ||s.last_report_.operational_effect)return false;
            return s.phase_!=lantern::Phase::Briefing||s.selected_==lantern::Sector::RailApproach;
        }
        lantern::Simulation expected;
        if(!expected.AssignRecon(s.selected_)||!expected.CompleteRecon()
            ||!SamePhoto(s.photo_,*expected.photo()))return false;
        if(s.phase_==lantern::Phase::PhotoReady)
            return !s.last_report_.credibility_delta&&!s.last_report_.operational_effect;
        // A filed display retains the actual adjudicated result, not an
        // invented result that could be replayed against cumulative totals.
        for(unsigned i=0;i<4;++i){
            auto report=expected;lantern::ReportResult result;
            if(report.SubmitReport(static_cast<lantern::Assessment>(i),&result)
                &&result.credibility_delta==s.last_report_.credibility_delta
                &&result.operational_effect==s.last_report_.operational_effect)return true;
        }
        return false;
    }
    static bool Valid(const ForwardState& state){
        const auto& s=state.simulation;
        if(static_cast<unsigned>(s.phase_)>5||static_cast<unsigned>(s.route_)>1
            ||static_cast<unsigned>(s.decision_)>2||s.vehicle_condition_<0||s.vehicle_condition_>100
            ||s.fuel_remaining_<0||s.fuel_remaining_>20)return false;
        const bool briefing=s.phase_==forward::Phase::Briefing;
        if(state.confirm_refit&&!briefing)return false;
        const int choices=state.confirm_refit?2:briefing?3:s.phase_==forward::Phase::Contact?3:1;
        if(state.choice>=choices)return false;
        const int leg=forward::Simulation::RoundTripFuel(s.route_)/2;
        if(briefing||s.phase_==forward::Phase::Travelling||s.phase_==forward::Phase::Contact){
            if(s.decision_!=forward::Decision::Avoid||s.information_value_||s.unnecessary_force_)return false;
            if(briefing)return s.route_==forward::Route::MainRoad;
            if(s.vehicle_condition_<=0)return false;
            if(s.phase_==forward::Phase::Travelling)return s.fuel_remaining_>=2*leg;
            return s.fuel_remaining_>=leg&&s.fuel_remaining_<=20-leg;
        }
        if(s.phase_==forward::Phase::Resolved&&(s.fuel_remaining_<leg||s.fuel_remaining_>20-leg))return false;
        if(s.phase_!=forward::Phase::Resolved&&s.fuel_remaining_>20-2*leg)return false;
        const int value=s.route_==forward::Route::MainRoad?
            (s.decision_==forward::Decision::HaltAndReport?3:s.decision_==forward::Decision::Avoid?1:-3):
            (s.decision_==forward::Decision::HaltAndReport?-1:s.decision_==forward::Decision::Avoid?2:-5);
        if(s.decision_==forward::Decision::Engage
            &&s.vehicle_condition_>(s.route_==forward::Route::MainRoad?75:65))return false;
        return s.information_value_==value
            &&s.unnecessary_force_==(s.decision_==forward::Decision::Engage)
            &&(s.vehicle_condition_>0||s.unnecessary_force_);
    }
    static Bytes Start(uint8_t kind){
        Bytes b{};std::memcpy(b.data(),"RECON001",8);b[8]=1;b[9]=kind;return b;
    }
    static uint64_t Hash(const uint8_t* b){
        uint64_t hash=14695981039346656037ull;
        for(size_t i=0;i<kBytes-8;++i)hash=(hash^b[i])*1099511628211ull;
        return hash;
    }
    static void Finish(Bytes& b){
        uint64_t hash=Hash(b.data());
        for(size_t i=kBytes-8;i<kBytes;++i){b[i]=static_cast<uint8_t>(hash);hash>>=8;}
    }
    static bool Header(const uint8_t* b,size_t size,uint8_t kind){
        if(!b||size!=kBytes||std::memcmp(b,"RECON001",8)||b[8]!=1||b[9]!=kind||b[10]||b[11])return false;
        uint64_t hash=Hash(b);
        for(size_t i=kBytes-8;i<kBytes;++i){if(b[i]!=static_cast<uint8_t>(hash))return false;hash>>=8;}
        return true;
    }
    static bool Padding(const uint8_t* b,size_t from){
        for(size_t i=from;i<kBytes-8;++i)if(b[i])return false;
        return true;
    }
    static void Put16(uint8_t* p,uint16_t value){p[0]=static_cast<uint8_t>(value);p[1]=static_cast<uint8_t>(value>>8);}
    static uint16_t Get16(const uint8_t* p){return static_cast<uint16_t>(p[0]|uint16_t(p[1])<<8);}
    static void PutI(uint8_t* p,int32_t value){
        uint32_t bits;std::memcpy(&bits,&value,4);
        for(unsigned i=0;i<4;++i)p[i]=static_cast<uint8_t>(bits>>(i*8));
    }
    static int32_t GetI(const uint8_t* p){
        uint32_t bits=0;for(unsigned i=0;i<4;++i)bits|=uint32_t(p[i])<<(i*8);
        int32_t value;std::memcpy(&value,&bits,4);return value;
    }
};
}}
#endif
