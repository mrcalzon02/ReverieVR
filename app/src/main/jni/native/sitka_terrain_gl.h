#ifndef REVERIE_SITKA_TERRAIN_GL_H
#define REVERIE_SITKA_TERRAIN_GL_H

// Game-private GLES2 terrain proof: immutable indexed VBO/EBO per context,
// shared buffers for both eyes, no uint32 index extension or per-eye upload.
#include "sitka_terrain_mesh.h"
#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include <GLES2/gl2.h>
#include <cstddef>
#include <string>
#include <vector>

namespace reverie { namespace sitka {
class TerrainGl {
public:
    TerrainGl() = default;
    TerrainGl(const TerrainGl&) = delete;
    TerrainGl& operator=(const TerrainGl&) = delete;
    bool ready() const { return program_ != 0u && !batches_.empty(); }
    size_t batch_count() const { return batches_.size(); }
    // Number of successful static VBO/EBO batch uploads in this GL context.
    size_t upload_count() const { return upload_count_; }

    // Must run on the active shell GL context. Release before context loss.
    bool Create(const TerrainMesh& mesh, std::string* error = nullptr) {
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
            "void main(){ gl_FragColor=vec4(v_Color,1.0); }\n";
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
        if (matrix_ < 0) {
            if (error) *error = "Sitka terrain matrix uniform unavailable";
            Release(); restore(); return false;
        }
        try {
            auto packed = PackTerrainGles2(mesh);
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
        restore();
        return ready();
    }
    bool RenderEye(const ReverieNativeEyeV1* eye) const {
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
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE); // Exporter land triangles have no winding guarantee yet.
        glUseProgram(program_);
        glUniformMatrix4fv(matrix_, 1, GL_FALSE, vp);
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
        if (program_) glDeleteProgram(program_);
        program_ = 0u;
        matrix_ = -1;
    }
private:
    struct GpuBatch { GLuint vertex = 0u, element = 0u; GLsizei count = 0; };
    GLuint program_ = 0u;
    GLint matrix_ = -1;
    std::vector<GpuBatch> batches_;
    size_t upload_count_ = 0u;
};
}} // namespace reverie::sitka
#endif
