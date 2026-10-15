#include "classic_games.h"
#include "classic_breakout.h"
#include "classic_board_games.h"
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
#else
#error "Unknown classic game"
#endif
struct Module {
    const ReverieNativeHostV1 *host=nullptr;
    State game;
    reverie::boardgames::State board;
#if CLASSIC_GAME_KIND == 6
    Breakout breakout;
#endif
    int boardCursorX=0,boardCursorY=0;
    bool boardCursorActive=false;
    float cursorRepeat=0.f;
    bool previousSecondary=false;
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
const char *Glyph(char c){
    switch(c){
        case 'A':return "010101111101101";case 'B':return "110101110101110";
        case 'C':return "011100100100011";case 'D':return "110101101101110";
        case 'E':return "111100110100111";case 'F':return "111100110100100";
        case 'G':return "011100101101011";case 'H':return "101101111101101";
        case 'I':return "111010010010111";case 'J':return "001001001101010";
        case 'K':return "101101110101101";case 'L':return "100100100100111";
        case 'M':return "101111111101101";case 'N':return "101111111111101";
        case 'O':return "111101101101111";case 'P':return "110101110100100";
        case 'Q':return "111101101111011";case 'R':return "110101110101101";
        case 'S':return "011100010001110";case 'T':return "111010010010010";
        case 'U':return "101101101101111";case 'V':return "101101010101010";
        case 'W':return "101101111111101";case 'X':return "101101010101101";
        case 'Y':return "101101010010010";case 'Z':return "111001010100111";
        case '0':return "111101101101111";case '1':return "010110010010111";
        case '2':return "110001111100111";case '3':return "110001110001110";
        case '4':return "101101111001001";case '5':return "111100110001110";
        case '6':return "011100111101111";case '7':return "111001010010010";
        case '8':return "111101111101111";case '9':return "111101111001110";
        case '+':return "000010111010000";case '-':return "000000111000000";
        case ':':return "000010000010000";default:return "000000000000000";
    }
}
void Text(Module *m,const char *s,float x,float y,float scale,
          float r,float g,float b){
    if(!s)return;
    float px=x-2.f*scale*std::strlen(s);
    for(size_t k=0;k<std::strlen(s);k++){
        char c=s[k];
        if(c>='a'&&c<='z')c=static_cast<char>(c-'a'+'A');
        const char *bitmap=Glyph(c);
        for(int i=0;i<15;i++)if(bitmap[i]=='1')
            Rect(m,px+(i%3+.5f)*scale,y+(2.5f-i/3)*scale,
                 scale*.93f,scale*.93f,-2.89f,r,g,b);
        px+=4*scale;
    }
}
void Segment(Module *m,float x,float y,float xx,float yy,float width,
             float r,float g,float b){
    reverie::flatmesh::Segment(Emit(m),x,y,xx,yy,width,-2.87f,r,g,b);
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
void RebuildBoard(Module *m,bool fixed){
    m->staticPass=fixed;
    if(fixed)m->staticVertices.clear();
    else m->vertices.clear();
    const reverie::boardgames::State &s=m->board;
    const int size=s.Size();
    const float gap=BoardSpacing(s);
    if(fixed){
        Rect(m,0,0,5.5f,3.55f,-3.10f,.032f,.058f,.092f);
        Rect(m,0,0,3.08f,3.08f,-3.045f,.34f,.24f,.15f);
        Text(m,kTitle,0,1.58f,.082f,.91f,.94f,.94f);
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
            }else{
                Disc(m,px,py,.125f,-2.947f,
                     piece>0?.92f:.20f,piece>0?.92f:.23f,
                     piece>0?.80f:.36f);
                Disc(m,px,py,.096f,-2.934f,
                     piece>0?.73f:.42f,piece>0?.79f:.55f,
                     piece>0?.69f:.65f);
                if(s.kind==reverie::boardgames::Kind::Chess){
                    const char letters[]="?PNBRQK";
                    char label[]={letters[std::abs(piece)],'\0'};
                    Text(m,label,px,py-.005f,.040f,
                         piece>0?.05f:.97f,piece>0?.10f:.97f,
                         piece>0?.16f:.93f);
                }else if(std::abs(piece)==2){
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
        if(s.lastTo==cell){
            const float a=gap*.48f;
            Rect(m,px-a,py-a,.045f,.045f,-2.917f,.95f,.86f,.27f);
            Rect(m,px+a,py+a,.045f,.045f,-2.917f,.95f,.86f,.27f);
        }
        if(s.selected==cell)
            BoardOutline(m,px,py,gap*.96f,1.f,.99f,.20f);
        if(m->boardCursorActive && m->boardCursorX==x &&
           m->boardCursorY==y)
            BoardOutline(m,px,py,gap*.76f,.93f,.98f,.99f);
        }
    }
    if(fixed && s.kind==reverie::boardgames::Kind::Go){
        Rect(m,1.94f,-1.25f,.89f,.41f,-2.98f,.23f,.36f,.42f);
        Text(m,"PASS",1.94f,-1.25f,.067f,1.f,1.f,.89f);
        Text(m,"TWO PASSES END",0,-1.58f,.048f,.80f,.90f,.87f);
    }else if(fixed){
        Text(m,"SELECT PIECE THEN DESTINATION",0,-1.58f,.040f,
             .84f,.90f,.87f);
    }
    if(!fixed && s.finished){
        const char *msg=s.winner==1?"YOU WIN":s.winner==-1?
            "CPU WINS":"DRAW";
        Text(m,msg,0,1.34f,.074f,1.f,.91f,.58f);
        Text(m,"PRESS TO RESTART",1.91f,.66f,.043f,1.f,.95f,.78f);
    }else if(!fixed){
        Text(m,s.side==1?"YOUR TURN":"CPU THINKING",
             0,1.34f,.064f,.95f,.95f,.78f);
        if(s.kind==reverie::boardgames::Kind::Checkers && s.forced>=0)
            Text(m,"CONTINUE JUMP",1.88f,.63f,.044f,1.f,.90f,.56f);
        if(s.kind==reverie::boardgames::Kind::Chess&&s.side==1){
            if(s.selected>=0&&s.selected<64&&
               s.cells[s.selected]==1&&s.selected/8==1){
                const char *promo=s.promotionChoice==4?"PROMO ROOK":
                    s.promotionChoice==3?"PROMO BISHOP":
                    s.promotionChoice==2?"PROMO KNIGHT":"PROMO QUEEN";
                Text(m,promo,1.88f,.66f,.036f,.91f,.92f,.75f);
            }
            if(s.ChessDrawClaimable()){
                Rect(m,1.94f,-1.25f,.96f,.39f,-2.98f,
                     .23f,.36f,.42f);
                Text(m,"CLAIM DRAW",1.94f,-1.25f,.042f,
                     1.f,1.f,.89f);
            }
        }
    }
    if(fixed)m->staticCount=static_cast<GLsizei>(m->staticVertices.size()/6);
    else {m->dirty=false;m->upload=true;}
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
        }
        const int start[]={3,8,28,58,75,17,54,62,87,99};
        for(int i=0;i<10;i++){
            const int from=start[i],to=State::Jump(from);
            Segment(m,CellX(from),CellY(from),CellX(to),CellY(to),
                    .055f,i<5?.32f:.95f,i<5?.96f:.40f,.50f);
        }
        Rect(m,0,-1.56f,2.8f,.29f,-2.97f,.22f,.33f,.35f);
        }else{
        if(s.position>0)Rect(m,CellX(s.position)-.06f,CellY(s.position),
                             .11f,.15f,-2.92f,.32f,.96f,.73f);
        if(s.rival>0)Rect(m,CellX(s.rival)+.06f,CellY(s.rival),
                          .11f,.15f,-2.92f,1.f,.61f,.47f);
        std::snprintf(status,sizeof(status),"YOU %d   CPU %d",s.position,s.rival);
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
    if(m->vbo)glDeleteBuffers(1,&m->vbo);
    if(m->staticVbo)glDeleteBuffers(1,&m->staticVbo);
    if(m->program)glDeleteProgram(m->program);
    m->vbo=m->staticVbo=m->program=0;
    m->staticCount=0;
    m->dirty=m->upload=true;
}
void *Create(const ReverieNativeHostV1 *host){
    Module *m=new(std::nothrow) Module();
    if(m){
        m->host=host;
#if CLASSIC_GAME_KIND >= 3 && CLASSIC_GAME_KIND <= 5
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
            "ReverieClassicGL",shaderDiagnostic[0] != ' '
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
    if(ok){
        // Bake immutable primitives once per GL context, for both eyes.
        Rebuild(m,true);
        glBindBuffer(GL_ARRAY_BUFFER,m->staticVbo);
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(
            m->staticVertices.size()*sizeof(float)),
            m->staticVertices.data(),GL_STATIC_DRAW);
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
    const bool second=copy.secondary_down!=0;
    const bool secondaryEdge=second&&!m->previousSecondary;
    m->previousSecondary=second;
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
        if(m->board.kind==reverie::boardgames::Kind::Go)
            m->board.Pass();
        else if(m->board.kind==reverie::boardgames::Kind::Chess){
            m->board.CyclePromotion();
            m->dirty=true;
        }
    }
    if(edge){
        if(m->board.finished) {
            m->board.Reset(m->board.kind);
            m->boardCursorActive=false;
            m->boardCursorX=m->boardCursorY=0;
        }else if(m->board.side==1) {
            if(aim && m->board.kind==reverie::boardgames::Kind::Go &&
               hx>1.49f&&hx<2.42f&&hy<-.99f&&hy> -1.50f)
                m->board.Pass();
            else if(aim &&
                    m->board.kind==reverie::boardgames::Kind::Chess &&
                    m->board.ChessDrawClaimable() &&
                    hx>1.49f&&hx<2.42f&&hy<-.99f&&hy> -1.50f)
                m->board.ClaimChessDraw();
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
    m->board.Step(copy.delta_seconds);
    if(m->board.ply!=prevPly||m->board.selected!=prevSelected||
       m->board.side!=prevSide||m->board.finished!=prevFinished||
       m->board.winner!=prevWinner)m->dirty=true;
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
    glEnable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
    glUseProgram(m->program);glBindBuffer(GL_ARRAY_BUFFER,m->vbo);
    if(m->upload){
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(
            m->vertices.size()*sizeof(float)),m->vertices.data(),GL_DYNAMIC_DRAW);
        m->upload=false;
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
    drawLayer(m->vbo,static_cast<GLsizei>(m->vertices.size()/6));
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
