#ifndef REVERIE_SITKA_WATER_GL_H
#define REVERIE_SITKA_WATER_GL_H
// Game-private source-only GLES2 authored water, not a Sitka geographic mask.
#include "sitka_environment.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include <GLES2/gl2.h>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
namespace reverie { namespace sitka {
inline std::vector<float> WaterSurfaceVertices(const Water& w) {
    if (!std::isfinite(w.x)||!std::isfinite(w.y)||!std::isfinite(w.z)||
        !std::isfinite(w.yaw)||w.width<.1f||w.width>2000.f||
        w.depth<.1f||w.depth>2000.f||std::abs(w.yaw)>kPi)
        throw std::invalid_argument("invalid water footprint");
    std::vector<float> v; v.reserve(96u);
    const float c=std::cos(w.yaw),s=std::sin(w.yaw);
    const auto add=[&](float a,float b){
        v.push_back(w.x+a*c-b*s);v.push_back(w.y);v.push_back(w.z+a*s+b*c);
    };
    const float hx=w.width*.5f,hz=w.depth*.5f;
    if(w.shape=="SQUARE"){
        add(-hx,-hz);add(hx,-hz);add(hx,hz);add(-hx,hz);
    }else if(w.shape=="OCTAGON"){
        const float t=std::min(hx,hz)*.5f;
        add(-hx+t,-hz);add(hx-t,-hz);add(hx,-hz+t);add(hx,hz-t);
        add(hx-t,hz);add(-hx+t,hz);add(-hx,hz-t);add(-hx,-hz+t);
    }else if(w.shape=="CIRCLE"||w.shape=="OBLONG"||w.shape=="PILL"){
        const float radius=std::min(hx,hz);
        for(int i=0;i<32;++i){
            const float a=2.f*kPi*float(i)/32.f,ca=std::cos(a),sa=std::sin(a);
            if(w.shape=="CIRCLE")add(hx*ca,hx*sa);
            else if(w.shape=="OBLONG")add(hx*ca,hz*sa);
            else if(hx>=hz)add((ca>=0.f?1.f:-1.f)*(hx-radius)+radius*ca,radius*sa);
            else add(radius*ca,(sa>=0.f?1.f:-1.f)*(hz-radius)+radius*sa);
        }
    }else throw std::invalid_argument("unsupported water footprint");
    return v;
}
inline float WaterWindGain(const Weather& w){
    return .75f+std::min(std::max(w.wind,0.f),20.f)*.025f;
}
inline float WeatherWaveHeight(const Water& w,const Weather& weather,
                               float east,float north,float seconds){
    return w.y+(WaveHeight(w,east,north,seconds)-w.y)*WaterWindGain(weather);
}
inline std::array<float,3> WaterTint(const Weather& w){
    const float day=std::clamp(.35f+.65f*std::sin(kPi*(w.hour-6.f)/12.f),.25f,1.f);
    const float dim=day*(1.f-.28f*w.fog-.18f*w.rain);
    return {{.07f*dim,.38f*dim,.66f*dim}};
}
class WaterGl {
public:
    WaterGl()=default;
    WaterGl(const WaterGl&)=delete;
    WaterGl& operator=(const WaterGl&)=delete;
    bool ready()const{return program_!=0u&&!bodies_.empty();}
    size_t upload_count()const{return uploads_;}
    float time_seconds()const{return seconds_;}
    void Update(float dt){
        if(std::isfinite(dt)&&dt>0.f)
            seconds_=std::fmod(seconds_+std::min(dt,.1f),4096.f);
    }
    bool Create(const std::vector<Water>& waters,const Weather& weather,
                std::string* error=nullptr){
        Release();
        if(waters.empty()||waters.size()>64u){
            if(error)*error="Sitka water requires 1..64 authored footprints";
            return false;
        }
        ReverieNativeGlStateV1 state={};
        ReverieNativeGlStateCaptureV1(&state);
        const auto restore=[&](){ReverieNativeGlStateRestoreV1(&state);};
        static const char* vs=
            "uniform mat4 u_ViewProjection;\n"
            "uniform vec4 u_Wave;\n"
            "uniform vec2 u_Center;\n"
            "uniform float u_Seconds;\n"
            "attribute vec3 a_Position;\n"
            "void main(){\n"
            "float u=(a_Position.x-u_Center.x)/u_Wave.y;\n"
            "float v=(a_Position.z-u_Center.y)/u_Wave.y;\n"
            "float slow=sin(u*.39+v*.52+u_Seconds*u_Wave.z*.74);\n"
            "float chop=sin(u*1.14-v*.95-u_Seconds*(u_Wave.z+u_Wave.w)*1.81);\n"
            "vec3 p=vec3(a_Position.x,a_Position.y+u_Wave.x*(.68*slow+.32*chop),a_Position.z);\n"
            "gl_Position=u_ViewProjection*vec4(p,1.0);\n"
            "}\n";
        static const char* fs=
            "precision mediump float;\n"
            "uniform vec3 u_Tint;\n"
            "void main(){gl_FragColor=vec4(u_Tint,1.0);}\n";
        const ReverieNativeGlAttributeBindingV1 binding[]={{0u,"a_Position"}};
        char diagnostic[256]={};
        program_=ReverieNativeBuildProgramDiagnosticV1(vs,fs,binding,1u,
                                                       diagnostic,sizeof(diagnostic));
        if(!program_){if(error)*error=diagnostic;restore();return false;}
        matrix_=glGetUniformLocation(program_,"u_ViewProjection");
        wave_=glGetUniformLocation(program_,"u_Wave");
        center_=glGetUniformLocation(program_,"u_Center");
        time_=glGetUniformLocation(program_,"u_Seconds");
        tint_=glGetUniformLocation(program_,"u_Tint");
        if(matrix_<0||wave_<0||center_<0||time_<0||tint_<0){
            if(error)*error="Sitka water shader uniform unavailable";
            Release();restore();return false;
        }
        try{
            for(const Water& w:waters){
                const std::vector<float> v=WaterSurfaceVertices(w);
                if(v.size()<9u||v.size()>96u)
                    throw std::invalid_argument("water footprint vertex count invalid");
                GpuBody body;body.water=w;
                body.count=static_cast<GLsizei>(v.size()/3u);
                glGenBuffers(1,&body.buffer);
                if(!body.buffer)throw std::runtime_error("water VBO allocation failed");
                bodies_.push_back(body);
                glBindBuffer(GL_ARRAY_BUFFER,body.buffer);
                glBufferData(GL_ARRAY_BUFFER,
                    static_cast<GLsizeiptr>(v.size()*sizeof(float)),
                    v.data(),GL_STATIC_DRAW);
                if(glGetError()!=GL_NO_ERROR)
                    throw std::runtime_error("water VBO upload failed");
                ++uploads_;
            }
        }catch(const std::exception& e){
            if(error)*error=e.what();
            Release();restore();return false;
        }
        weather_=weather;seconds_=0.f;restore();return ready();
    }
    bool RenderEye(const ReverieNativeEyeV1* eye)const{
        if(!ready()||!ReverieNativeEyeRenderableV1(eye))return false;
        ReverieNativeGlStateV1 state={};
        ReverieNativeGlAttribStateV1 attr={};
        ReverieNativeGlStateCaptureV1(&state);
        ReverieNativeGlAttribCaptureV1(&attr,0u);
        float vp[16];ReverieNativeMat4Multiply(vp,eye->projection,eye->view);
        const auto tint=WaterTint(weather_);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND); // Provisional opaque water, no false optical claims.
        glDisable(GL_CULL_FACE);
        glUseProgram(program_);
        glUniformMatrix4fv(matrix_,1,GL_FALSE,vp);
        glUniform1f(time_,seconds_);
        glUniform3f(tint_,tint[0],tint[1],tint[2]);
        glEnableVertexAttribArray(0u);
        for(const GpuBody& body:bodies_){
            const Water& w=body.water;
            glUniform4f(wave_,w.height*WaterWindGain(weather_),
                        w.grid,w.speed,w.scrollB);
            glUniform2f(center_,w.x,w.z);
            glBindBuffer(GL_ARRAY_BUFFER,body.buffer);
            glVertexAttribPointer(0u,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr);
            glDrawArrays(GL_TRIANGLE_FAN,0,body.count);
        }
        ReverieNativeGlAttribRestoreV1(&attr);
        ReverieNativeGlStateRestoreV1(&state);
        return glGetError()==GL_NO_ERROR;
    }
    void Release(){
        for(const GpuBody& b:bodies_)if(b.buffer)glDeleteBuffers(1,&b.buffer);
        bodies_.clear();uploads_=0u;
        if(program_)glDeleteProgram(program_);
        program_=0u;matrix_=wave_=center_=time_=tint_=-1;seconds_=0.f;
    }
private:
    struct GpuBody{Water water;GLuint buffer=0u;GLsizei count=0;};
    GLuint program_=0u;
    GLint matrix_=-1,wave_=-1,center_=-1,time_=-1,tint_=-1;
    std::vector<GpuBody> bodies_;
    Weather weather_;
    size_t uploads_=0u;
    float seconds_=0.f;
};
}} // namespace reverie::sitka
#endif
