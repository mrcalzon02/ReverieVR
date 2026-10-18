#include "companion_showcase_renderer.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include <android/log.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

bool TestShowcase::Add(int id,const std::vector<float>&data,
                       int width,int height,const std::vector<uint32_t>&argb){
    if(id<0||id>4||width<1||height<1||width>1024||height>1024||
       data.empty()||data.size()%15u!=0u||data.size()>90000u||
       meshes.size()>=24u||argb.size()!=size_t(width)*size_t(height))
        return false;
    for(float value:data)if(!std::isfinite(value))return false;
    TestShowcaseMesh m;
    m.model=id;m.width=width;m.height=height;
    m.xyzuv=data;m.vertex_count=static_cast<GLsizei>(data.size()/5u);
    m.rgba.resize(argb.size()*4u);
    for(size_t i=0;i<argb.size();++i){
        const uint32_t p=argb[i];
        m.rgba[i*4u+0u]=static_cast<uint8_t>(p>>16u);
        m.rgba[i*4u+1u]=static_cast<uint8_t>(p>>8u);
        m.rgba[i*4u+2u]=static_cast<uint8_t>(p);
        m.rgba[i*4u+3u]=static_cast<uint8_t>(p>>24u);
    }
    meshes.push_back(std::move(m));
    return true;
}
bool TestShowcase::HasFiveModels()const{
    uint32_t mask=0;
    for(const auto &m:meshes)mask|=(1u<<static_cast<unsigned>(m.model));
    return mask==31u;
}
bool TestShowcase::CreateGl(){
    // Context may be recreated; old IDs no longer belong to this context.
    program=0;
    for(auto &m:meshes){m.vbo=0;m.texture=0;}
    if(!HasFiveModels())return false;
    static const char *vs=
        "uniform mat4 uMvp;\n"
        "attribute vec3 aPosition;\n"
        "attribute vec2 aUv;\n"
        "varying vec2 vUv;\n"
        "void main(){gl_Position=uMvp*vec4(aPosition,1.0);vUv=aUv;}\n";
    static const char *fs=
        "precision mediump float;\n"
        "uniform sampler2D uTex;\n"
        "varying vec2 vUv;\n"
        "void main(){gl_FragColor=texture2D(uTex,vUv);}\n";
    const ReverieNativeGlAttributeBindingV1 binds[]={
        {0u,"aPosition"},{1u,"aUv"}
    };
    ReverieNativeGlStateV1 state={};
    ReverieNativeGlStateCaptureV1(&state);
    auto restore=[&](){ReverieNativeGlStateRestoreV1(&state);};
    char detail[256]={};
    program=ReverieNativeBuildProgramDiagnosticV1(
        vs,fs,binds,2u,detail,sizeof(detail));
    if(!program){
        __android_log_print(ANDROID_LOG_ERROR,"TestChamberAssets",
            "GLB shader failed: %s",detail);
        restore();return false;
    }
    position=glGetAttribLocation(program,"aPosition");
    uv=glGetAttribLocation(program,"aUv");
    matrix=glGetUniformLocation(program,"uMvp");
    sampler=glGetUniformLocation(program,"uTex");
    if(position<0||uv<0||matrix<0||sampler<0){
        ReleaseGl();restore();return false;
    }
    glActiveTexture(GL_TEXTURE0);
    for(auto &m:meshes){
        glGenBuffers(1,&m.vbo);
        glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(m.xyzuv.size()*sizeof(float)),
            m.xyzuv.data(),GL_STATIC_DRAW);
        glGenTextures(1,&m.texture);
        glBindTexture(GL_TEXTURE_2D,m.texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,m.width,m.height,0,
            GL_RGBA,GL_UNSIGNED_BYTE,m.rgba.data());
        if(m.vbo==0||m.texture==0||glGetError()!=GL_NO_ERROR){
            __android_log_print(ANDROID_LOG_ERROR,"TestChamberAssets",
                "GPU upload failed for owner model %d",m.model);
            ReleaseGl();restore();return false;
        }
    }
    restore();
    __android_log_print(ANDROID_LOG_INFO,"TestChamberAssets",
        "Uploaded %zu textured GLB submeshes covering all five models",
        meshes.size());
    return true;
}
void TestShowcase::ReleaseGl(){
    for(auto &m:meshes){
        if(m.texture)glDeleteTextures(1,&m.texture);
        if(m.vbo)glDeleteBuffers(1,&m.vbo);
        m.texture=0;m.vbo=0;
    }
    if(program)glDeleteProgram(program);
    program=0;
}
bool TestShowcase::Draw(const ReverieNativeEyeV1 *eye)const{
    if(!program||!eye)return false;
    const float locations[5][4]={
        {-2.25f,-1.50f,-1.92f,1.00f}, // crate
        {-1.17f,-1.50f,-2.08f,1.00f}, // chair
        { 1.35f,-1.50f,-2.42f,1.00f}, // shelf
        { 0.00f,-1.50f,-2.05f,0.82f}, // dinner table
        { 2.65f,-1.50f,-2.90f,0.86f}  // full-size door
    };
    ReverieNativeGlStateV1 state={};
    ReverieNativeGlAttribStateV1 attr0={},attr1={};
    ReverieNativeGlStateCaptureV1(&state);
    ReverieNativeGlAttribCaptureV1(&attr0,static_cast<GLuint>(position));
    ReverieNativeGlAttribCaptureV1(&attr1,static_cast<GLuint>(uv));
    float vp[16];
    ReverieNativeMat4Multiply(vp,eye->projection,eye->view);
    glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);
    glUseProgram(program);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(sampler,0);
    glEnableVertexAttribArray(static_cast<GLuint>(position));
    glEnableVertexAttribArray(static_cast<GLuint>(uv));
    for(const auto &m:meshes){
        if(!m.vbo||!m.texture)continue;
        const float *p=locations[m.model];
        const float s=p[3];
        float model[16]={
            s,0,0,0, 0,s,0,0, 0,0,s,0, p[0],p[1],p[2],1
        };
        float mvp[16];
        ReverieNativeMat4Multiply(mvp,vp,model);
        glUniformMatrix4fv(matrix,1,GL_FALSE,mvp);
        glBindTexture(GL_TEXTURE_2D,m.texture);
        glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
        glVertexAttribPointer(static_cast<GLuint>(position),3,
            GL_FLOAT,GL_FALSE,5*sizeof(float),
            reinterpret_cast<const void *>(0));
        glVertexAttribPointer(static_cast<GLuint>(uv),2,
            GL_FLOAT,GL_FALSE,5*sizeof(float),
            reinterpret_cast<const void *>(3*sizeof(float)));
        glDrawArrays(GL_TRIANGLES,0,m.vertex_count);
    }
    ReverieNativeGlAttribRestoreV1(&attr1);
    ReverieNativeGlAttribRestoreV1(&attr0);
    ReverieNativeGlStateRestoreV1(&state);
    return glGetError()==GL_NO_ERROR;
}
