#ifndef REVERIE_CLASSIC_VECTOR_ART_H
#define REVERIE_CLASSIC_VECTOR_ART_H

// Game-private, font-file-free GLES2 triangle recipes for the eight classics.
// XY strokes are continuous paths, not a 3x5 block-character grid. All geometry
// goes through the existing flat mesh builder and survives GL-context loss.
#include "reverie_native_flat_mesh.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace reverie { namespace classicart {

// Each pair is an (x,y) knot on a 5x7 design grid. Semicolons lift the pen.
// Glyph paths are deliberately outlines/strokes, never one quad per pixel.
inline const char* Letter(char c) {
    switch (c) {
        case 'A': return "000516364540;0343";
        case 'B': return "0006364544330300;3344413000";
        case 'C': return "4536160501103041";
        case 'D': return "00062645412000";
        case 'E': return "40000646;0333";
        case 'F': return "000646;0333";
        case 'G': return "4536160501103041424323";
        case 'H': return "0006;400446;0343";
        case 'I': return "0646;2600;0040";
        case 'J': return "06364641013010";
        case 'K': return "0006;46033340";
        case 'L': return "060040";
        case 'M': return "0006244640";
        case 'N': return "00064640";
        case 'O': return "0110053645413001";
        case 'P': return "00063645443303";
        case 'Q': return "0110053645413001;2340";
        case 'R': return "00063645443303;233340";
        case 'S': return "45361605012334413010";
        case 'T': return "0646;2600";
        case 'U': return "061010304146";
        case 'V': return "06204046";
        case 'W': return "060013204046";
        case 'X': return "0640;0046";
        case 'Y': return "062326;2340";
        case 'Z': return "06464000";
        case '0': return "0110053645413001";
        case '1': return "150026;0040";
        case '2': return "06364643330040";
        case '3': return "06364643332340413010";
        case '4': return "06430343;4600";
        case '5': return "460616033343413010";
        case '6': return "4536160501103041433303";
        case '7': return "06462000";
        case '8': return "0110053645413001;033343";
        case '9': return "4130361605034346";
        case '+': return "032343;2600";
        case '-': return "0343";
        case ':': return "1213;1110";
        case '.': return "0010";
        case '/': return "0046";
        case '!': return "2602;2000";
        case '?': return "061636454323;2100";
        case 39: return "2635";
        case '(': return "3516141021";
        case ')': return "1516363041";
        case ' ': return "";
        default: return "061636454323;2100"; // visible unknown glyph
    }
}

inline void StrokeText(std::vector<float>& vertices,const char* text,
                       float x,float y,float scale,float r,float g,float b,
                       float z=-2.89f) {
    if (!text || scale <= 0.f) return;
    const size_t count=std::strlen(text);
    float pen=x-2.f*scale*static_cast<float>(count);
    for(size_t k=0;k<count;++k,pen+=4.f*scale) {
        char c=text[k];
        if(c>='a'&&c<='z')c=static_cast<char>(c-'a'+'A');
        const char* path=Letter(c);
        bool drawing=false;
        float previousX=0.f,previousY=0.f;
        while(*path) {
            if(*path==';') {
                drawing=false;
                ++path;
                continue;
            }
            const char a=*path++;
            if(!*path)break;
            const char b0=*path++;
            if(a<'0'||a>'4'||b0<'0'||b0>'6') {
                drawing=false;
                continue;
            }
            const float nx=pen+(.3f+static_cast<float>(a-'0')*.82f)*scale;
            const float ny=y+(static_cast<float>(b0-'0')-3.f)*.80f*scale;
            if(drawing)
                reverie::flatmesh::Segment(vertices,previousX,previousY,nx,ny,
                                            scale*.38f,z,r,g,b);
            previousX=nx;
            previousY=ny;
            drawing=true;
        }
    }
}

inline void ShapeRect(std::vector<float>& v,float x,float y,float s,
                      float xx,float yy,float w,float h,float z,
                      float r,float g,float b) {
    reverie::flatmesh::Rect(v,x+xx*s,y+yy*s,w*s,h*s,z,r,g,b);
}
inline void ShapeLine(std::vector<float>& v,float x,float y,float s,
                      float ax,float ay,float bx,float by,float width,float z,
                      float r,float g,float b) {
    reverie::flatmesh::Segment(v,x+ax*s,y+ay*s,x+bx*s,y+by*s,
                               width*s,z,r,g,b);
}
inline void ShapeDisc(std::vector<float>& v,float x,float y,float s,
                      float xx,float yy,float radius,float z,
                      float r,float g,float b) {
    reverie::flatmesh::Disc(v,x+xx*s,y+yy*s,radius*s,z,r,g,b);
}
inline void ShapeTriangle(std::vector<float>& v,float x,float y,float s,
                          float ax,float ay,float bx,float by,
                          float cx,float cy,float z,float r,float g,float b) {
    reverie::flatmesh::Triangle(v,x+ax*s,y+ay*s,x+bx*s,y+by*s,
                                x+cx*s,y+cy*s,z,r,g,b);
}

// Actual chess-piece silhouettes inset onto high-contrast ivory/ebony tokens.
// Piece numbering follows classic_board_games.h: P=1 N=2 B=3 R=4 Q=5 K=6.
// Every type has unique head/crown geometry. The white/black colors come
// from the backing disc; glyphs use the opposite ink for readability.
inline void ChessPiece(std::vector<float>& v,int piece,
                       float x,float y,float scale,
                       float r,float g,float b) {
    const int kind=std::abs(piece);
    if(kind<1||kind>6||scale<=0.f)return;
    constexpr float z=-2.919f;
    // Shared pedestal and foot, essential for classical chess silhouettes.
    ShapeRect(v,x,y,scale,0.f,-.92f,1.55f,.18f,z,r,g,b);
    ShapeRect(v,x,y,scale,0.f,-.73f,1.20f,.18f,z,r,g,b);
    if(kind==1) { // pawn: round head, tapered neck, collar
        ShapeTriangle(v,x,y,scale,-.43f,-.65f,.43f,-.65f,
                      0.f,.10f,z,r,g,b);
        ShapeRect(v,x,y,scale,0.f,-.04f,.90f,.16f,z,r,g,b);
        ShapeDisc(v,x,y,scale,0.f,.42f,.37f,z,r,g,b);
    } else if(kind==2) { // knight: horse head, ears/muzzle and bowed neck
        ShapeTriangle(v,x,y,scale,-.61f,-.65f,.54f,-.65f,
                      -.32f,.73f,z,r,g,b);
        ShapeTriangle(v,x,y,scale,-.32f,.73f,.38f,.53f,
                      -.05f,-.08f,z,r,g,b);
        ShapeTriangle(v,x,y,scale,-.55f,.60f,-.34f,1.06f,
                      -.16f,.64f,z,r,g,b);
        ShapeRect(v,x,y,scale,.25f,.27f,.85f,.25f,z,r,g,b);
        // A cut-in light eye distinguishes the horse from a letter N.
        ShapeDisc(v,x,y,scale,-.12f,.47f,.075f,z+.012f,
                  r>.5f?.08f:.96f,r>.5f?.10f:.93f,r>.5f?.13f:.83f);
    } else if(kind==3) { // bishop: mitre, diagonal split
        ShapeTriangle(v,x,y,scale,-.42f,-.59f,.42f,-.59f,
                      0.f,.50f,z,r,g,b);
        ShapeDisc(v,x,y,scale,0.f,.34f,.42f,z,r,g,b);
        ShapeDisc(v,x,y,scale,0.f,.88f,.13f,z,r,g,b);
        ShapeLine(v,x,y,scale,-.26f,.08f,.25f,.55f,.11f,z+.012f,
                  r>.5f?.10f:.97f,r>.5f?.12f:.94f,r>.5f?.16f:.85f);
    } else if(kind==4) { // rook: battlements and square tower
        ShapeRect(v,x,y,scale,0.f,-.05f,.98f,1.34f,z,r,g,b);
        ShapeRect(v,x,y,scale,0.f,.65f,1.38f,.17f,z,r,g,b);
        for(int i=-1;i<=1;++i)
            ShapeRect(v,x,y,scale,i*.48f,.84f,.30f,.34f,z,r,g,b);
    } else if(kind==5) { // queen: 5-tipped crown over a flared skirt
        ShapeTriangle(v,x,y,scale,-.65f,-.59f,.65f,-.59f,
                      0.f,.35f,z,r,g,b);
        for(int i=-2;i<=2;++i) {
            const float xx=i*.31f;
            ShapeLine(v,x,y,scale,xx*.72f,.12f,xx,.76f-std::abs(i)*.12f,
                      .19f,z,r,g,b);
            ShapeDisc(v,x,y,scale,xx,.78f-std::abs(i)*.12f,
                      .13f,z,r,g,b);
        }
        ShapeRect(v,x,y,scale,0.f,-.12f,1.15f,.20f,z,r,g,b);
    } else { // king: central cross and crown
        ShapeTriangle(v,x,y,scale,-.52f,-.61f,.52f,-.61f,
                      0.f,.35f,z,r,g,b);
        ShapeRect(v,x,y,scale,0.f,.17f,1.15f,.19f,z,r,g,b);
        ShapeRect(v,x,y,scale,0.f,.69f,.19f,.81f,z,r,g,b);
        ShapeRect(v,x,y,scale,0.f,.81f,.66f,.18f,z,r,g,b);
    }
}

}} // namespace reverie::classicart
#endif
