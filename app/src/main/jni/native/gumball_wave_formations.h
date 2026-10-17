#ifndef REVERIE_GUMBALL_WAVE_FORMATIONS_H
#define REVERIE_GUMBALL_WAVE_FORMATIONS_H
#include <cstdint>
#include <cmath>
namespace reverie { namespace gumball {
// The five slots in an assault are an authored formation, not five random
// independent X coordinates. Waves cycle the eight doctrines per lane.
enum class WaveFormation : uint8_t {
    Column, Fan, Pincer, Crossfire, Spiral, RearPressure, Siege, Escort
};
struct WaveEntry {
    float x=0.f,y=1.35f,next_delay=.94f;
    WaveFormation formation=WaveFormation::Column;
};
inline WaveFormation FormationFor(uint32_t seed,uint32_t lane,int spawn){
    uint32_t salt=seed^(lane*0x9E3779B9u);
    salt^=salt>>16;salt*=0x7feb352du;salt^=salt>>15;
    return static_cast<WaveFormation>((salt%8u+
        static_cast<unsigned>(spawn>=0?spawn:0)/5u)%8u);
}
inline const char* FormationName(WaveFormation formation){
    switch(formation){
    case WaveFormation::Column:return "COLUMN";
    case WaveFormation::Fan:return "FAN";
    case WaveFormation::Pincer:return "PINCER";
    case WaveFormation::Crossfire:return "CROSS-FIRE";
    case WaveFormation::Spiral:return "SPIRAL";
    case WaveFormation::RearPressure:return "REAR PRESSURE";
    case WaveFormation::Siege:return "SIEGE";
    case WaveFormation::Escort:return "ESCORT";
    }
    return "UNKNOWN";
}
inline WaveEntry PlanWaveEntry(uint32_t seed,uint32_t lane,int spawn,
                              int total,uint32_t variation){
    static constexpr float positions[8][5]={
        { 0.f, 0.f, 0.f, 0.f, 0.f},
        { 0.f,-.77f, .77f,-1.54f, 1.54f},
        {-1.55f,1.55f,-1.19f,1.19f,0.f},
        {-1.50f,1.50f,0.f,1.04f,-1.04f},
        { 0.f,.68f,1.36f,.36f,-.90f},
        { 0.f,-1.23f,1.23f,-.60f,.60f},
        { 0.f,-.37f,.37f,-.74f,.74f},
        {-1.12f,1.12f,0.f,-1.57f,1.57f}
    };
    const int safeSpawn=spawn<0?0:spawn;
    const unsigned slot=static_cast<unsigned>(safeSpawn)%5u;
    const WaveFormation pattern=FormationFor(seed,lane,safeSpawn);
    const unsigned kind=static_cast<unsigned>(pattern);
    const float jitter=(static_cast<float>(variation%13u)-6.f)*.007f;
    WaveEntry result;
    result.formation=pattern;
    result.x=positions[kind][slot]+jitter;
    result.y=pattern==WaveFormation::RearPressure?.98f:
        (pattern==WaveFormation::Crossfire?1.23f:1.35f);
    // Close-spaced assaults are legible formations; break before the next.
    const bool endOfGroup=slot==4u||safeSpawn+1>=total;
    result.next_delay=endOfGroup?2.90f:
        (pattern==WaveFormation::RearPressure?.73f:.81f);
    return result;
}
// Flight behaviors for ordinary Sour ships. Specials, named rivals and
// flagships retain their own movement and firing rules. Each output is
// finite and bounded, so formations do not depend on player firing state.
struct WaveTactics {
    float lateral_speed=0.f;
    float shot_aim=0.f;
    float fire_interval_scale=1.f;
};
inline float WaveClamp(float value,float limit){
    return value>limit?limit:(value< -limit?-limit:value);
}
inline WaveTactics TacticsFor(WaveFormation formation,unsigned slot,
                             float age,float x,float playerX){
    WaveTactics tactics;
    if(!std::isfinite(age)||!std::isfinite(x)||!std::isfinite(playerX))
        return tactics;
    const float track=WaveClamp(playerX-x,1.f);
    const float side=x<-.12f?-1.f:(x>.12f?1.f:0.f);
    switch(formation){
    case WaveFormation::Column: break; // disciplined straight line
    case WaveFormation::Fan:
        tactics.lateral_speed=side*.12f;
        tactics.shot_aim=side*.12f;
        break;
    case WaveFormation::Pincer:
        tactics.lateral_speed=track*.31f;
        tactics.shot_aim=track*.16f;
        break;
    case WaveFormation::Crossfire:
        tactics.lateral_speed=(slot%2u==0u?1.f:-1.f)*.21f;
        tactics.shot_aim=-side*.20f;
        break;
    case WaveFormation::Spiral:
        tactics.lateral_speed=std::sin(age*3.2f+static_cast<float>(slot)*.83f)*.43f;
        tactics.shot_aim=std::sin(age*2.0f+static_cast<float>(slot))*.14f;
        break;
    case WaveFormation::RearPressure:
        tactics.lateral_speed=track*.51f;
        tactics.shot_aim=track*.22f;
        tactics.fire_interval_scale=.93f;
        break;
    case WaveFormation::Siege:
        tactics.shot_aim=-side*.11f;
        tactics.fire_interval_scale=.94f;
        break;
    case WaveFormation::Escort:
        if(slot!=2u)tactics.lateral_speed=WaveClamp(-x,.7f)*.19f;
        tactics.shot_aim=track*.08f;
        tactics.fire_interval_scale=1.08f;
        break;
    }
    return tactics;
}
}} // namespace reverie::gumball
#endif
