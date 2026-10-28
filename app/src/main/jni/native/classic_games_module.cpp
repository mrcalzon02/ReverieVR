#include "classic_games.h"
#include "classic_breakout.h"
#include "classic_vector_art.h"
#include "classic_board_games.h"
#include "classic_board_cpu.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include "reverie_native_flat_mesh.h"
#include <GLES2/gl2.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <vector>

#ifndef CLASSIC_GAME_KIND
#error "Build must choose a classic game"
#endif
#if CLASSIC_GAME_KIND == 5
#include "classic_chess_book.h"
#include "classic_chess_reference.h"
#include "classic_chess_sprite_atlas.h"
#include "classic_chess_wood_atlas.h"
#include "classic_chess_panel_layout.h"
#endif
// Phase one: Checkers uses the existing Lato glyph atlas; others unchanged.
#if CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5
#include "breakwater_font_atlas.h"
#endif
namespace {
using namespace reverie::classics;
#if CLASSIC_GAME_KIND == 0
constexpr char kId[]="classic-pong";
constexpr char kTitle[]="PONG";
#elif CLASSIC_GAME_KIND == 1
constexpr char kId[]="classic-snake";
constexpr char kTitle[]="SNAKE";
#elif CLASSIC_GAME_KIND == 2
constexpr char kId[]="classic-chutes-ladders";
constexpr char kTitle[]="CHUTES AND LADDERS";
#elif CLASSIC_GAME_KIND == 3
constexpr char kId[]="classic-checkers";
constexpr char kTitle[]="CHECKERS";
#elif CLASSIC_GAME_KIND == 4
constexpr char kId[]="classic-go";
constexpr char kTitle[]="GO 19X19";
#elif CLASSIC_GAME_KIND == 5
constexpr char kId[]="classic-chess";
constexpr char kTitle[]="CHESS";
#elif CLASSIC_GAME_KIND == 6
constexpr char kId[]="classic-breakout";
constexpr char kTitle[]="BREAKOUT";
#elif CLASSIC_GAME_KIND == 7
constexpr char kId[]="classic-reversi";
constexpr char kTitle[]="REVERSI 8X8";
#else
#error "Unknown classic game"
#endif
struct Module {
    const ReverieNativeHostV1 *host=nullptr;
    State game;
    reverie::boardgames::State board;
    reverie::boardgames::CpuWorker cpu;
    bool cpuFailed=false;
    int boardControlHover=0;
#if CLASSIC_GAME_KIND == 6
    Breakout breakout;
#endif
    int boardCursorX=0,boardCursorY=0;
    bool boardCursorActive=false;
    float cursorRepeat=0.f;
    bool previousSecondary=false;
#if CLASSIC_GAME_KIND == 5
    reverie::chessbook::Analysis chessAnalysis;
    int chessAnalyzedPly=-1;
    int chessPanelTab=0; // 0 = legal next move; 1 = indexed classical line
    bool chessPanelHover=false;
    int chessHoverSquare=-1,chessPreviewSquare=-1,chessPreviewPly=-1;
    std::vector<int> chessLegalDestinations;
    float chessPulseClock=0.f;
    bool chessPulseLit=true;
    bool chessWikiOpen=false;
    int chessWikiSection=0,chessWikiPage=0,chessWikiIndex=0;
    std::vector<int> chessWikiMatches;
    GLuint chessProgram=0,chessVbo=0,chessTexture=0;
    GLuint chessBoardVbo=0,chessBoardTexture=0;
    GLint chessPos=-1,chessUv=-1,chessMvp=-1,chessSampler=-1;
    bool chessReady=false;
    std::vector<float> chessSprites;
    std::vector<float> chessBoardSprites;
#endif
#if CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5
    GLuint fontProgram=0,fontVbo=0,fontStaticVbo=0,fontTexture=0;
    GLint fontPos=-1,fontUv=-1,fontCol=-1,fontMvp=-1,fontSampler=-1;
    GLsizei fontStaticCount=0;
    bool fontReady=false;
    std::vector<float> fontVertices,fontStaticVertices;
#endif
    GLuint program=0,vbo=0,staticVbo=0;
    GLint pos=-1,col=-1,mvp=-1;
    bool down=false,dirty=true,upload=true,staticPass=false;
    GLsizei staticCount=0;
    std::vector<float> vertices,staticVertices;
};
std::vector<float> &Emit(Module *m){
    return m->staticPass ? m->staticVertices : m->vertices;
}
void Triangle(Module *m,float ax,float ay,float bx,float by,float cx,float cy,
              float z,float r,float g,float b){
    reverie::flatmesh::Triangle(Emit(m),ax,ay,bx,by,cx,cy,z,r,g,b);
}
void Rect(Module *m,float x,float y,float w,float h,float z,float r,float g,float b){
    reverie::flatmesh::Rect(Emit(m),x,y,w,h,z,r,g,b);
}
// Checkers and Chess use the actual proportional Lato atlas, not 3D stroke glyphs.
void Text(Module *m,const char *s,float x,float y,float scale,
          float r,float g,float b){
#if CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5
    auto &v=m->staticPass?m->fontStaticVertices:m->fontVertices;
    reverie::breakwater::font::AppendText(v,s,x,y,scale,r,g,b,-2.885f);
#else
    reverie::classicart::StrokeText(Emit(m),s,x,y,scale,r,g,b);
#endif
}
void Segment(Module *m,float x,float y,float xx,float yy,float width,
             float r,float g,float b){
    reverie::flatmesh::Segment(Emit(m),x,y,xx,yy,width,-2.965f,r,g,b);
}
float CellX(int n){const int idx=n-1,row=idx/10,col=idx%10;
    return -1.24f+(row%2==0?col:9-col)*.274f;}
float CellY(int n){return -1.22f+((n-1)/10)*.274f;}

void Disc(Module *m,float x,float y,float rad,float z,float r,float g,float b){
    reverie::flatmesh::Disc(Emit(m),x,y,rad,z,r,g,b);
}
float BoardSpacing(const reverie::boardgames::State &s){
    return s.Size()==reverie::boardgames::State::kGoSize?.128f:.320f;
}
float BoardX(const reverie::boardgames::State &s,int x){
    return (x-(s.Size()-1)*.5f)*BoardSpacing(s);
}
float BoardY(const reverie::boardgames::State &s,int y){
    return ((s.Size()-1)*.5f-y)*BoardSpacing(s);
}
void BoardOutline(Module *m,float x,float y,float length,
                  float r,float g,float b){
    const float half=length*.5f;
    Rect(m,x,y+half,length,.020f,-2.94f,r,g,b);
    Rect(m,x,y-half,length,.020f,-2.94f,r,g,b);
    Rect(m,x-half,y,.020f,length,-2.94f,r,g,b);
    Rect(m,x+half,y,.020f,length,-2.94f,r,g,b);
}
#if CLASSIC_GAME_KIND == 5
void ChessPanelTransform(std::vector<float>& v,size_t start,size_t stride,int side){
    for(size_t i=start;i+stride<=v.size();i+=stride)
        reverie::chesspanel::Transform(side,v[i],v[i+2]);
}
void ChessPanelRect(Module *m,float x,float y,float w,float h,float z,
                    float r,float g,float b){
    auto &v=Emit(m);
    const size_t start=v.size();
    reverie::flatmesh::Rect(v,x,y,w,h,z,r,g,b);
    ChessPanelTransform(v,start,6,x<0?-1:1);
}
void ChessPanelText(Module *m,const char *s,float x,float y,float scale,
                    float r,float g,float b,float width=1.06f){
    auto &v=m->staticPass?m->fontStaticVertices:m->fontVertices;
    const size_t start=v.size();
    reverie::breakwater::font::AppendText(v,s,x,y,
        reverie::breakwater::font::FittedScale(s,width,scale),
        r,g,b,-3.012f);
    ChessPanelTransform(v,start,8,x<0?-1:1);
}
void ChessPanelLine(Module *m,const std::string &msg,float x,float y,
                    size_t maxChars=14){
    const std::string line=msg.substr(0,maxChars);
    ChessPanelText(m,line.c_str(),x,y,.027f,.94f,.96f,.85f);
}
void ChessPanelExample(Module *m,const std::string &name,float x,float y){
    ChessPanelLine(m,name,x,y,14);
    if(name.size()>14)
        ChessPanelLine(m,name.substr(14),x,y-.19f,14);
}

void ChessWikiText(Module *m,const std::string &line,float x,float y,
                   float scale=.045f){
    auto &v=m->fontVertices;
    reverie::breakwater::font::AppendText(v,line.c_str(),x,y,
        reverie::breakwater::font::FittedScale(line.c_str(),5.05f,scale),
        .91f,.95f,.85f,-2.725f);
}

void ChessWikiOpen(Module *m,int section){
    m->chessWikiOpen=true;
    m->chessWikiSection=section;
    m->chessWikiPage=0;
    m->chessWikiIndex=std::max(0,reverie::chessref::Current(m->board));
    m->chessWikiMatches=reverie::chessref::Matching(m->board);
    m->dirty=true;
}
void ChessWikiClick(Module *m,float x,float y) {
    if(y>1.30f && x>2.08f){m->chessWikiOpen=false;m->dirty=true;return;}
    if(y>.91f&&y<1.29f){
        const int tab=std::clamp(static_cast<int>(std::floor(
            (x+2.675f)/1.07f)),0,4);
        m->chessWikiSection=tab;
        m->chessWikiPage=0;
        if(tab==0)m->chessWikiIndex=std::max(0,
            reverie::chessref::Current(m->board));
        m->dirty=true;
        return;
    }
    if(y< -1.22f&&y> -1.64f){
        const int change=x< -1.47f?-10:x<0.f?-1:x<1.47f?1:10;
        int maxPage=0;
        if(m->chessWikiSection==1)
            maxPage=std::max(0,static_cast<int>(m->chessWikiMatches.size())-1);
        else if(m->chessWikiSection==2)
            maxPage=std::max(0,static_cast<int>(
                (m->board.chessSanHistory.size()+19)/20)-1);
        else if(m->chessWikiSection==3)maxPage=8;
        else maxPage=reverie::chessref::kEntries-1;
        if(m->chessWikiSection==0||m->chessWikiSection==4)
            m->chessWikiIndex=std::clamp(m->chessWikiIndex+change,0,maxPage);
        else
            m->chessWikiPage=std::clamp(m->chessWikiPage+change,0,maxPage);
        m->dirty=true;
    }
}
void ChessWikiDraw(Module *m) {
    const auto &state=m->board;
    using namespace reverie::chessref;
    Rect(m,0.f,0.f,5.62f,3.53f,-2.79f,.047f,.073f,.11f);
    Rect(m,0.f,1.52f,5.44f,.38f,-2.775f,.12f,.20f,.26f);
    ChessWikiText(m,"CHESS REFERENCE LIBRARY",-.08f,1.49f,.057f);
    Rect(m,2.42f,1.51f,.61f,.29f,-2.768f,.46f,.17f,.15f);
    ChessWikiText(m,"CLOSE",2.42f,1.49f,.040f);
    static const char *const tabs[]={"OPENING","VARIANTS","MOVES","RULES","INDEX"};
    for(int i=0;i<5;++i){
        const float x=(i-2)*1.07f;
        Rect(m,x,1.09f,1.015f,.31f,-2.768f,
            i==m->chessWikiSection?.28f:.12f,
            i==m->chessWikiSection?.39f:.23f,
            i==m->chessWikiSection?.43f:.29f);
        ChessWikiText(m,tabs[i],x,1.065f,.041f);
    }
    std::vector<std::string> lines;
    auto add=[&](const std::string &message){
        const auto wrapped=Wrap(message,58);
        lines.insert(lines.end(),wrapped.begin(),wrapped.end());
    };
    int total=1,page=0;
    if(m->chessWikiSection==2) {
        const auto &hist=state.chessSanHistory;
        total=std::max(1,static_cast<int>((hist.size()+19)/20));
        page=std::clamp(m->chessWikiPage,0,total-1);
        add("PLAYED MOVES - VERIFIED SAN FROM CURRENT BOARD");
        if(hist.empty())add("No moves played yet.");
        const size_t begin=static_cast<size_t>(page)*20;
        for(size_t i=begin;i<hist.size()&&i<begin+20;i+=2){
            const std::string black=i+1<hist.size()?"   "+hist[i+1]:"";
            add(std::to_string(i/2+1)+". "+hist[i]+black);
        }
        add("WHITE IS THE PLAYER. BLACK IS THE CPU.");
    } else if(m->chessWikiSection==3) {
        static const char *const title[]={
            "PIECES AND COORDINATES","KING SAFETY","CASTLING",
            "EN PASSANT","PAWN PROMOTION","CHECKMATE AND STALEMATE",
            "DRAWS AND REPETITION","OPENING BOOK VS CPU ANALYSIS",
            "CONTROLS AND LEGAL PREVIEWS"};
        static const char *const paragraph[][3]={
            {"Files are a through h left to right. Ranks are 1 to 8 from White to Black.",
             "King: one square. Queen: diagonals and ranks/files. Rook: ranks/files. Bishop: diagonals. Knight: L shape and can jump. Pawns: advance one or two from start, capture diagonally.",
             "The highlighted destinations come from the same legality function used to execute both players' moves."},
            {"No move can leave your own king in check. You cannot capture a king.",
             "When checking a piece during the opponent's turn, the displayed destinations are a hypothetical next-turn preview, not permission to move out of turn.",
             "A pinned piece cannot move if that would expose its king."},
            {"To castle, king and the chosen rook must not have moved. The path must be clear, and the king cannot start, cross, or finish on an attacked square.",
             "Kingside: king goes two squares right and rook lands beside it. Queenside: king goes two left and rook lands beside it.",
             "Castling is unavailable when in check."},
            {"A pawn that has just advanced two squares may be captured by an adjacent enemy pawn as though it moved only one.",
             "En passant is allowed only on the immediately following turn; the captured pawn is removed from its original square.",
             "The king must remain safe after this capture."},
            {"On reaching the opposite back rank, a pawn becomes a queen, rook, bishop, or knight.",
             "Use the promotion control while a pawn is selected before moving to the final rank. Default is queen.",
             "The CPU can also choose all four promotion pieces."},
            {"Checkmate: the side to move is in check and has no legal response. Stalemate: no legal move, but the king is not in check.",
             "The game prevents moves exposing the mover's king. A check can be answered by escape, block, or capture when legal.",
             "Kings are never removed as captured pieces."},
            {"Draws can result from stalemate, a dead position, fivefold repetition, or 75 moves by each side without pawn movement or capture.",
             "Threefold repetition and the 50-move rule can be claimed when eligible.",
             "This game's move history records both sides, including captures and check."},
            {"The offline ECO library contains 3,865 indexed opening lines from lichess-org/chess-openings (CC0 snapshot, October 2026). Each entry has its full name, ECO code and SAN move sequence.",
             "Book suggestions are continuations appearing in indexed lines, not win predictions, engine evaluations or master game statistics.",
             "Outside the book the CPU advice is material-only, explicitly labeled. Never interpret book frequency as a probability of a good move."},
            {"Hover over any piece to see color, type, board coordinate, and flashing legal destinations. Click a White piece then click a highlighted square to play.",
             "The left and right wings open this full offline reference. Use the tabs above and PAGE controls below to explore.",
             "Opponent pieces are inspectable, but only the current side can actually play."}
        };
        total=static_cast<int>(sizeof(title)/sizeof(title[0]));
        page=std::clamp(m->chessWikiPage,0,total-1);
        add(title[page]);
        for(const char *text:paragraph[page]){add("");add(text);}
    } else {
        int entry=m->chessWikiIndex;
        if(m->chessWikiSection==1){
            total=std::max(1,static_cast<int>(m->chessWikiMatches.size()));
            page=std::clamp(m->chessWikiPage,0,total-1);
            entry=m->chessWikiMatches.empty()?-1:m->chessWikiMatches[page];
            add("BOOK LINES MATCHING YOUR CURRENT MOVE HISTORY");
        } else {
            total=kEntries;
            entry=std::clamp(entry,0,kEntries-1);
            page=entry;
            add(m->chessWikiSection==4?
                "COMPLETE OFFLINE OPENING INDEX":"CURRENT OPENING AND NEARBY INDEX ENTRIES");
        }
        if(entry>=0){
            const Entry ref=Read(entry);
            add("ECO "+ref.eco+"  |  ENTRY "+std::to_string(entry+1)+
                " OF "+std::to_string(kEntries));
            add(ref.name);
            add("");
            add("COMPLETE INDEXED SAN MOVE LINE:");
            add(ref.san);
            add("");
            add("SOURCE: LICHESS OPENINGS CC0 (OCT 2026)");
            add("INDEXED LINE, NOT AN ENGINE PREDICTION.");
        }else{
            add("No opening catalog line follows this exact position.");
            add("Use INDEX to browse all 3,865 source entries.");
        }
    }
    constexpr int kVisible=12;
    for(int i=0;i<kVisible && i<static_cast<int>(lines.size());++i){
        if(lines[i].empty())continue;
        ChessWikiText(m,lines[i],0.f,.77f-static_cast<float>(i)*.151f);
    }
    Rect(m,-2.07f,-1.40f,1.16f,.30f,-2.765f,.19f,.29f,.35f);
    Rect(m,-.80f,-1.40f,1.16f,.30f,-2.765f,.19f,.29f,.35f);
    Rect(m,.80f,-1.40f,1.16f,.30f,-2.765f,.19f,.29f,.35f);
    Rect(m,2.07f,-1.40f,1.16f,.30f,-2.765f,.19f,.29f,.35f);
    ChessWikiText(m,"-10",-2.07f,-1.415f,.045f);
    ChessWikiText(m,"PREV",-.80f,-1.415f,.045f);
    ChessWikiText(m,"NEXT",.80f,-1.415f,.045f);
    ChessWikiText(m,"+10",2.07f,-1.415f,.045f);
    ChessWikiText(m,"PAGE "+std::to_string(page+1)+" / "+
        std::to_string(total),0.f,-1.13f,.044f);
}
#endif

#if CLASSIC_GAME_KIND == 5
void AppendChessBoardSquare(Module *m,bool dark,float x,float y,float gap){
    // A different licensed PSX wood tile for each alternating square.
    // Rendered at chessboard depth, strictly behind pieces/highlights.
    const float half=(gap-.020f)*.5f,z=-2.993f;
    const float l=x-half,r=x+half,t=y+half,b=y-half;
    const float u0=dark?.5f:0.f,u1=u0+.5f;
    const float vertices[]={
        l,t,z,u0,0, l,b,z,u0,1, r,b,z,u1,1,
        l,t,z,u0,0, r,b,z,u1,1, r,t,z,u1,0
    };
    m->chessBoardSprites.insert(m->chessBoardSprites.end(),
                                vertices,vertices+30);
}
void AppendChessSprite(Module *m,int piece,float x,float y){
    const int tile=reverie::chessart::TileForPiece(piece);
    if(tile<0)return;
    const float half=.151f,z=-2.918f;
    const float l=x-half,r=x+half,t=y+half,b=y-half;
    const float u0=(tile%6)*64.f/reverie::chessart::kWidth;
    const float v0=(tile/6)*64.f/reverie::chessart::kHeight;
    const float u1=u0+64.f/reverie::chessart::kWidth;
    const float v1=v0+64.f/reverie::chessart::kHeight;
    // Atlas is stored SVG top-first, so v0 belongs to the TOP of the quad.
    const float vertices[]={
        l,t,z,u0,v0, l,b,z,u0,v1, r,b,z,u1,v1,
        l,t,z,u0,v0, r,b,z,u1,v1, r,t,z,u1,v0
    };
    m->chessSprites.insert(m->chessSprites.end(),vertices,vertices+30);
}
#endif
void RebuildBoard(Module *m,bool fixed){
    m->staticPass=fixed;
    if(fixed)m->staticVertices.clear();
    else m->vertices.clear();
#if CLASSIC_GAME_KIND == 5
    if(fixed)m->chessBoardSprites.clear();
    else m->chessSprites.clear();
#endif
#if CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5
    if(fixed)m->fontStaticVertices.clear();
    else m->fontVertices.clear();
#endif
    const reverie::boardgames::State &s=m->board;
    const int size=s.Size();
    const float gap=BoardSpacing(s);
    if(fixed){
#if CLASSIC_GAME_KIND == 5
        Rect(m,0,0,3.10f,3.55f,-3.10f,.032f,.058f,.092f);
#else
        Rect(m,0,0,5.5f,3.55f,-3.10f,.032f,.058f,.092f);
#endif
        Rect(m,0,0,3.08f,3.08f,-3.045f,.34f,.24f,.15f);
#if CLASSIC_GAME_KIND == 5
        Text(m,kTitle,0,1.65f,.051f,.91f,.94f,.94f);
        ChessPanelRect(m,-2.03f,.02f,1.25f,3.14f,-3.040f,.085f,.122f,.158f);
        ChessPanelRect(m, 2.03f,.02f,1.25f,3.14f,-3.040f,.085f,.122f,.158f);
        ChessPanelText(m,"CPU LAST",-2.03f,1.24f,.031f,.73f,.91f,.98f);
        ChessPanelText(m,"MOVE BOOK",2.03f,1.24f,.031f,.73f,.91f,.98f);
#else
        Text(m,kTitle,0,1.58f,.082f,.91f,.94f,.94f);
#endif
    }
    for(int y=0;y<size;y++)for(int x=0;x<size;x++){
        float px=BoardX(s,x),py=BoardY(s,y);
        const bool dark=(x+y)%2!=0;
        if(fixed && s.kind==reverie::boardgames::Kind::Go){
            Rect(m,px,py,gap-.015f,gap-.015f,-3.01f,
                 .69f,.51f,.29f);
            Rect(m,px,py,gap-.018f,.008f,-2.985f,.19f,.13f,.09f);
            Rect(m,px,py,.008f,gap-.018f,-2.985f,.19f,.13f,.09f);
            if((x==3||x==9||x==15)&&(y==3||y==9||y==15))
                Disc(m,px,py,.023f,-2.979f,.15f,.11f,.09f);
        }else if(fixed){
            Rect(m,px,py,gap-.008f,gap-.008f,-3.00f,
                 dark?.19f:.77f,dark?.32f:.80f,dark?.35f:.70f);
#if CLASSIC_GAME_KIND == 5
            if(s.kind==reverie::boardgames::Kind::Chess)
                AppendChessBoardSquare(m,dark,px,py,gap);
#endif
        }
        if(!fixed){
        int cell=y*size+x;
        const int piece=s.cells[cell];
        if(piece!=0){
            if(s.kind==reverie::boardgames::Kind::Go){
                if(piece>0){
                    Disc(m,px,py,gap*.42f,-2.947f,.045f,.062f,.074f);
                    Disc(m,px,py,gap*.08f,-2.935f,.86f,.88f,.79f);
                }else{
                    Disc(m,px,py,gap*.42f,-2.947f,.91f,.90f,.79f);
                    Disc(m,px,py,gap*.08f,-2.935f,.18f,.17f,.14f);
                }
            }else if(s.kind==reverie::boardgames::Kind::Reversi){
                Disc(m,px,py,gap*.38f,-2.947f,
                     piece>0?.045f:.91f,piece>0?.07f:.91f,
                     piece>0?.075f:.86f);
                Disc(m,px,py,gap*.29f,-2.939f,
                     piece>0?.09f:.98f,piece>0?.12f:.98f,
                     piece>0?.13f:.93f);
            }else if(s.kind==reverie::boardgames::Kind::Chess){
                // Opposite-valued backing keeps the source white and
                // black Livius outlines legible on both square colors.
                Disc(m,px,py,.148f,-2.949f,.60f,.49f,.29f);
                Disc(m,px,py,.140f,-2.940f,
                     piece>0?.085f:.93f,piece>0?.115f:.90f,
                     piece>0?.15f:.78f);
#if CLASSIC_GAME_KIND == 5
                AppendChessSprite(m,piece,px,py);
#endif
            }else{
                // Checkers retain physical tokens and true crowned kings.
                Disc(m,px,py,.125f,-2.947f,
                     piece>0?.92f:.20f,piece>0?.92f:.23f,
                     piece>0?.80f:.36f);
                Disc(m,px,py,.096f,-2.934f,
                     piece>0?.73f:.42f,piece>0?.79f:.55f,
                     piece>0?.69f:.65f);
                if(std::abs(piece)==2){
                    Text(m,"K",px,py-.005f,.039f,
                         piece>0?.04f:.97f,piece>0?.09f:.97f,
                         piece>0?.12f:.93f);
                }else{
                    Disc(m,px,py,.031f,-2.927f,
                         piece>0?.06f:.95f,piece>0?.12f:.94f,
                         piece>0?.16f:.88f);
                }
            }
        }
        if(s.kind==reverie::boardgames::Kind::Reversi &&
           !s.finished&&s.side==1&&piece==0 &&
           reverie::boardgames::State::ReversiFlips(s.cells,cell,1)>0)
            Disc(m,px,py,gap*.065f,-2.949f,.94f,.88f,.25f);
        if(s.lastTo==cell){
            const float a=gap*.48f;
            Rect(m,px-a,py-a,.045f,.045f,-2.917f,.95f,.86f,.27f);
            Rect(m,px+a,py+a,.045f,.045f,-2.917f,.95f,.86f,.27f);
        }
        if(s.selected==cell)
            BoardOutline(m,px,py,gap*.96f,1.f,.99f,.20f);
#if CLASSIC_GAME_KIND == 5
        if(s.kind==reverie::boardgames::Kind::Chess &&
           m->chessPulseLit &&
           std::find(m->chessLegalDestinations.begin(),
                     m->chessLegalDestinations.end(),cell)
                !=m->chessLegalDestinations.end()){
            BoardOutline(m,px,py,gap*.86f,.19f,.97f,.58f);
            if(piece==0)Disc(m,px,py,.046f,-2.930f,.19f,.97f,.58f);
        }
        if(s.kind==reverie::boardgames::Kind::Chess &&
           m->chessHoverSquare==cell)
            BoardOutline(m,px,py,gap*.95f,.97f,.83f,.27f);
#endif
        if(s.kind==reverie::boardgames::Kind::Go && s.goReview &&
           s.goDead[cell])
            BoardOutline(m,px,py,gap*.92f,.98f,.30f,.24f);
        if(m->boardCursorActive && m->boardCursorX==x &&
           m->boardCursorY==y)
            BoardOutline(m,px,py,gap*.76f,.93f,.98f,.99f);
        }
    }
    if(fixed && s.kind==reverie::boardgames::Kind::Go){
        Text(m,"TWO PASSES: REVIEW DEAD GROUPS",0,-1.58f,.040f,
             .80f,.90f,.87f);
    }else if(fixed){
        Text(m,s.kind==reverie::boardgames::Kind::Reversi
             ?"SELECT A MARKED SQUARE":"SELECT PIECE THEN DESTINATION",
#if CLASSIC_GAME_KIND == 5
             0,-1.60f,.025f,.84f,.90f,.87f);
#else
             0,-1.58f,.040f,.84f,.90f,.87f);
#endif
    }
    if(!fixed && s.kind==reverie::boardgames::Kind::Go &&
       !s.finished){
        Rect(m,1.94f,-1.25f,.89f,.41f,-2.98f,.23f,.36f,.42f);
        Text(m,s.goReview?"SCORE":"PASS",1.94f,-1.25f,.053f,
             1.f,1.f,.89f);
        if(s.goReview){
            Rect(m,-1.94f,-1.25f,.89f,.41f,-2.98f,.23f,.36f,.42f);
            Text(m,"RESUME",-1.94f,-1.25f,.047f,1.f,1.f,.89f);
        }
    }
    if(!fixed && s.finished){
        const char *msg=s.winner==1?"YOU WIN":s.winner==-1?
            "CPU WINS":"DRAW";
#if CLASSIC_GAME_KIND == 5
        Text(m,msg,0,1.40f,.032f,1.f,.91f,.58f);
#else
        Text(m,msg,0,1.34f,.074f,1.f,.91f,.58f);
#endif
        if(s.kind==reverie::boardgames::Kind::Go){
            char scored[64];
            std::snprintf(scored,sizeof(scored),"BLACK %.1f  WHITE %.1f",
                          s.goBlackScore,s.goWhiteScore);
            Text(m,scored,0,1.15f,.044f,.95f,.96f,.85f);
        }else if(s.kind==reverie::boardgames::Kind::Reversi){
            char scored[64];
            std::snprintf(scored,sizeof(scored),"BLACK %d  WHITE %d",
                          s.ReversiCount(1),s.ReversiCount(-1));
            Text(m,scored,0,1.15f,.044f,.95f,.96f,.85f);
        }
#if CLASSIC_GAME_KIND == 5
        ChessPanelText(m,"RESTART",2.03f,-1.35f,.031f,
                       1.f,.95f,.78f);
#else
        Text(m,"PRESS TO RESTART",1.91f,.66f,.043f,1.f,.95f,.78f);
#endif
    }else if(!fixed){
        Text(m,s.kind==reverie::boardgames::Kind::Go&&s.goReview
             ?"MARK DEAD GROUPS":m->cpuFailed?"CPU ERROR RESTART":
             s.side==1?"YOUR TURN":"CPU THINKING",
#if CLASSIC_GAME_KIND == 5
             0,1.40f,.032f,.95f,.95f,.78f);
#else
             0,1.34f,.064f,.95f,.95f,.78f);
#endif
        if(s.kind==reverie::boardgames::Kind::Reversi){
            char scored[64];
            std::snprintf(scored,sizeof(scored),"BLACK %d  WHITE %d",
                          s.ReversiCount(1),s.ReversiCount(-1));
            Text(m,scored,0,1.15f,.044f,.95f,.96f,.85f);
            if(s.passes==1)
                Text(m,"OPPONENT PASSED",0,-1.39f,.042f,
                     .97f,.91f,.48f);
        }
        if(s.kind==reverie::boardgames::Kind::Checkers && s.forced>=0)
            Text(m,"CONTINUE JUMP",1.88f,.63f,.044f,1.f,.90f,.56f);
        if(s.kind==reverie::boardgames::Kind::Checkers &&
           s.side==1){
            Rect(m,1.94f,-.65f,.96f,.39f,-2.98f,
                 m->boardControlHover==1?.38f:.23f,.36f,.42f);
            Text(m,s.checkerDifficulty==0?"AI EASY":
                   s.checkerDifficulty==2?"AI HARD":"AI NORMAL",
                 1.94f,-.65f,.042f,1.f,1.f,.89f);
        }
        if(s.kind==reverie::boardgames::Kind::Checkers &&
           s.CheckerDrawClaimable()){
            Rect(m,1.94f,-1.25f,.96f,.39f,-2.98f,
                 .23f,.36f,.42f);
            Text(m,"CLAIM DRAW",1.94f,-1.25f,.042f,1.f,1.f,.89f);
        }
        if(s.kind==reverie::boardgames::Kind::Chess&&s.side==1){
#if CLASSIC_GAME_KIND == 5
            ChessPanelRect(m,2.03f,-.98f,.98f,.32f,-3.026f,
                           m->boardControlHover==1?.38f:.23f,.36f,.42f);
            ChessPanelText(m,s.chessDifficulty==0?"AI EASY":
                           s.chessDifficulty==2?"AI HARD":"AI NORMAL",
                           2.03f,-.98f,.027f,1.f,1.f,.89f,.91f);
            if(s.selected>=0&&s.selected<64&&
               s.cells[s.selected]==1&&s.selected/8==1){
                const char *promo=s.promotionChoice==4?"PROMO ROOK":
                    s.promotionChoice==3?"PROMO BISHOP":
                    s.promotionChoice==2?"PROMO KNIGHT":"PROMO QUEEN";
                ChessPanelText(m,promo,2.03f,.66f,.025f,.91f,.92f,.75f);
            }
            if(s.ChessDrawClaimable()){
                ChessPanelRect(m,2.03f,-1.36f,.98f,.30f,-3.026f,
                               .23f,.36f,.42f);
                ChessPanelText(m,"CLAIM DRAW",2.03f,-1.36f,.024f,
                               1.f,1.f,.89f,.91f);
            }
#endif
        }
    }
#if CLASSIC_GAME_KIND == 5
    if(!fixed && s.kind==reverie::boardgames::Kind::Chess){
        const int focus=m->chessHoverSquare>=0?m->chessHoverSquare:s.selected;
        if(focus>=0&&focus<64){
            const int piece=s.cells[focus];
            const std::string identity=reverie::chessref::PieceName(piece)+
                " "+reverie::chessref::Square(focus);
            Text(m,identity.c_str(),0.f,1.14f,.030f,.96f,.94f,.68f);
            if(piece){
                const std::string count="VALID DESTINATIONS: "+
                    std::to_string(reverie::chessref::Destinations(s,focus).size());
                Text(m,count.c_str(),0.f,-1.36f,.024f,.74f,.98f,.77f);
            }
        }
    }
    if(!fixed){
        const auto &book=m->chessAnalysis;
        ChessPanelLine(m,"LAST: "+book.previousCpuMove,-2.01f,.84f);
        ChessPanelLine(m,book.cpuComparison,-2.01f,.55f);
        ChessPanelLine(m,"OPENING "+book.eco,-2.01f,.18f);
        ChessPanelExample(m,book.opening,-2.01f,-.12f);
        ChessPanelLine(m,"CC0 ECO INDEX",-2.01f,-.84f);
        ChessPanelRect(m,2.03f,.88f,1.05f,.36f,-3.026f,
             m->chessPanelHover?.32f:.21f,
             m->chessPanelHover?.45f:.34f,.48f);
        ChessPanelLine(m,m->chessPanelTab==0?"NEXT: EXAMPLE":
                        "EXAMPLE: NEXT",2.01f,.87f);
        if(m->chessPanelTab==0){
            ChessPanelLine(m,"PLAY "+book.recommended,2.01f,.47f);
            ChessPanelLine(m,book.recommendationOrigin.substr(0,14),
                           2.01f,.13f);
            ChessPanelLine(m,"REFERENCE:",2.01f,-.24f);
            ChessPanelExample(m,book.example,2.01f,-.48f);
        }else{
            ChessPanelLine(m,"CLASSICAL LINE",2.01f,.49f);
            ChessPanelExample(m,book.example,2.01f,.19f);
            ChessPanelLine(m,book.eco,2.01f,-.44f);
            ChessPanelLine(m,"NOT ENGINE EVAL",2.01f,-.83f);
        }
    }
    if(!fixed && m->chessWikiOpen)ChessWikiDraw(m);
#endif
    if(fixed){
        m->staticCount=static_cast<GLsizei>(m->staticVertices.size()/6);
#if CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5
        m->fontStaticCount=static_cast<GLsizei>(m->fontStaticVertices.size()/8);
#endif
    }else {m->dirty=false;m->upload=true;}
}
void Rebuild(Module *m,bool fixed){
#if CLASSIC_GAME_KIND == 6
    m->staticPass=fixed;
    if(fixed)m->staticVertices.clear();
    else m->vertices.clear();
    const Breakout &s=m->breakout;
    if(fixed){
        Rect(m,0,0,5.5f,3.55f,-3.10f,.032f,.058f,.092f);
        Text(m,kTitle,0,1.57f,.092f,.78f,.94f,.96f);
        Rect(m,0,0,2.07f,2.03f,-3.04f,.08f,.13f,.20f);
        // Court edges and rules are static; bricks and ball are dynamic.
        Rect(m,-1.015f,0,.025f,2.03f,-3.01f,.28f,.45f,.49f);
        Rect(m,1.015f,0,.025f,2.03f,-3.01f,.28f,.45f,.49f);
        Text(m,"MOVE PAD SELECT SERVE",0,-1.50f,.042f,.75f,.90f,.83f);
    }else{
        for(int i=0;i<Breakout::kBricks;i++)if(s.bricks[i]){
            const int row=i/Breakout::kColumns;
            const float shade=row*.095f;
            Rect(m,Breakout::BrickX(i%Breakout::kColumns),
                 Breakout::BrickY(row),.130f,.104f,-2.97f,
                 .85f-shade,.62f+shade*.48f,.37f+shade);
        }
        Rect(m,s.paddle,-.82f,s.PaddleWidth(),.060f,-2.95f,
             .28f,.91f,.73f);
        Disc(m,s.ballX,s.ballY,.049f,-2.92f,.95f,.96f,.77f);
        char status[48];
        std::snprintf(status,sizeof(status),"SCORE %d LIVES %d WALL %d",
                      s.score,s.lives,s.wall);
        Text(m,status,0,1.35f,.055f,.90f,.95f,.81f);
        Text(m,s.finished
             ? (s.won?"YOU WIN PRESS":"GAME OVER PRESS")
             : (s.moving?"CLEAR ALL BRICKS":"PRESS TO SERVE"),
             0,-1.37f,.043f,.90f,.89f,.74f);
    }
    if(fixed)m->staticCount=static_cast<GLsizei>(
        m->staticVertices.size()/6);
    else {m->dirty=false;m->upload=true;}
#else
#if CLASSIC_GAME_KIND >= 3
    RebuildBoard(m,fixed);
    return;
#endif
    m->staticPass=fixed;
    if(fixed)m->staticVertices.clear();
    else m->vertices.clear();
    const State &s=m->game;
    if(fixed){
        Rect(m,0,0,5.5f,3.55f,-3.10f,.032f,.058f,.092f);
        Text(m,kTitle,0,1.57f,.092f,.78f,.94f,.96f);
    }
    char status[48];
    if(s.game==Game::Pong){
        if(fixed){
        Rect(m,0,0,3.7f,2.60f,-3.04f,.08f,.13f,.20f);
        for(int i=0;i<9;i++)
            Rect(m,0,-1.16f+i*.29f,.032f,.13f,-2.99f,.30f,.39f,.43f);
        }else{
        Rect(m,-1.40f,s.paddle*1.19f,.075f,.56f,-2.96f,.33f,.95f,.78f);
        Rect(m,1.40f,s.ai*1.19f,.075f,.56f,-2.96f,.98f,.60f,.44f);
        Rect(m,s.ball_x*1.55f,s.ball_y*1.22f,.11f,.11f,-2.93f,.94f,.95f,.70f);
        std::snprintf(status,sizeof(status),"%d : %d",s.score,s.opponent);
        Text(m,status,0,1.31f,.07f,1,1,.79f);
        Text(m,s.finished?"PRESS TO RESTART":
             s.moving?"MOVE PADDLE TO BLOCK":"PRESS TO SERVE",
             0,-1.46f,.040f,.70f,.83f,.83f);
        }
    }else if(s.game==Game::Snake){
        if(fixed){
        Rect(m,0,0,2.63f,2.62f,-3.04f,.06f,.16f,.15f);
        for(int y=0;y<State::kGrid;y++)for(int x=0;x<State::kGrid;x++){
            const float px=(x-5.5f)*.21f,py=(5.5f-y)*.21f;
            Rect(m,px,py,.19f,.19f,-3.0f,
                 (x+y)%2?.085f:.075f,.19f,.18f);
        }
        }else{
        if(s.food>=0){
            const int x=s.food%12,y=s.food/12;
            Rect(m,(x-5.5f)*.21f,(5.5f-y)*.21f,
                 .17f,.17f,-2.96f,1,.40f,.30f);
        }
        for(int i=s.length-1;i>=0;i--){
            const int cell=s.snake[i];if(cell<0)continue;
            Rect(m,((cell%12)-5.5f)*.21f,
                 (5.5f-(cell/12))*.21f,.185f,.185f,-2.94f,
                 i==0?.76f:.32f,i==0?1.f:.82f,.45f);
        }
        }
        if(fixed){
        const char *dirs[]={"UP","RIGHT","DOWN","LEFT"};
        const float x[]={0,2.02f,0,-2.02f};
        const float y[]={1.02f,0,-1.02f,0};
        for(int i=0;i<4;i++){
            Rect(m,x[i],y[i],i%2?.79f:.89f,.36f,
                 -2.97f,.20f,.33f,.39f);
            Text(m,dirs[i],x[i],y[i],.052f,.85f,.93f,.88f);
        }
        }else{
        std::snprintf(status,sizeof(status),"FOOD %d",s.score);
        Text(m,status,0,1.34f,.06f,.86f,.91f,.68f);
        Text(m,s.finished?"PRESS TO RESTART":
             s.moving?"POINT TO TURN":"PRESS TO START",
             0,-1.48f,.043f,.78f,.90f,.83f);
        }
    }else{
        if(fixed){
        Rect(m,0,0,2.85f,2.86f,-3.04f,.15f,.16f,.17f);
        for(int n=1;n<=100;n++){
            const float x=CellX(n),y=CellY(n);
            const bool dark=(((n-1)/10+(n-1)%10)%2)==0;
            Rect(m,x,y,.258f,.258f,-2.99f,
                 dark?.19f:.24f,dark?.26f:.31f,dark?.32f:.37f);
            char label[4];
            std::snprintf(label,sizeof(label),"%d",n);
            Text(m,label,x,y+.073f,.017f,.76f,.84f,.85f);
        }
        for(const auto &link:State::kBoardLinks){
            Segment(m,CellX(link.from),CellY(link.from),
                    CellX(link.to),CellY(link.to),.037f,
                    link.to>link.from?.32f:.95f,
                    link.to>link.from?.96f:.40f,.50f);
        }
        Rect(m,0,-1.56f,2.8f,.29f,-2.97f,.22f,.33f,.35f);
        }else{
        if(s.position>0)Rect(m,CellX(s.position)-.06f,CellY(s.position),
                             .11f,.15f,-2.92f,.32f,.96f,.73f);
        if(s.rival>0)Rect(m,CellX(s.rival)+.06f,CellY(s.rival),
                          .11f,.15f,-2.92f,1.f,.61f,.47f);
        if(s.die>0)
            std::snprintf(status,sizeof(status),"YOU %d CPU %d SPIN %d",
                          s.position,s.rival,s.die);
        else
            std::snprintf(status,sizeof(status),"YOU %d CPU %d",
                          s.position,s.rival);
        Text(m,status,0,1.36f,.047f,.94f,.89f,.68f);
        Text(m,s.finished?(s.position==100?"YOU WIN PRESS":"CPU WINS PRESS"):
             s.player_turn?"SELECT TO ROLL":"CPU ROLLING",
             0,-1.56f,.045f,1,.96f,.78f);
        }
    }
    if(fixed)m->staticCount=static_cast<GLsizei>(m->staticVertices.size()/6);
    else {m->dirty=false;m->upload=true;}
#endif // CLASSIC_GAME_KIND == 6
}
void DestroyGl(Module *m){
#if CLASSIC_GAME_KIND == 5
    if(m->chessBoardTexture)glDeleteTextures(1,&m->chessBoardTexture);
    if(m->chessBoardVbo)glDeleteBuffers(1,&m->chessBoardVbo);
    if(m->chessTexture)glDeleteTextures(1,&m->chessTexture);
    if(m->chessVbo)glDeleteBuffers(1,&m->chessVbo);
    if(m->chessProgram)glDeleteProgram(m->chessProgram);

    m->chessTexture=m->chessVbo=m->chessProgram=0;
    m->chessBoardTexture=m->chessBoardVbo=0;
    m->chessReady=false;
#endif
#if CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5
    if(m->fontTexture)glDeleteTextures(1,&m->fontTexture);
    if(m->fontVbo)glDeleteBuffers(1,&m->fontVbo);
    if(m->fontStaticVbo)glDeleteBuffers(1,&m->fontStaticVbo);
    if(m->fontProgram)glDeleteProgram(m->fontProgram);
    m->fontTexture=m->fontVbo=m->fontStaticVbo=m->fontProgram=0;
    m->fontStaticCount=0;m->fontReady=false;
#endif
    if(m->vbo)glDeleteBuffers(1,&m->vbo);
    if(m->staticVbo)glDeleteBuffers(1,&m->staticVbo);
    if(m->program)glDeleteProgram(m->program);
    m->vbo=m->staticVbo=m->program=0;
    m->staticCount=0;
    m->dirty=m->upload=true;
}

#if CLASSIC_GAME_KIND == 5
bool CreateChessSprites(Module *m){
    const char *vs=
        "uniform mat4 u_Mvp;attribute vec3 a_Pos;attribute vec2 a_Uv;"
        "varying vec2 v_Uv;void main(){v_Uv=a_Uv;"
        "gl_Position=u_Mvp*vec4(a_Pos,1.);}";
    const char *fs=
        "precision mediump float;uniform sampler2D u_Tex;varying vec2 v_Uv;"
        "void main(){vec4 pixel=texture2D(u_Tex,v_Uv);"
        "if(pixel.a<0.02)discard;gl_FragColor=pixel;}";
    const ReverieNativeGlAttributeBindingV1 bindings[]={
        {0u,"a_Pos"},{1u,"a_Uv"}};
    char diagnostic[256]={};
    m->chessProgram=ReverieNativeBuildProgramDiagnosticV1(
        vs,fs,bindings,2,diagnostic,sizeof(diagnostic));
    if(!m->chessProgram){
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,
            "ClassicChessArt",diagnostic[0]?diagnostic:"Chess sprite shader failed");
        return false;
    }
    m->chessPos=glGetAttribLocation(m->chessProgram,"a_Pos");
    m->chessUv=glGetAttribLocation(m->chessProgram,"a_Uv");
    m->chessMvp=glGetUniformLocation(m->chessProgram,"u_Mvp");
    m->chessSampler=glGetUniformLocation(m->chessProgram,"u_Tex");
    std::vector<uint8_t> pixels;
    if(m->chessPos<0||m->chessUv<0||m->chessMvp<0||m->chessSampler<0||
       !reverie::chessart::Decode(pixels)){
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,
            "ClassicChessArt","Atlas RLE decode or shader uniforms invalid");
        return false;
    }
    GLint unpack=4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT,&unpack);
    glGenBuffers(1,&m->chessVbo);
    glGenTextures(1,&m->chessTexture);
    if(!m->chessVbo||!m->chessTexture)return false;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,m->chessTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE_ALPHA,
        reverie::chessart::kWidth,reverie::chessart::kHeight,0,
        GL_LUMINANCE_ALPHA,GL_UNSIGNED_BYTE,pixels.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT,unpack);
    if(glGetError()!=GL_NO_ERROR){
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,
            "ClassicChessArt","Livius GLES2 atlas upload failed");
        return false;
    }
    // Source-derived PSX wood lives in a separate, immutable RGB565 atlas.
    // The existing chess shader handles both opaque RGB board material and
    // alpha-masked Livius pieces without loading a raw purchased texture.
    std::vector<uint16_t> wood;
    if(!reverie::chesswood::Decode(wood)){
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,
            "ClassicChessArt","Licensed wood board atlas decode failed");
        return false;
    }
    glGenBuffers(1,&m->chessBoardVbo);
    glGenTextures(1,&m->chessBoardTexture);
    if(!m->chessBoardVbo||!m->chessBoardTexture)return false;
    glBindTexture(GL_TEXTURE_2D,m->chessBoardTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,
        reverie::chesswood::kWidth,reverie::chesswood::kHeight,0,
        GL_RGB,GL_UNSIGNED_SHORT_5_6_5,wood.data());
    if(glGetError()!=GL_NO_ERROR){
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,
            "ClassicChessArt","Licensed wood GLES2 atlas upload failed");
        return false;
    }
    m->chessReady=true;
    return true;
}
#endif

#if CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5
bool CreateNativeFont(Module *m){
    const char *vs="uniform mat4 u_Mvp;attribute vec3 a_Pos;"
        "attribute vec2 a_Uv;attribute vec3 a_Col;"
        "varying vec2 v_Uv;varying vec3 v_Col;"
        "void main(){gl_Position=u_Mvp*vec4(a_Pos,1.);"
        "v_Uv=a_Uv;v_Col=a_Col;}";
    const char *fs="precision mediump float;uniform sampler2D u_Glyph;"
        "varying vec2 v_Uv;varying vec3 v_Col;"
        "void main(){float a=texture2D(u_Glyph,v_Uv).a;"
        "if(a<.01)discard;gl_FragColor=vec4(v_Col,a);}";
    const ReverieNativeGlAttributeBindingV1 bindings[]={
        {0u,"a_Pos"},{1u,"a_Uv"},{2u,"a_Col"}};
    char diagnostic[256]={};
    m->fontProgram=ReverieNativeBuildProgramDiagnosticV1(
        vs,fs,bindings,3,diagnostic,sizeof(diagnostic));
    if(!m->fontProgram){
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,
                           "ClassicGameFont",diagnostic);
        return false;
    }
    m->fontPos=glGetAttribLocation(m->fontProgram,"a_Pos");
    m->fontUv=glGetAttribLocation(m->fontProgram,"a_Uv");
    m->fontCol=glGetAttribLocation(m->fontProgram,"a_Col");
    m->fontMvp=glGetUniformLocation(m->fontProgram,"u_Mvp");
    m->fontSampler=glGetUniformLocation(m->fontProgram,"u_Glyph");
    std::vector<unsigned char> pixels;
    if(m->fontPos<0||m->fontUv<0||m->fontCol<0||
       m->fontMvp<0||m->fontSampler<0||
       !reverie::breakwater::font::DecodeAtlas(pixels)){
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,
                           "ClassicGameFont","Lato atlas invalid");
        return false;
    }
    glGenBuffers(1,&m->fontVbo);
    glGenBuffers(1,&m->fontStaticVbo);
    glGenTextures(1,&m->fontTexture);
    if(!m->fontVbo||!m->fontStaticVbo||!m->fontTexture)return false;
    GLint unpack=4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT,&unpack);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,m->fontTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_ALPHA,
        reverie::breakwater::font::kWidth,
        reverie::breakwater::font::kHeight,0,GL_ALPHA,
        GL_UNSIGNED_BYTE,pixels.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT,unpack);
    m->fontReady=glGetError()==GL_NO_ERROR;
    if(!m->fontReady)ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,
                                      "ClassicGameFont","Atlas upload failed");
    return m->fontReady;
}
#endif
void *Create(const ReverieNativeHostV1 *host){
    Module *m=new(std::nothrow) Module();
    if(m){
        m->host=host;
#if (CLASSIC_GAME_KIND >= 3 && CLASSIC_GAME_KIND <= 5) || CLASSIC_GAME_KIND == 7
        m->board.Reset(static_cast<reverie::boardgames::Kind>(CLASSIC_GAME_KIND));
#elif CLASSIC_GAME_KIND == 6
        m->breakout.Reset();
#else
        m->game.Reset(static_cast<Game>(CLASSIC_GAME_KIND));
#endif
    }
    return m;
}
void Destroy(void *ptr){delete static_cast<Module *>(ptr);}
int32_t GlCreate(void *ptr){
    Module *m=static_cast<Module *>(ptr);if(!m)return 0;
    ReverieNativeGlStateV1 saved={};
    ReverieNativeGlStateCaptureV1(&saved);
    m->program=m->vbo=m->staticVbo=0;
    const char *vs="uniform mat4 u_Mvp; attribute vec3 a_Pos;"
        "attribute vec3 a_Col;varying vec3 v_Col;"
        "void main(){gl_Position=u_Mvp*vec4(a_Pos,1.);v_Col=a_Col;}";
    const char *fs="precision mediump float;varying vec3 v_Col;"
        "void main(){gl_FragColor=vec4(v_Col,1.);}";
    const ReverieNativeGlAttributeBindingV1 bindings[]={
        {0u,"a_Pos"},{1u,"a_Col"}};
    char shaderDiagnostic[256] = {};
    m->program=ReverieNativeBuildProgramDiagnosticV1(
        vs,fs,bindings,2,shaderDiagnostic,sizeof(shaderDiagnostic));
    if(!m->program){
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,
            "ReverieClassicGL",shaderDiagnostic[0] != '\0'
                ? shaderDiagnostic : "GLES2 shader program unavailable");
    }
    if(m->program){
        m->pos=glGetAttribLocation(m->program,"a_Pos");
        m->col=glGetAttribLocation(m->program,"a_Col");
        m->mvp=glGetUniformLocation(m->program,"u_Mvp");
        glGenBuffers(1,&m->vbo);
        glGenBuffers(1,&m->staticVbo);
    }
    bool ok=m->program&&m->vbo&&m->staticVbo&&
        m->pos>=0&&m->col>=0&&m->mvp>=0;
#if CLASSIC_GAME_KIND == 5
    if(ok)ok=CreateChessSprites(m);
#endif
#if CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5
    if(ok)ok=CreateNativeFont(m);
#endif
    if(ok){
        // Bake immutable primitives once per GL context, for both eyes.
        Rebuild(m,true);
        glBindBuffer(GL_ARRAY_BUFFER,m->staticVbo);
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(
            m->staticVertices.size()*sizeof(float)),
            m->staticVertices.data(),GL_STATIC_DRAW);
#if CLASSIC_GAME_KIND == 5
        glBindBuffer(GL_ARRAY_BUFFER,m->chessBoardVbo);
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(
            m->chessBoardSprites.size()*sizeof(float)),
            m->chessBoardSprites.data(),GL_STATIC_DRAW);
#endif
#if CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5
        glBindBuffer(GL_ARRAY_BUFFER,m->fontStaticVbo);
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(
            m->fontStaticVertices.size()*sizeof(float)),
            m->fontStaticVertices.data(),GL_STATIC_DRAW);
#endif
        const GLenum error=glGetError();
        if(error!=GL_NO_ERROR){
            char detail[80];
            std::snprintf(detail,sizeof(detail),
                "Classic static geometry GL error 0x%x",
                static_cast<unsigned>(error));
            ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,
                               "ReverieClassicGL",detail);
            ok=false;
        }
        // Deterministic recipe stays in source; release expanded CPU copy.
        std::vector<float>().swap(m->staticVertices);
#if CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5
        std::vector<float>().swap(m->fontStaticVertices);
#endif
    }
    if(!ok)DestroyGl(m);
    m->dirty=m->upload=true;
    ReverieNativeGlStateRestoreV1(&saved);
    return ok?1:0;
}
void GlRelease(void *ptr){Module *m=static_cast<Module *>(ptr);if(m)DestroyGl(m);}
void Resume(void *){}
void Pause(void *){}
void Update(void *ptr,const ReverieNativeInputV1 *input){
    Module *m=static_cast<Module *>(ptr);
    if(!m||!ReverieNativeInputHasBaseV1(input))return;
    ReverieNativeInputV1 copy=*input;
    ReverieNativeSanitizeBaseInputV1(&copy);
    ReverieNativeSanitizePointerV1(&copy);
    const bool pressed=copy.primary_down!=0;
    const bool edge=pressed&&!m->down;
    m->down=pressed;
    float hx=0,hy=0;
    bool aim=false;
    if(copy.pointer_kind!=REVERIE_NATIVE_POINTER_NONE &&
       std::abs(copy.pointer_direction[2])>.00001f){
        const float t=(-2.91f-copy.pointer_origin[2])/copy.pointer_direction[2];
        if(t>0&&t<30){
            hx=copy.pointer_origin[0]+t*copy.pointer_direction[0];
            hy=copy.pointer_origin[1]+t*copy.pointer_direction[1];
            aim=std::isfinite(hx)&&std::isfinite(hy);
        }
    }

#if CLASSIC_GAME_KIND == 6
    Breakout &game=m->breakout;
    const float oldPaddle=game.paddle,oldBallX=game.ballX;
    const float oldBallY=game.ballY;
    const int oldLives=game.lives,oldScore=game.score;
    const bool oldMoving=game.moving,oldFinished=game.finished;
    if(aim&&!game.finished)
        game.SetPaddle(hx/1.08f);
    if(edge)game.Press();
    game.Step(copy.delta_seconds,aim?0.f:copy.move_x);
    if(game.paddle!=oldPaddle||game.ballX!=oldBallX||
       game.ballY!=oldBallY||game.lives!=oldLives||
       game.score!=oldScore||game.moving!=oldMoving||
       game.finished!=oldFinished)m->dirty=true;
    return;
#else
#if CLASSIC_GAME_KIND >= 3
    const int prevPly=m->board.ply,prevSelected=m->board.selected;
    const int prevWinner=m->board.winner,prevSide=m->board.side;
    const bool prevFinished=m->board.finished;
    const int prevGoReviewRevision=m->board.goReviewRevision;
    const bool second=copy.secondary_down!=0;
    const bool secondaryEdge=second&&!m->previousSecondary;
    m->previousSecondary=second;
#if CLASSIC_GAME_KIND == 5
    float panelX=0.f,panelY=0.f;
    float leftPanelX=0.f,leftPanelY=0.f;
    const bool chessLeftPanelAim=
        copy.pointer_kind!=REVERIE_NATIVE_POINTER_NONE &&
        reverie::chesspanel::Hit(-1,copy.pointer_origin,
                                copy.pointer_direction,&leftPanelX,&leftPanelY);
    float wikiX=0.f,wikiY=0.f;
    bool wikiAim=false;
    if(copy.pointer_kind!=REVERIE_NATIVE_POINTER_NONE &&
       std::abs(copy.pointer_direction[2])>.00001f){
        const float t=(-2.75f-copy.pointer_origin[2])/copy.pointer_direction[2];
        if(t>0.f&&t<30.f){
            wikiX=copy.pointer_origin[0]+t*copy.pointer_direction[0];
            wikiY=copy.pointer_origin[1]+t*copy.pointer_direction[1];
            wikiAim=std::isfinite(wikiX)&&std::isfinite(wikiY)&&
                std::abs(wikiX)<2.82f&&std::abs(wikiY)<1.77f;
        }
    }
    const bool chessPanelAim=
        copy.pointer_kind!=REVERIE_NATIVE_POINTER_NONE &&
        reverie::chesspanel::Hit(1,copy.pointer_origin,
                                copy.pointer_direction,&panelX,&panelY);
    const int controlHover=chessPanelAim&&!m->board.finished&&
        m->board.side==1&&std::abs(panelX)<.49f&&
        panelY<-.82f&&panelY>-1.14f?1:0;
    const bool chessPanelHover=chessPanelAim &&
        std::abs(panelX)<.525f&&panelY>.70f&&panelY<1.06f;
#else
    const int controlHover=aim&&!m->board.finished&&m->board.side==1&&
        m->board.kind==reverie::boardgames::Kind::Checkers&&
        hx>1.49f&&hx<2.42f&&hy<-.45f&&hy>-.85f?1:0;
#endif
#if CLASSIC_GAME_KIND == 5
    if(chessPanelHover!=m->chessPanelHover){
        m->chessPanelHover=chessPanelHover;
        m->dirty=true;
        if(chessPanelHover)ReverieNativeRequestFeedbackV1(m->host,
                                    REVERIE_NATIVE_FEEDBACK_FOCUS);
    }
#endif
    if(controlHover!=m->boardControlHover){
        m->boardControlHover=controlHover;m->dirty=true;
        if(controlHover)ReverieNativeRequestFeedbackV1(m->host,
                                      REVERIE_NATIVE_FEEDBACK_FOCUS);
    }
    const int n=m->board.Size();
    const float gap=BoardSpacing(m->board);
    const float ax=copy.move_x,ay=copy.move_y;
    if(std::abs(ax)>.55f||std::abs(ay)>.55f){
        m->boardCursorActive=true;
        m->cursorRepeat+=copy.delta_seconds;
        if(m->cursorRepeat>=.17f){
            m->cursorRepeat=0.f;
            if(std::abs(ax)>std::abs(ay))
                m->boardCursorX=std::clamp(m->boardCursorX+(ax>0?1:-1),0,n-1);
            else
                m->boardCursorY=std::clamp(m->boardCursorY+(ay>0?1:-1),0,n-1);
            m->dirty=true;
        }
    }else m->cursorRepeat=.17f;
    if(secondaryEdge && m->board.side==1){
        if(m->board.kind==reverie::boardgames::Kind::Go){
            if(m->board.goReview)m->board.AcceptGoScore();
            else m->board.Pass();
        }
        else if(m->board.kind==reverie::boardgames::Kind::Chess){
            if(m->board.selected<0)
                m->board.chessDifficulty=(m->board.chessDifficulty+1)%3;
            else m->board.CyclePromotion();
            m->dirty=true;
            ReverieNativeRequestFeedbackV1(m->host,
                REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        }else if(m->board.kind==reverie::boardgames::Kind::Checkers &&
                 m->board.forced<0){
            m->board.checkerDifficulty=(m->board.checkerDifficulty+1)%3;
            m->dirty=true;
            ReverieNativeRequestFeedbackV1(m->host,
                REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        }
    }
    if(edge){
#if CLASSIC_GAME_KIND == 5
        if(m->chessWikiOpen) {
            if(wikiAim)ChessWikiClick(m,wikiX,wikiY);
            ReverieNativeRequestFeedbackV1(m->host,
                REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        } else if(
            (chessLeftPanelAim && std::abs(leftPanelX)<.82f &&
                std::abs(leftPanelY)<1.57f) ||
            (chessPanelAim && std::abs(panelX)<.82f &&
                std::abs(panelY)<1.57f &&
                controlHover!=1 &&
                !(m->board.ChessDrawClaimable()&&
                  panelY< -1.21f && panelY> -1.51f))) {
            ChessWikiOpen(m,chessLeftPanelAim?0:1);
            ReverieNativeRequestFeedbackV1(m->host,
                REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        } else
#endif
        if(m->board.finished||m->cpuFailed) {
            m->cpuFailed=false;
            m->board.Reset(m->board.kind);
            m->boardCursorActive=false;
            m->boardCursorX=m->boardCursorY=0;
        }else if(m->board.side==1) {
            if(aim && m->board.kind==reverie::boardgames::Kind::Go &&
               hx>1.49f&&hx<2.42f&&hy<-.99f&&hy> -1.50f){
                if(m->board.goReview)m->board.AcceptGoScore();
                else m->board.Pass();
            }else if(aim && m->board.kind==reverie::boardgames::Kind::Go &&
                     m->board.goReview &&
                     hx> -2.42f&&hx< -1.49f&&hy<-.99f&&hy> -1.50f)
                m->board.ResumeGoPlay();
            else if(
#if CLASSIC_GAME_KIND == 5
                    controlHover==1 ||
#endif
                    (aim && m->board.kind==reverie::boardgames::Kind::Checkers &&
                     hx>1.49f&&hx<2.42f&&hy<-.45f&&hy>-.85f)){
                int &difficulty=
                    m->board.kind==reverie::boardgames::Kind::Chess
                        ?m->board.chessDifficulty:m->board.checkerDifficulty;
                difficulty=(difficulty+1)%3;
                ReverieNativeRequestFeedbackV1(m->host,
                    REVERIE_NATIVE_FEEDBACK_ACTIVATION);
                m->dirty=true;
            }else if(
#if CLASSIC_GAME_KIND == 5
                    chessPanelAim && std::abs(panelX)<.49f &&
                    panelY<-1.21f && panelY>-1.51f &&
#else
                    false &&
#endif
                    m->board.kind==reverie::boardgames::Kind::Chess &&
                    m->board.ChessDrawClaimable())
                m->board.ClaimChessDraw();
            else if(aim &&
                    m->board.kind==reverie::boardgames::Kind::Checkers &&
                    m->board.CheckerDrawClaimable() &&
                    hx>1.49f&&hx<2.42f&&hy<-.99f&&hy> -1.50f)
                m->board.ClaimCheckerDraw();
            else {
                int x=m->boardCursorX,y=m->boardCursorY;
                if(aim){
                    m->boardCursorActive=false;
                    x=static_cast<int>(std::floor((hx+n*.5f*gap)/gap));
                    y=static_cast<int>(std::floor((n*.5f*gap-hy)/gap));
                }
                if(x>=0&&x<n&&y>=0&&y<n)
                    m->board.Select(y*n+x);
            }
        }
    }
    try {
        if(m->cpu.Poll(m->board))m->dirty=true;
        if(!m->cpuFailed&&!m->cpu.Busy()&&m->board.side==-1&&
           !m->board.finished)m->cpu.Start(m->board);
    }catch(const std::exception &error){
        m->cpuFailed=true;m->dirty=true;
        ReverieNativeLogV1(m->host,REVERIE_NATIVE_LOG_ERROR,
                            "ClassicCpu",error.what());
    }
#if CLASSIC_GAME_KIND == 5
    // Each pointer/cursor hover names the actual board piece and previews
    // exactly those king-safe destinations the chess engine would accept.
    // Opponent inspection uses hypothetical next-turn rules, never executes.
    int hover=-1;
    if(aim){
        const int x=static_cast<int>(std::floor((hx+4.f*gap)/gap));
        const int y=static_cast<int>(std::floor((4.f*gap-hy)/gap));
        if(x>=0&&x<8&&y>=0&&y<8)hover=y*8+x;
    }else if(m->boardCursorActive){
        hover=m->boardCursorY*8+m->boardCursorX;
    }
    if(m->chessWikiOpen)hover=-1;
    if(hover!=m->chessHoverSquare){
        m->chessHoverSquare=hover;
        m->dirty=true;
    }
    const int preview=m->board.selected>=0?m->board.selected:hover;
    if(preview!=m->chessPreviewSquare ||
       m->chessPreviewPly!=m->board.ply){
        m->chessPreviewSquare=preview;
        m->chessPreviewPly=m->board.ply;
        m->chessLegalDestinations=
            reverie::chessref::Destinations(m->board,preview);
        m->dirty=true;
    }
    if(preview>=0){
        m->chessPulseClock+=std::clamp(copy.delta_seconds,0.f,.1f);
        if(m->chessPulseClock>=.38f){
            m->chessPulseClock=0.f;
            m->chessPulseLit=!m->chessPulseLit;
            m->dirty=true;
        }
    }else if(!m->chessPulseLit){
        m->chessPulseLit=true;
        m->chessPulseClock=0.f;
        m->dirty=true;
    }
    // Advisory ECO lookup runs on the game's single Update, never during
    // either eye draw. Re-scan only when a legally played move increments ply.
    if(m->chessAnalyzedPly!=m->board.ply){
        m->chessAnalysis=reverie::chessbook::Analyze(m->board);
        m->chessAnalyzedPly=m->board.ply;
        m->dirty=true;
    }
#endif
    if(m->board.ply!=prevPly||m->board.selected!=prevSelected||
       m->board.side!=prevSide||m->board.finished!=prevFinished||
       m->board.winner!=prevWinner||
       m->board.goReviewRevision!=prevGoReviewRevision)m->dirty=true;
    return;
#endif
    // Snapshot only what the generated screen actually displays.
    const bool oldMoving=m->game.moving,oldFinished=m->game.finished;
    const bool oldTurn=m->game.player_turn;
    const int oldScore=m->game.score,oldOpponent=m->game.opponent;
    const int oldHead=m->game.snake[0],oldFood=m->game.food;
    const int oldPosition=m->game.position,oldRival=m->game.rival;
    const float oldPaddle=m->game.paddle,oldAi=m->game.ai;
    const float oldBallX=m->game.ball_x,oldBallY=m->game.ball_y;
    if(m->game.game==Game::Pong && aim && !m->game.finished)
        m->game.paddle=std::clamp(hy/1.19f,-.78f,.78f);
    if(m->game.game==Game::Snake){
        if(std::abs(copy.move_x)>.4f||std::abs(copy.move_y)>.4f){
            const float x=copy.move_x,y=copy.move_y;
            m->game.Direction(std::abs(x)>std::abs(y)?
                (x>0?1:3):(y>0?2:0));
        }
        if(edge&&aim){
            if(hx>1.57f)m->game.Direction(1);
            else if(hx< -1.57f)m->game.Direction(3);
            else if(hy>1.38f)m->game.Direction(0);
            else if(hy< -1.38f)m->game.Direction(2);
        }
    }
    if(edge)m->game.Press();
    m->game.Step(copy.delta_seconds,
                 m->game.game==Game::Pong?copy.move_y:copy.move_x);
    const State &now=m->game;
    bool visible=now.moving!=oldMoving||now.finished!=oldFinished;
    if(now.game==Game::Pong)
        visible|=now.score!=oldScore||now.opponent!=oldOpponent||
            now.paddle!=oldPaddle||now.ai!=oldAi||
            now.ball_x!=oldBallX||now.ball_y!=oldBallY;
    else if(now.game==Game::Snake)
        visible|=now.snake[0]!=oldHead||now.food!=oldFood||
            now.score!=oldScore;
    else
        visible|=now.position!=oldPosition||now.rival!=oldRival||
            now.player_turn!=oldTurn;
    if(visible)m->dirty=true;
#endif // CLASSIC_GAME_KIND == 6
}
int32_t Draw(void *ptr,const ReverieNativeEyeV1 *eye){
    Module *m=static_cast<Module *>(ptr);
    if(!m||!ReverieNativeEyeRenderableV1(eye)||!m->program||
       !m->vbo||!m->staticVbo)return 0;
    if(m->dirty)Rebuild(m,false);
    ReverieNativeGlStateV1 prior={};
    ReverieNativeGlAttribStateV1 a={},b={};
    ReverieNativeGlStateCaptureV1(&prior);
    ReverieNativeGlAttribCaptureV1(&a,static_cast<GLuint>(m->pos));
    ReverieNativeGlAttribCaptureV1(&b,static_cast<GLuint>(m->col));
#if CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5
    ReverieNativeGlAttribStateV1 fontAttrib={};
    if(m->fontReady)ReverieNativeGlAttribCaptureV1(
        &fontAttrib,static_cast<GLuint>(m->fontCol));
#endif
    glEnable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
    glUseProgram(m->program);glBindBuffer(GL_ARRAY_BUFFER,m->vbo);
    if(m->upload){
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(
            m->vertices.size()*sizeof(float)),m->vertices.data(),GL_DYNAMIC_DRAW);
        m->upload=false;
#if CLASSIC_GAME_KIND == 5
        glBindBuffer(GL_ARRAY_BUFFER,m->chessVbo);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(m->chessSprites.size()*sizeof(float)),
            m->chessSprites.data(),GL_DYNAMIC_DRAW);
#endif
#if CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5
        glBindBuffer(GL_ARRAY_BUFFER,m->fontVbo);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(m->fontVertices.size()*sizeof(float)),
            m->fontVertices.data(),GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER,m->vbo);
#endif
    }
    glEnableVertexAttribArray(static_cast<GLuint>(m->pos));
    glEnableVertexAttribArray(static_cast<GLuint>(m->col));
    float vp[16];ReverieNativeMat4Multiply(vp,eye->projection,eye->view);
    glUniformMatrix4fv(m->mvp,1,GL_FALSE,vp);
    const auto drawLayer=[&](GLuint buffer,GLsizei count){
        if(count<=0)return;
        glBindBuffer(GL_ARRAY_BUFFER,buffer);
        glVertexAttribPointer(static_cast<GLuint>(m->pos),3,GL_FLOAT,
                              GL_FALSE,6*sizeof(float),reinterpret_cast<void *>(0));
        glVertexAttribPointer(static_cast<GLuint>(m->col),3,GL_FLOAT,
                              GL_FALSE,6*sizeof(float),
                              reinterpret_cast<void *>(3*sizeof(float)));
        glDrawArrays(GL_TRIANGLES,0,count);
    };
    drawLayer(m->staticVbo,m->staticCount);
#if CLASSIC_GAME_KIND == 5
    // Board must be rendered after the fixed backing but BEFORE highlights,
    // control markers and pieces; its opaque RGB texture writes depth.
    if(m->chessReady && !m->chessBoardSprites.empty()){
        glUseProgram(m->chessProgram);
        glBindBuffer(GL_ARRAY_BUFFER,m->chessBoardVbo);
        glVertexAttribPointer(static_cast<GLuint>(m->chessPos),3,GL_FLOAT,
            GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(0));
        glVertexAttribPointer(static_cast<GLuint>(m->chessUv),2,GL_FLOAT,
            GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
        glUniformMatrix4fv(m->chessMvp,1,GL_FALSE,vp);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,m->chessBoardTexture);
        glUniform1i(m->chessSampler,0);
        glDrawArrays(GL_TRIANGLES,0,
            static_cast<GLsizei>(m->chessBoardSprites.size()/5));
        glUseProgram(m->program);
        glUniformMatrix4fv(m->mvp,1,GL_FALSE,vp);
    }
#endif
    drawLayer(m->vbo,static_cast<GLsizei>(m->vertices.size()/6));

#if CLASSIC_GAME_KIND == 5
    // One GPU atlas and one dynamic quad buffer for both stereo eyes.
    if(m->chessReady && !m->chessSprites.empty()){
        GLint srcRgb=GL_ONE,dstRgb=GL_ZERO,srcAlpha=GL_ONE,dstAlpha=GL_ZERO;
        GLboolean depthWrite=GL_TRUE;
        glGetIntegerv(GL_BLEND_SRC_RGB,&srcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB,&dstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA,&srcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA,&dstAlpha);
        glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(m->chessProgram);
        glBindBuffer(GL_ARRAY_BUFFER,m->chessVbo);
        glVertexAttribPointer(static_cast<GLuint>(m->chessPos),3,GL_FLOAT,
            GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(0));
        glVertexAttribPointer(static_cast<GLuint>(m->chessUv),2,GL_FLOAT,
            GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
        glUniformMatrix4fv(m->chessMvp,1,GL_FALSE,vp);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,m->chessTexture);
        glUniform1i(m->chessSampler,0);
        glDrawArrays(GL_TRIANGLES,0,
            static_cast<GLsizei>(m->chessSprites.size()/5));
        glDepthMask(depthWrite);
        glBlendFuncSeparate(static_cast<GLenum>(srcRgb),
            static_cast<GLenum>(dstRgb),static_cast<GLenum>(srcAlpha),
            static_cast<GLenum>(dstAlpha));
    }
#endif

#if CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5
    if(m->fontReady){
        GLint srcRgb=GL_ONE,dstRgb=GL_ZERO,srcAlpha=GL_ONE,dstAlpha=GL_ZERO;
        GLboolean depthWrite=GL_TRUE;
        glGetIntegerv(GL_BLEND_SRC_RGB,&srcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB,&dstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA,&srcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA,&dstAlpha);
        glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(m->fontProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,m->fontTexture);
        glUniform1i(m->fontSampler,0);
        glUniformMatrix4fv(m->fontMvp,1,GL_FALSE,vp);
        glEnableVertexAttribArray(static_cast<GLuint>(m->fontPos));
        glEnableVertexAttribArray(static_cast<GLuint>(m->fontUv));
        glEnableVertexAttribArray(static_cast<GLuint>(m->fontCol));
        const auto drawFont=[&](GLuint buffer,GLsizei count){
            if(count<=0)return;
            glBindBuffer(GL_ARRAY_BUFFER,buffer);
            glVertexAttribPointer(static_cast<GLuint>(m->fontPos),3,GL_FLOAT,
                GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(0));
            glVertexAttribPointer(static_cast<GLuint>(m->fontUv),2,GL_FLOAT,
                GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
            glVertexAttribPointer(static_cast<GLuint>(m->fontCol),3,GL_FLOAT,
                GL_FALSE,8*sizeof(float),reinterpret_cast<void*>(5*sizeof(float)));
            glDrawArrays(GL_TRIANGLES,0,count);
        };
        drawFont(m->fontStaticVbo,m->fontStaticCount);
        drawFont(m->fontVbo,static_cast<GLsizei>(m->fontVertices.size()/8));
        glDepthMask(depthWrite);
        glBlendFuncSeparate(static_cast<GLenum>(srcRgb),
            static_cast<GLenum>(dstRgb),static_cast<GLenum>(srcAlpha),
            static_cast<GLenum>(dstAlpha));
        ReverieNativeGlAttribRestoreV1(&fontAttrib);
    }
#endif
    ReverieNativeGlAttribRestoreV1(&b);
    ReverieNativeGlAttribRestoreV1(&a);
    ReverieNativeGlStateRestoreV1(&prior);
    return glGetError()==GL_NO_ERROR?1:0;
}
const ReverieNativeModuleApiV1 api={
    sizeof(ReverieNativeModuleApiV1),REVERIE_NATIVE_MODULE_ABI_VERSION,
    {sizeof(ReverieNativeModuleDescriptorV1),
     REVERIE_NATIVE_MODULE_ABI_VERSION,kId,kTitle,2u,0u},
    Create,Destroy,GlCreate,GlRelease,Resume,Pause,Update,Draw,nullptr
};
}
extern "C" __attribute__((visibility("default")))
const ReverieNativeModuleApiV1 *reverie_native_module_entry_v1(void){
    return &api;
}

