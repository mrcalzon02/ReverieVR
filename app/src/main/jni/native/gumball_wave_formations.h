#ifndef REVERIE_GUMBALL_WAVE_FORMATIONS_H
#define REVERIE_GUMBALL_WAVE_FORMATIONS_H
#include <cstdint>
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
}} // namespace reverie::gumball
#endif
