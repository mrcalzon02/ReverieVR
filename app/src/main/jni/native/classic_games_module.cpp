#include "classic_games.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
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
#else
#error "Unknown classic game"
#endif
struct Module {
    State game;
    GLuint program=0,vbo=0;
    GLint pos=-1,col=-1,mvp=-1;
    bool down=false,dirty=true,upload=true;
    std::vector<float> vertices;
};
void Vertex(Module *m,float x,float y,float z,float r,float g,float b){
    auto &a=m->vertices;
    a.insert(a.end(),{x,y,z,r,g,b});
}
void Triangle(Module *m,float ax,float ay,float bx,float by,
              float cx,float cy,float z,float r,float g,float b){
    Vertex(m,ax,ay,z,r,g,b);
    Vertex(m,bx,by,z,r,g,b);
    Vertex(m,cx,cy,z,r,g,b);
}
void Rect(Module *m,float x,float y,float w,float h,
          float z,float r,float g,float b){
    const float l=x-w*.5f,q=x+w*.5f,d=y-h*.5f,t=y+h*.5f;
    Triangle(m,l,d,q,d,q,t,z,r,g,b);
    Triangle(m,l,d,q,t,l,t,z,r,g,b);
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
void Segment(Module *m,float x,float y,float xx,float yy,
             float width,float r,float g,float b){
    const float dx=xx-x,dy=yy-y;
    const float length=std::sqrt(dx*dx+dy*dy);
    if(length<.00001f)return;
    const float px=-dy/length*width*.5f,py=dx/length*width*.5f;
    Triangle(m,x+px,y+py,x-px,y-py,xx+px,yy+py,-2.87f,r,g,b);
    Triangle(m,xx+px,yy+py,x-px,y-py,xx-px,yy-py,-2.87f,r,g,b);
}
float CellX(int n){const int idx=n-1,row=idx/10,col=idx%10;
    return -1.24f+(row%2==0?col:9-col)*.274f;}
float CellY(int n){return -1.22f+((n-1)/10)*.274f;}
void Rebuild(Module *m){
    m->vertices.clear();
    const State &s=m->game;
    Rect(m,0,0,5.5f,3.55f,-3.10f,.032f,.058f,.092f);
    Text(m,kTitle,0,1.57f,.092f,.78f,.94f,.96f);
    char status[48];
    if(s.game==Game::Pong){
        Rect(m,0,0,3.7f,2.60f,-3.04f,.08f,.13f,.20f);
        for(int i=0;i<9;i++)
            Rect(m,0,-1.16f+i*.29f,.032f,.13f,-2.99f,.30f,.39f,.43f);
        Rect(m,s.paddle*1.55f,-1.12f,.68f,.075f,-2.96f,.33f,.95f,.78f);
        Rect(m,s.ai*1.55f,1.12f,.68f,.075f,-2.96f,.98f,.60f,.44f);
        Rect(m,s.ball_x*1.55f,s.ball_y*1.22f,.11f,.11f,-2.93f,.94f,.95f,.70f);
        std::snprintf(status,sizeof(status),"%d : %d",s.score,s.opponent);
        Text(m,status,0,1.31f,.07f,1,1,.79f);
        Text(m,s.finished?"PRESS TO RESTART":
             s.moving?"MOVE PADDLE TO BLOCK":"PRESS TO SERVE",
             0,-1.46f,.040f,.70f,.83f,.83f);
    }else if(s.game==Game::Snake){
        Rect(m,0,0,2.63f,2.62f,-3.04f,.06f,.16f,.15f);
        for(int y=0;y<State::kGrid;y++)for(int x=0;x<State::kGrid;x++){
            const float px=(x-5.5f)*.21f,py=(5.5f-y)*.21f;
            Rect(m,px,py,.19f,.19f,-3.0f,
                 (x+y)%2?.085f:.075f,.19f,.18f);
        }
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
        const char *dirs[]={"UP","RIGHT","DOWN","LEFT"};
        const float x[]={0,2.02f,0,-2.02f};
        const float y[]={1.02f,0,-1.02f,0};
        for(int i=0;i<4;i++){
            Rect(m,x[i],y[i],i%2?.79f:.89f,.36f,
                 -2.97f,.20f,.33f,.39f);
            Text(m,dirs[i],x[i],y[i],.052f,.85f,.93f,.88f);
        }
        std::snprintf(status,sizeof(status),"FOOD %d",s.score);
        Text(m,status,0,1.34f,.06f,.86f,.91f,.68f);
        Text(m,s.finished?"PRESS TO RESTART":
             s.moving?"POINT TO TURN":"PRESS TO START",
             0,-1.48f,.043f,.78f,.90f,.83f);
    }else{
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
        if(s.position>0)Rect(m,CellX(s.position)-.06f,CellY(s.position),
                             .11f,.15f,-2.92f,.32f,.96f,.73f);
        if(s.rival>0)Rect(m,CellX(s.rival)+.06f,CellY(s.rival),
                          .11f,.15f,-2.92f,1.f,.61f,.47f);
        std::snprintf(status,sizeof(status),"YOU %d   CPU %d",s.position,s.rival);
        Text(m,status,0,1.36f,.047f,.94f,.89f,.68f);
        Rect(m,0,-1.56f,2.8f,.29f,-2.97f,.22f,.33f,.35f);
        Text(m,s.finished?(s.position==100?"YOU WIN PRESS":"CPU WINS PRESS"):
             s.player_turn?"SELECT TO ROLL":"CPU ROLLING",
             0,-1.56f,.045f,1,.96f,.78f);
    }
    m->dirty=false;m->upload=true;
}
void DestroyGl(Module *m){
    if(m->vbo)glDeleteBuffers(1,&m->vbo);
    if(m->program)glDeleteProgram(m->program);
    m->vbo=m->program=0;m->dirty=m->upload=true;
}
void *Create(const ReverieNativeHostV1 *){
    Module *m=new(std::nothrow) Module();
    if(m)m->game.Reset(static_cast<Game>(CLASSIC_GAME_KIND));
    return m;
}
void Destroy(void *ptr){delete static_cast<Module *>(ptr);}
int32_t GlCreate(void *ptr){
    Module *m=static_cast<Module *>(ptr);if(!m)return 0;
    ReverieNativeGlStateV1 saved={};
    ReverieNativeGlStateCaptureV1(&saved);
    m->program=m->vbo=0;
    const char *vs="uniform mat4 u_Mvp; attribute vec3 a_Pos;"
        "attribute vec3 a_Col;varying vec3 v_Col;"
        "void main(){gl_Position=u_Mvp*vec4(a_Pos,1.);v_Col=a_Col;}";
    const char *fs="precision mediump float;varying vec3 v_Col;"
        "void main(){gl_FragColor=vec4(v_Col,1.);}";
    const ReverieNativeGlAttributeBindingV1 bindings[]={
        {0u,"a_Pos"},{1u,"a_Col"}};
    m->program=ReverieNativeBuildProgram(vs,fs,bindings,2);
    if(m->program){
        m->pos=glGetAttribLocation(m->program,"a_Pos");
        m->col=glGetAttribLocation(m->program,"a_Col");
        m->mvp=glGetUniformLocation(m->program,"u_Mvp");
        glGenBuffers(1,&m->vbo);
    }
    const bool ok=m->program&&m->vbo&&m->pos>=0&&m->col>=0&&m->mvp>=0;
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
    if(m->game.game==Game::Pong && aim && !m->game.finished)
        m->game.paddle=std::clamp(hx/1.55f,-.78f,.78f);
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
    m->game.Step(copy.delta_seconds,copy.move_x);
    m->dirty=true;
}
int32_t Draw(void *ptr,const ReverieNativeEyeV1 *eye){
    Module *m=static_cast<Module *>(ptr);
    if(!m||!ReverieNativeEyeRenderableV1(eye)||!m->program||!m->vbo)
        return 0;
    if(m->dirty)Rebuild(m);
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
    glVertexAttribPointer(static_cast<GLuint>(m->pos),3,GL_FLOAT,
                          GL_FALSE,6*sizeof(float),reinterpret_cast<void *>(0));
    glVertexAttribPointer(static_cast<GLuint>(m->col),3,GL_FLOAT,
                          GL_FALSE,6*sizeof(float),
                          reinterpret_cast<void *>(3*sizeof(float)));
    float vp[16];ReverieNativeMat4Multiply(vp,eye->projection,eye->view);
    glUniformMatrix4fv(m->mvp,1,GL_FALSE,vp);
    glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(m->vertices.size()/6));
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
