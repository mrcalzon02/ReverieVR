#ifndef REVERIE_RED_LEDGER_SIGNAGE_H
#define REVERIE_RED_LEDGER_SIGNAGE_H
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "red_ledger_stock_layout.h"

// Game-private, context-lifetime baked, real letter geometry. Each glyph is
// rasterized to horizontal runs (two triangles/run), not one draw per letter.
// Same 10-float position/UV/color/tile format as the room's static batch.
namespace reverie { namespace redledger { namespace signage {
constexpr size_t kVertexStride = 10u;
struct Glyph { char letter; uint8_t rows[7]; };
struct Sign { const char *text; float x,y,z,pixel; int room=0; };
static const Glyph kGlyphs[] = {
 {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
 {'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
 {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
 {'G',{14,17,16,23,17,17,15}}, {'H',{17,17,17,31,17,17,17}},
 {'I',{14,4,4,4,4,4,14}}, {'J',{7,2,2,2,18,18,12}},
 {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
 {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
 {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
 {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
 {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
 {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
 {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
 {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
 {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
 {'2',{14,17,1,2,4,8,31}}, {'3',{30,1,1,14,1,1,30}},
 {'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
 {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
 {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
 {'-',{0,0,0,31,0,0,0}}, {':',{0,4,4,0,4,4,0}},
 {'/',{1,1,2,4,8,16,16}}, {'?',{14,17,1,2,4,0,4}}
};
// Signs sit in front of the lowered bar fascia, not under props or inside
// interactive hitboxes. The counter labels are grouped into two short rows.
static const Sign kSigns[] = {
 {"TAP",-0.85f,0.39f,-0.78f,0.022f},
 {"PAY",-1.58f,-0.81f,-0.55f,0.019f},
 {"CASH",-1.00f,-1.02f,-0.55f,0.019f},
 {"WASH",-0.28f,-0.82f,-0.55f,0.019f},
 {"RACK",0.42f,-1.02f,-0.55f,0.019f},
 {"DRINK",1.25f,-0.82f,-0.72f,0.018f},
 {"SNACK",1.73f,-0.82f,-0.72f,0.018f},
 {"PATRON",-1.30f,0.64f,-1.78f,0.022f},
 {"EXIT",-2.55f,0.78f,-2.83f,0.021f},
 {"BETWEEN DELIVERIES",0.0f,1.35f,-2.88f,0.024f},
 {"STOCK",2.53f,0.76f,-2.83f,0.021f},
 {"ROOM",0.60f,0.76f,-2.83f,0.021f},
 {"TO BAR",-2.47f,0.71f,-2.81f,0.023f,1},
 {"STOCKROOM",0.0f,1.30f,-2.88f,0.028f,1},
 {"NEXT",-0.70f,-0.20f,-0.78f,0.022f,1},
 {"BUY",0.40f,-0.20f,-0.78f,0.022f,1},
 {"BEER",1.20f,-0.77f,-0.55f,0.019f,1},
 {"CUPS",1.68f,-0.77f,-0.55f,0.019f,1},
 {"TO BAR",-2.47f,0.72f,-2.81f,0.023f,2},
 {"TO STOCK",2.47f,0.72f,-2.81f,0.021f,2},
 {"LIVING QUARTERS",0.0f,1.30f,-2.88f,0.026f,2},
 {"LEDGER",0.85f,-0.20f,-0.64f,0.021f,2},
 {"SLEEP",1.35f,-0.62f,-1.07f,0.026f,2}
};
inline const uint8_t *Rows(char c) {
    for (const Glyph &glyph : kGlyphs)
        if (glyph.letter == c) return glyph.rows;
    for (const Glyph &glyph : kGlyphs)
        if (glyph.letter == '?') return glyph.rows;
    return nullptr;
}
inline void Quad(float *buffer, size_t &vertices,
                 float left,float bottom,float right,float top,float z,
                 float red,float green,float blue) {
    const float xy[12] = {
        left,bottom, right,bottom, right,top,
        left,bottom, right,top, left,top
    };
    for (size_t v=0;v<6u;++v) {
        if (buffer != nullptr) {
            float *p=&buffer[(vertices+v)*kVertexStride];
            p[0]=xy[v*2u]; p[1]=xy[v*2u+1u]; p[2]=z;
            p[3]=0.50f; p[4]=0.50f; // sample center of paper tile
            p[5]=red; p[6]=green; p[7]=blue;
            p[8]=0.50390625f; p[9]=0.50390625f;
        }
    }
    vertices+=6u;
}
inline size_t EmitLine(float *out,size_t at,const char *label,
                       float cx,float cy,float z,float px);
inline size_t Emit(float *buffer,int room=0) {
    size_t vertices=0;
    for (const Sign &sign : kSigns) {
        if (sign.room!=room) continue;
        const size_t len=std::strlen(sign.text);
        const float px=sign.pixel;
        const float width=static_cast<float>(len*6u-1u)*px;
        const float left=sign.x-width*0.5f;
        const float bottom=sign.y-4.0f*px;
        Quad(buffer,vertices,left-2.0f*px,bottom-2.0f*px,
             left+width+2.0f*px,bottom+9.0f*px,sign.z-0.003f,
             0.055f,0.075f,0.085f);
        for (size_t index=0;index<len;++index) {
            if (sign.text[index]==' ') continue;
            const uint8_t *rows=Rows(sign.text[index]);
            if (rows==nullptr) continue;
            const float gx=left+static_cast<float>(index*6u)*px;
            for (size_t row=0;row<7u;++row) {
                unsigned col=0;
                while(col<5u) {
                    if ((rows[row] & (1u << (4u-col)))==0u) {
                        ++col; continue;
                    }
                    const unsigned start=col;
                    while(col<5u && (rows[row] & (1u << (4u-col)))!=0u)
                        ++col;
                    const float y=bottom+static_cast<float>(6u-row)*px;
                    Quad(buffer,vertices,
                         gx+static_cast<float>(start)*px,y,
                         gx+static_cast<float>(col)*px,y+px,
                         sign.z+0.003f,0.97f,0.92f,0.75f);
                }
            }
        }
    }
    if(room==1) {
        // Baked, one-draw stockroom catalog: six faction columns, four real
        // product groups. An offer is selected by looking at its physical card.
        static const float kFactionInk[stock::kSuppliers][3]={
            {0.17f,0.27f,0.35f},{0.31f,0.27f,0.14f},
            {0.36f,0.17f,0.15f},{0.25f,0.19f,0.15f},
            {0.34f,0.23f,0.13f},{0.16f,0.29f,0.18f}
        };
        for(int slot=0;slot<stock::kCells;++slot){
            const stock::Cell cell=stock::At(slot);
            const float *ink=kFactionInk[cell.faction];
            Quad(buffer,vertices,
                 cell.x-stock::kHalfWidth,
                 cell.y-stock::kHalfHeight,
                 cell.x+stock::kHalfWidth,
                 cell.y+stock::kHalfHeight,
                 cell.z,ink[0],ink[1],ink[2]);
            char label[24]={};
            std::snprintf(label,sizeof(label),"%d %s",
                stock::SupplierCode(cell.faction),
                stock::GroupLabel(cell.group));
            // Keep the clickable card label below its original product art.
            vertices=EmitLine(buffer,vertices,label,
                cell.x,cell.y-0.096f,cell.z+0.014f,0.0070f);
        }
    }
    return vertices;
}
inline size_t VertexCount(int room=0) { return Emit(nullptr,room); }
// Capacity and null checks run before writing a single vertex. No GL dependency.
inline bool Build(float *out,size_t capacity_floats,size_t *out_vertices,int room=0) {
    if (out_vertices==nullptr) return false;
    const size_t count=VertexCount(room);
    *out_vertices=count;
    if (out==nullptr || capacity_floats<count*kVertexStride) return false;
    return Emit(out,room)==count;
}
// Editable data text from real letter geometry: callers count first using
// out=null, then allocate exactly count*kVertexStride floats and emit once.
// Refresh only on a world-state change, never per-eye or every frame.
inline size_t EmitLine(float *out,size_t at,const char *label,
                       float cx,float cy,float z,float px) {
    if(label==nullptr) return at;
    size_t len=std::strlen(label);
    if(len>64u)len=64u;
    const float width=len?static_cast<float>(len*6u-1u)*px:0.0f;
    const float left=cx-width*0.5f, bottom=cy-4.0f*px;
    Quad(out,at,left-2.0f*px,bottom-2.0f*px,
         left+width+2.0f*px,bottom+9.0f*px,z-0.004f,
         0.055f,0.075f,0.085f);
    for(size_t index=0;index<len;++index) {
        if(label[index]==' ')continue;
        const uint8_t *rows=Rows(label[index]);
        if(!rows)continue;
        const float gx=left+static_cast<float>(index*6u)*px;
        for(size_t row=0;row<7u;++row){
            unsigned col=0;
            while(col<5u){
                if((rows[row]&(1u<<(4u-col)))==0u){++col;continue;}
                const unsigned start=col;
                while(col<5u&&(rows[row]&(1u<<(4u-col)))!=0u)++col;
                const float y=bottom+static_cast<float>(6u-row)*px;
                Quad(out,at,gx+static_cast<float>(start)*px,y,
                    gx+static_cast<float>(col)*px,y+px,z+0.003f,
                    0.97f,0.92f,0.75f);
            }
        }
    }
    return at;
}
}}}
#endif
