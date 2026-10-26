#ifndef REVERIE_NATIVE_LATO_TEXT_H
#define REVERIE_NATIVE_LATO_TEXT_H
// Genuine licensed Lato Semibold atlas (SIL OFL 1.1) shared across game HUDs.
// Draws the original atlas glyph alpha as textured quads, NOT procedural glyph
// strokes/placeholder 3D letters. One GL texture and VBO per context, one
// uploaded dynamic glyph batch per changed screen; unchanged across both eyes.
// Existing source/license: breakwater_font_atlas.h, LICENSES/OFL-1.1.txt.
#include "breakwater_font_atlas.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include <GLES2/gl2.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace reverie { namespace nativetext {
struct LatoLayer {
    std::vector<float> vertices;
    GLuint program=0,texture=0,vbo=0;
    GLint pos=-1,uv=-1,col=-1,mvp=-1,sampler=-1;
    bool upload=true;
    // Titles and labels keep the authored game layout/scale; genuine metrics
    // replace per-character four-unit stroke-letter approximations.
    void Clear(){vertices.clear();upload=true;}
    void Text(const char* value,float x,float y,float scale,
              float r,float g,float b,float z=-2.85f){
        reverie::breakwater::font::AppendText(vertices,value,x,y,
                                               scale,r,g,b,z);
        upload=true;
    }
    void Fit(const char* value,float x,float y,float width,float scale,
             float r,float g,float b,float z=-2.85f){
        if(!value||width<=0.f)return;
        Text(value,x,y,
             reverie::breakwater::font::FittedScale(value,width,scale),
             r,g,b,z);
    }
    void Release(){
        if(vbo)glDeleteBuffers(1,&vbo);
        if(texture)glDeleteTextures(1,&texture);
        if(program)glDeleteProgram(program);
        vbo=texture=program=0;
        pos=uv=col=mvp=sampler=-1;
        upload=true;
    }
    bool Create(){
        // Calling host must already have a current GLES2 context.
        ReverieNativeGlStateV1 old{};
        ReverieNativeGlStateCaptureV1(&old);
        Release();
        const char* vs=
          "uniform mat4 u_Mvp;attribute vec3 a_Pos;"
          "attribute vec2 a_Uv;attribute vec3 a_Col;"
          "varying vec2 v_Uv;varying vec3 v_Col;"
          "void main(){gl_Position=u_Mvp*vec4(a_Pos,1.);"
          "v_Uv=a_Uv;v_Col=a_Col;}";
        const char* fs=
          "precision mediump float;uniform sampler2D u_Glyph;"
          "varying vec2 v_Uv;varying vec3 v_Col;"
          "void main(){float coverage=texture2D(u_Glyph,v_Uv).a;"
          "if(coverage<.01)discard;gl_FragColor=vec4(v_Col,coverage);}";
        const ReverieNativeGlAttributeBindingV1 bindings[]={
            {3u,"a_Pos"},{4u,"a_Uv"},{5u,"a_Col"}};
        char diag[256]={};
        program=ReverieNativeBuildProgramDiagnosticV1(
            vs,fs,bindings,3u,diag,sizeof(diag));
        if(program){
            pos=glGetAttribLocation(program,"a_Pos");
            uv=glGetAttribLocation(program,"a_Uv");
            col=glGetAttribLocation(program,"a_Col");
            mvp=glGetUniformLocation(program,"u_Mvp");
            sampler=glGetUniformLocation(program,"u_Glyph");
        }
        std::vector<unsigned char> alpha;
        bool good=program&&pos==3&&uv==4&&col==5&&mvp>=0&&
                  sampler>=0&&reverie::breakwater::font::DecodeAtlas(alpha);
        if(good){
            glGenBuffers(1,&vbo);
            glGenTextures(1,&texture);
            good=vbo!=0&&texture!=0;
        }
        if(good){
            GLint unpack=4;
            glGetIntegerv(GL_UNPACK_ALIGNMENT,&unpack);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,texture);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glPixelStorei(GL_UNPACK_ALIGNMENT,1);
            glTexImage2D(GL_TEXTURE_2D,0,GL_ALPHA,
                reverie::breakwater::font::kWidth,
                reverie::breakwater::font::kHeight,0,GL_ALPHA,
                GL_UNSIGNED_BYTE,alpha.data());
            glPixelStorei(GL_UNPACK_ALIGNMENT,unpack);
            good=glGetError()==GL_NO_ERROR;
        }
        if(!good)Release();
        ReverieNativeGlStateRestoreV1(&old);
        upload=true;
        return good;
    }
    bool Draw(const float* vp){
        if(!program||!texture||!vbo||!vp)return false;
        if(vertices.empty())return true;
        // The three reserved attribute slots do not conflict with the game
        // mesh (0/1) or Gumball artwork (2). Restore their COMPLETE state.
        ReverieNativeGlAttribStateV1 attrs[3]{};
        ReverieNativeGlStateV1 old{};
        ReverieNativeGlStateCaptureV1(&old);
        for(int i=0;i<3;++i)
            ReverieNativeGlAttribCaptureV1(&attrs[i],3u+GLuint(i));
        GLint srcRgb=GL_ONE,dstRgb=GL_ZERO,srcAlpha=GL_ONE,dstAlpha=GL_ZERO;
        glGetIntegerv(GL_BLEND_SRC_RGB,&srcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB,&dstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA,&srcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA,&dstAlpha);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,texture);
        glUniform1i(sampler,0);
        glUniformMatrix4fv(mvp,1,GL_FALSE,vp);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        if(upload){
            glBufferData(GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(vertices.size()*sizeof(float)),
                vertices.data(),GL_DYNAMIC_DRAW);
            upload=false;
        }
        for(int i=0;i<3;++i)glEnableVertexAttribArray(3u+GLuint(i));
        glVertexAttribPointer(3u,3,GL_FLOAT,GL_FALSE,
                              8*sizeof(float),reinterpret_cast<void*>(0));
        glVertexAttribPointer(4u,2,GL_FLOAT,GL_FALSE,
                              8*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
        glVertexAttribPointer(5u,3,GL_FLOAT,GL_FALSE,
                              8*sizeof(float),reinterpret_cast<void*>(5*sizeof(float)));
        glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()/8u));
        const GLenum error=glGetError();
        glBlendFuncSeparate(static_cast<GLenum>(srcRgb),
            static_cast<GLenum>(dstRgb),static_cast<GLenum>(srcAlpha),
            static_cast<GLenum>(dstAlpha));
        for(int i=2;i>=0;--i)ReverieNativeGlAttribRestoreV1(&attrs[i]);
        ReverieNativeGlStateRestoreV1(&old);
        return error==GL_NO_ERROR;
    }
};
}}
#endif
