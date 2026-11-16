#ifndef REVERIE_SITKA_CHART_SURVEY_H
#define REVERIE_SITKA_CHART_SURVEY_H

#include "sitka_land_tile_stream.h"
#include "sitka_fishing.h"

namespace reverie { namespace sitka {
// Three referenced vertices of the archived 4 m NOAA-land/3DEP mesh, in
// EPSG:32608. Labels describe the chart samples, not public landing sites.
struct ChartStation {
    const char* name;
    double east,north;
    float elevation;
    unsigned row,column;
    uint32_t tile_crc;
};
inline constexpr std::array<ChartStation,kChartStations> kSurveyRoute={{
    {"CRESCENT HARBOR SHORE",479960.,6323120.,1.6830f,2,2,0xd7e11ec4u},
    {"EAST ISLET",480396.,6322868.,2.2663f,3,3,0x2df44a76u},
    {"WEST ISLET",479172.,6322636.,6.6941f,3,0,0xbed185d6u}
}};
inline constexpr double kChartSurveyRadius=18.;
inline bool HasSurveyRoute(const LandTileStream& tiles){
    if(tiles.rows()!=4||tiles.columns()!=4||
       tiles.origin_east()!=478720.||tiles.origin_north()!=6324480.)return false;
    for(const auto& site:kSurveyRoute)
        if(tiles.record(site.row,site.column).crc!=site.tile_crc)return false;
    return true;
}
// A cartographic symbol at game scale, anchored at the sampled terrain vertex.
inline TerrainMesh ChartStationPin(){
    TerrainMesh m;
    m.vertices={{0,0,0,255,210,68},{-7,15,0,255,222,75},{0,26,0,255,247,175},
        {7,15,0,255,162,50},{0,15,-7,255,188,54},{0,15,7,249,130,45}};
    m.indices={0,1,4,0,4,3,0,3,5,0,5,1,2,4,1,2,3,4,2,5,3,2,1,5};
    return m;
}
}} // namespace reverie::sitka
#endif
