#ifndef REVERIE_SITKA_TERRAIN_GL_H
#define REVERIE_SITKA_TERRAIN_GL_H

// Game-private GLES2 terrain proof: immutable indexed VBO/EBO per context,
// shared buffers for both eyes, no uint32 index extension or per-eye upload.
#include "sitka_terrain_mesh.h"
#include "sitka_lighting.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include <GLES2/gl2.h>
#include <cstddef>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace reverie { namespace sitka {
class TerrainGl {
public:
    TerrainGl() = default;
    TerrainGl(const TerrainGl&) = delete;
    TerrainGl& operator=(const TerrainGl&) = delete;
    bool ready() const { return program_ != 0u && (overlay_mode_ || !batches_.empty()); }
    size_t batch_count() const { return batches_.size(); }
    // Number of successful static VBO/EBO batch uploads in this GL context.
    size_t upload_count() const { return upload_count_; }

    // Must run on the active shell GL context. Release before context loss.
    bool Create(const TerrainMesh& mesh, const Lighting& lighting,
                std::string* error = nullptr) {
        return CreateWithExtras(mesh,{},lighting,error);
    }
    // A dedicated shore layer has no permanent base mesh. Keep the GLES2
    // program alive while its dynamic batches are swapped between eye pairs.
    bool CreateOverlay(const Lighting& lighting,std::string* error=nullptr){
        return CreateWithExtras(TerrainMesh{},{},lighting,error,true);
    }
    // Optional vetted seafloor batches share the existing terrain program,
    // ambient shader and indexed draw path; no duplicate per-eye resources.
    bool CreateWithExtras(const TerrainMesh& mesh,
                          const std::vector<TerrainMesh>& seafloor,
                          const Lighting& lighting,
                          std::string* error = nullptr,
                          bool allow_empty_base = false) {
        if(!std::isfinite(lighting.ambient_scale) ||
           lighting.ambient_scale < .5f || lighting.ambient_scale > 1.5f) {
            if(error)*error = "Invalid authored Sitka ambient intensity";
            return false;
        }
        Release();
        ReverieNativeGlStateV1 state = {};
        ReverieNativeGlStateCaptureV1(&state);
        GLint old_elements = 0;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &old_elements);
        const auto restore = [&]() {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(old_elements));
            ReverieNativeGlStateRestoreV1(&state);
        };
        static const char *vs =
            "uniform mat4 u_ViewProjection;\n"
            "attribute vec3 a_Position;\n"
            "attribute vec3 a_Color;\n"
            "varying vec3 v_Color;\n"
            "void main(){ v_Color=a_Color; gl_Position=u_ViewProjection*vec4(a_Position,1.0); }\n";
        static const char *fs =
            "precision mediump float;\n"
            "varying vec3 v_Color;\n"
            "uniform float u_AmbientScale;\n"
            "void main(){ gl_FragColor=vec4(min(v_Color*u_AmbientScale,vec3(1.0)),1.0); }\n";
        const ReverieNativeGlAttributeBindingV1 attributes[] = {
            {0u,"a_Position"}, {1u,"a_Color"}
        };
        char diagnostic[256] = {};
        program_ = ReverieNativeBuildProgramDiagnosticV1(
            vs, fs, attributes, 2u, diagnostic, sizeof(diagnostic));
        if (!program_) {
            if (error) *error = diagnostic;
            restore();
            return false;
        }
        matrix_ = glGetUniformLocation(program_, "u_ViewProjection");
        ambient_ = glGetUniformLocation(program_, "u_AmbientScale");
        if (matrix_ < 0 || ambient_ < 0) {
            if (error) *error = "Sitka terrain matrix uniform unavailable";
            Release(); restore(); return false;
        }
        try {
            auto packed = allow_empty_base && mesh.indices.empty()
                ? std::vector<TerrainBatch>{} : PackTerrainGles2(mesh);
            land_batches_=packed.size();
            for(const auto& floor : seafloor){
                if(floor.indices.empty())continue;
                auto extra=PackTerrainGles2(floor);
                packed.insert(packed.end(),
                    std::make_move_iterator(extra.begin()),
                    std::make_move_iterator(extra.end()));
            }
            for (const auto& batch : packed) {
                GpuBatch gpu;
                glGenBuffers(1, &gpu.vertex);
                glGenBuffers(1, &gpu.element);
                if (!gpu.vertex || !gpu.element) {
                    if (gpu.vertex) glDeleteBuffers(1, &gpu.vertex);
                    if (gpu.element) glDeleteBuffers(1, &gpu.element);
                    if (error) *error = "Sitka terrain GPU buffer allocation failed";
                    Release(); restore(); return false;
                }
                // Own the handles before an upload can fail.
                gpu.count = static_cast<GLsizei>(batch.indices.size());
                batches_.push_back(gpu);
                glBindBuffer(GL_ARRAY_BUFFER, gpu.vertex);
                glBufferData(GL_ARRAY_BUFFER,
                    static_cast<GLsizeiptr>(batch.vertices.size()*sizeof(TerrainBatch::Vertex)),
                    batch.vertices.data(), GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.element);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    static_cast<GLsizeiptr>(batch.indices.size()*sizeof(uint16_t)),
                    batch.indices.data(), GL_STATIC_DRAW);
                if (glGetError() != GL_NO_ERROR) {
                    if (error) *error = "Sitka terrain static GPU upload failed";
                    Release(); restore(); return false;
                }
                ++upload_count_;
            }
        } catch (const std::exception& e) {
            if (error) *error = e.what();
            Release(); restore(); return false;
        }
        ambient_scale_ = lighting.ambient_scale;
        overlay_mode_=allow_empty_base;
        restore();
        return ready();
    }
    // GL-owned transition operation: run between completed stereo frames
    // on the render context thread, never from render_eye. Old land buffers
    // are preserved; seabed replacement is fully staged before commit.
    bool ReplaceSeafloor(const std::vector<TerrainMesh>& extras,
                         std::string* error=nullptr){
        if(!ready()||(!overlay_mode_&&land_batches_==0u)||
           land_batches_>batches_.size()){
            if(error)*error="Sitka GL context unavailable for seabed swap";
            return false;
        }
        std::vector<TerrainBatch> packed;
        try{
            for(const auto& tile:extras){
                if(tile.indices.empty())continue;
                auto batches=PackTerrainGles2(tile);
                packed.insert(packed.end(),
                    std::make_move_iterator(batches.begin()),
                    std::make_move_iterator(batches.end()));
            }
            batches_.reserve(land_batches_+packed.size());
        }catch(const std::exception& e){
            if(error)*error=e.what();
            return false;
        }
        ReverieNativeGlStateV1 state{};
        ReverieNativeGlStateCaptureV1(&state);
        GLint old_element=0;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&old_element);
        const auto restore=[&](){
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLuint>(old_element));
            ReverieNativeGlStateRestoreV1(&state);
        };
        std::vector<GpuBatch> prepared;
        try{
            prepared.reserve(packed.size());
            for(const auto& data:packed){
                GpuBatch gpu{};
                glGenBuffers(1,&gpu.vertex);
                glGenBuffers(1,&gpu.element);
                if(!gpu.vertex||!gpu.element){
                    if(gpu.vertex)glDeleteBuffers(1,&gpu.vertex);
                    if(gpu.element)glDeleteBuffers(1,&gpu.element);
                    throw std::runtime_error("Sitka streamed GPU allocation");
                }
                gpu.count=static_cast<GLsizei>(data.indices.size());
                prepared.push_back(gpu); // Own handles prior to any upload.
                glBindBuffer(GL_ARRAY_BUFFER,gpu.vertex);
                glBufferData(GL_ARRAY_BUFFER,
                    static_cast<GLsizeiptr>(data.vertices.size()*
                                          sizeof(TerrainBatch::Vertex)),
                    data.vertices.data(),GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,gpu.element);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    static_cast<GLsizeiptr>(data.indices.size()*sizeof(uint16_t)),
                    data.indices.data(),GL_STATIC_DRAW);
                if(glGetError()!=GL_NO_ERROR)
                    throw std::runtime_error("Sitka streamed GPU upload");
            }
        }catch(const std::exception& e){
            for(const auto& gpu:prepared){
                glDeleteBuffers(1,&gpu.vertex);
                glDeleteBuffers(1,&gpu.element);
            }
            if(error)*error=e.what();
            restore();
            return false;
        }
        for(size_t i=land_batches_;i<batches_.size();++i){
            if(batches_[i].vertex)glDeleteBuffers(1,&batches_[i].vertex);
            if(batches_[i].element)glDeleteBuffers(1,&batches_[i].element);
        }
        batches_.resize(land_batches_);
        for(const auto& gpu:prepared)batches_.push_back(gpu);
        upload_count_+=prepared.size();
        ++seabed_swaps_;
        restore();
        return true;
    }
    size_t land_batch_count()const{return land_batches_;}
    size_t seabed_swap_count()const{return seabed_swaps_;}

    // Optional boat-local transform; ordinary terrain remains untransformed.
    bool RenderEye(const ReverieNativeEyeV1* eye,const float* model=nullptr) const {
        if(model)for(int k=0;k<16;++k)
            if(!std::isfinite(model[k]))return false;
        if (!ready() || !ReverieNativeEyeRenderableV1(eye)) return false;
        ReverieNativeGlStateV1 state = {};
        ReverieNativeGlAttribStateV1 position = {}, color = {};
        ReverieNativeGlStateCaptureV1(&state);
        ReverieNativeGlAttribCaptureV1(&position, 0u);
        ReverieNativeGlAttribCaptureV1(&color, 1u);
        GLint old_elements = 0;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &old_elements);
        float vp[16];
        ReverieNativeMat4Multiply(vp, eye->projection, eye->view);
        float mvp[16];
        if(model)ReverieNativeMat4Multiply(mvp,vp,model);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE); // Exporter land triangles have no winding guarantee yet.
        glUseProgram(program_);
        glUniformMatrix4fv(matrix_, 1, GL_FALSE, model?mvp:vp);
        glUniform1f(ambient_, ambient_scale_);
        glEnableVertexAttribArray(0u);
        glEnableVertexAttribArray(1u);
        for (const auto& gpu : batches_) {
            glBindBuffer(GL_ARRAY_BUFFER, gpu.vertex);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.element);
            glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE,
                sizeof(TerrainBatch::Vertex), reinterpret_cast<const void*>(0));
            glVertexAttribPointer(1u, 3, GL_UNSIGNED_BYTE, GL_TRUE,
                sizeof(TerrainBatch::Vertex), reinterpret_cast<const void*>(12));
            glDrawElements(GL_TRIANGLES, gpu.count, GL_UNSIGNED_SHORT, nullptr);
        }
        ReverieNativeGlAttribRestoreV1(&color);
        ReverieNativeGlAttribRestoreV1(&position);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(old_elements));
        ReverieNativeGlStateRestoreV1(&state);
        return glGetError() == GL_NO_ERROR;
    }
    void Release() {
        for (const auto& batch : batches_) {
            if (batch.vertex) glDeleteBuffers(1, &batch.vertex);
            if (batch.element) glDeleteBuffers(1, &batch.element);
        }
        batches_.clear();
        upload_count_ = 0u;
        land_batches_=seabed_swaps_=0u;
        overlay_mode_=false;
        if (program_) glDeleteProgram(program_);
        program_ = 0u;
        matrix_ = -1;
        ambient_ = -1;
        ambient_scale_ = 1.0f;
    }
private:
    struct GpuBatch { GLuint vertex = 0u, element = 0u; GLsizei count = 0; };
    GLuint program_ = 0u;
    GLint matrix_ = -1;
    GLint ambient_ = -1;
    float ambient_scale_ = 1.0f;
    std::vector<GpuBatch> batches_;
    size_t upload_count_ = 0u;
    size_t land_batches_ = 0u,seabed_swaps_ = 0u;
    bool overlay_mode_=false;
};
}} // namespace reverie::sitka
#endif
