#include "reverie_native_sdk.h"
#include "reverie_scene_document.h"
#include "reverie_vr_workshop.h"
#include "reverie_native_lato_text.h"
#include "reverie_native_avatar_geometry.h"
#include "reverie_native_avatar_wardrobe.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include "gentexture.hpp"

#include <GLES2/gl2.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <new>
#include <string>
#include <vector>

namespace {

constexpr int kTextureSize = 256;

struct ModuleState {
    const ReverieNativeHostV1 *host = nullptr;

    GLuint program = 0;
    GLuint vbo = 0;
    GLuint texture = 0;
    ReverieNativeAvatarV1 avatar = {};
    GLsizei avatar_vertex_count = 0;
    GLsizei wardrobe_vertex_count = 0;
    uint32_t wardrobe_visible = 0u;

    GLint position_location = -1;
    GLint uv_location = -1;
    GLint matrix_location = -1;
    GLint tint_location = -1;
    GLint texture_location = -1;
    GLint offset_location = -1;
    GLint pivot_location = -1;
    GLint spin_location = -1;
    GLint light_location = -1;
    GLint ambient_location = -1;
    GLint emissive_location = -1;

    bool primary_was_down = false;
    bool alternate_tint = false;
    int demo_mode=0;
    float clock=0.f;
    reverie::nativetext::LatoLayer font;
    std::vector<reverie::authoring::Piece> pieces;
    std::vector<GLint> piece_first;
    std::vector<GLsizei> piece_count;
    reverie::authoring::VrWorkshop workshop;
    std::vector<float> room_avatar_prefix;
    bool workshop_hud_dirty = true;

    std::vector<uint8_t> rgba;
    // Optional editor-authored primitives, identical across eye renders.
    std::vector<float> editor_mesh;
    GLsizei editor_vertex_count = 0;
};


int ReadWorkshop(void* user,size_t slot,std::string& output){
    auto* state=static_cast<ModuleState*>(user);
    if(slot>=reverie::authoring::kVrProjectSlots)return -1;
    const std::string filename="vr-workshop-project-"+std::to_string(slot)+".rvscene";
    uint32_t size=0u;
    const int32_t probe=state->host->read_save(filename.c_str(),nullptr,0u,&size);
    if(probe==REVERIE_NATIVE_SAVE_NOT_FOUND)return 0;
    if(probe!=REVERIE_NATIVE_SAVE_OK||size==0u||
       size>reverie::authoring::kVrSlotMaxBytes)return -1;
    std::vector<char> bytes(size);
    uint32_t actual=0u;
    if(state->host->read_save(filename.c_str(),bytes.data(),size,&actual)
       !=REVERIE_NATIVE_SAVE_OK||actual!=size)return -1;
    output.assign(bytes.begin(),bytes.end());
    return 1;
}
bool WriteWorkshop(void* user,size_t slot,const std::string& data){
    auto* state=static_cast<ModuleState*>(user);
    if(slot>=reverie::authoring::kVrProjectSlots||
       data.size()>reverie::authoring::kVrSlotMaxBytes)return false;
    const std::string filename="vr-workshop-project-"+std::to_string(slot)+".rvscene";
    return state->host->write_save(filename.c_str(),data.data(),
        static_cast<uint32_t>(data.size()))==REVERIE_NATIVE_SAVE_OK;
}
reverie::authoring::VrStore WorkshopStore(ModuleState* state){
    return {state,ReadWorkshop,WriteWorkshop};
}
void BakeWorkshopGeometry(ModuleState* state){
    state->pieces=state->workshop.scene.pieces;
    state->editor_mesh.clear();
    state->piece_first.clear();
    state->piece_count.clear();
    for(const auto& piece:state->pieces){
        reverie::authoring::Scene one{state->workshop.scene.environment,{piece}};
        const std::vector<float> mesh=reverie::authoring::PrimitiveTriangles(one);
        state->piece_first.push_back(static_cast<GLint>(state->editor_mesh.size()/5u));
        state->piece_count.push_back(static_cast<GLsizei>(mesh.size()/5u));
        state->editor_mesh.insert(state->editor_mesh.end(),mesh.begin(),mesh.end());
    }
    state->editor_vertex_count=static_cast<GLsizei>(state->editor_mesh.size()/5u);
    state->workshop.mesh_dirty=false;
}
void UpdateWorkshopHud(ModuleState* state){
    state->font.Clear();
    if(!state->workshop.editing){
        state->font.Text("PROCEDURAL TEST CHAMBER",-.98f,1.30f,.045f,
            .90f,.97f,.97f,-2.62f);
        state->font.Text("PRIMARY: NEXT LIGHT / AUDIO",-.97f,-1.24f,.033f,
            .82f,.94f,.84f,-2.63f);
        state->font.Text("SECONDARY: VR WORKSHOP",-.97f,-1.45f,.033f,
            .94f,.85f,.53f,-2.63f);
        return;
    }
    static const char* actions[10]={
        "ADD BOX","ADD CYLINDER","MOVE X/Z","ROTATE","SCALE",
        "DELETE","UNDO","SAVE","PLAY TEST","NEXT PROJECT"};
    state->font.Text("VR WORKSHOP", -3.2f,1.43f,.052f,.93f,.96f,.99f,-2.62f);
    for(int i=0;i<10;++i) {
        const bool active=(i==2&&state->workshop.tool==reverie::authoring::VrTool::Move)
            ||(i==3&&state->workshop.tool==reverie::authoring::VrTool::Rotate)
            ||(i==4&&state->workshop.tool==reverie::authoring::VrTool::Scale);
        state->font.Text(actions[i],-3.16f,1.09f-i*.235f,.038f,
            active?1.f:.77f,active?.79f:.9f,active?.37f:1.f,-2.62f);
    }
    const std::string project="PROJECT "+std::to_string(state->workshop.slot+1)+
        " / "+std::to_string(reverie::authoring::kVrProjectSlots);
    state->font.Text(project.c_str(),-.69f,1.32f,.039f,.82f,.94f,.99f,-2.62f);
    const std::string object=state->workshop.scene.pieces.empty()?
        "NO OBJECT SELECTED" : ("SELECTED: "+
        state->workshop.scene.pieces[state->workshop.selected].id);
    state->font.Text(object.c_str(),-.69f,1.08f,.035f,.92f,.89f,.64f,-2.62f);
    const std::string count="OBJECTS "+std::to_string(state->workshop.scene.pieces.size())+
        " / "+std::to_string(reverie::authoring::kVrEditMaxPieces);
    state->font.Text(count.c_str(),-.69f,.85f,.034f,.8f,.93f,.89f,-2.62f);
    state->font.Text("POINT AND CLICK TOOL",-.69f,.53f,.029f,.85f,.94f,.98f,-2.62f);
    state->font.Text("DRAG OBJECT TO ADJUST",-.69f,.32f,.029f,.85f,.94f,.98f,-2.62f);
    state->font.Text("SECONDARY: SAVE + TEST",-.69f,-.88f,.028f,.95f,.88f,.53f,-2.62f);
    if(state->workshop.dirty)
        state->font.Text("UNSAVED EDITS",-.69f,-1.09f,.034f,1.f,.71f,.36f,-2.62f);
    if(!state->workshop.last_error.empty())
        state->font.Fit(state->workshop.last_error.c_str(),-.69f,-1.31f,3.8f,
            .033f,1.f,.57f,.45f,-2.62f);
}

void Log(
    const ModuleState *state,
    int32_t level,
    const char *message
) {
    if (state == nullptr) {
        return;
    }

    ReverieNativeLogV1(
        state->host,
        level,
        "ReverieTestChamber",
        message
    );
}

GLuint BuildProgram() {
    static const char *kVertexShader =
        "uniform mat4 u_ViewProjection;\n"
        "uniform vec3 u_Offset; uniform vec3 u_Pivot;\n"
        "uniform vec3 u_LightPosition; uniform float u_Spin;\n"
        "uniform float u_Ambient;\n"
        "attribute vec3 a_Position; attribute vec2 a_Uv;\n"
        "varying vec2 v_Uv; varying float v_Light;\n"
        "void main(){\n"
        "  float c=cos(u_Spin); float s=sin(u_Spin);\n"
        "  vec3 d=a_Position-u_Pivot;\n"
        "  vec3 p=u_Pivot+vec3(c*d.x-s*d.z,d.y,s*d.x+c*d.z)+u_Offset;\n"
        "  float dist=distance(p,u_LightPosition);\n"
        "  v_Light=clamp(u_Ambient+0.85/(1.0+0.65*dist*dist),0.0,1.6);\n"
        "  gl_Position=u_ViewProjection*vec4(p,1.0);\n"
        "  v_Uv=a_Uv;\n"
        "}\n";

    static const char *kFragmentShader =
        "precision mediump float;\n"
        "uniform sampler2D u_Texture; uniform vec3 u_Tint;\n"
        "uniform float u_Emissive;\n"
        "varying vec2 v_Uv; varying float v_Light;\n"
        "void main(){\n"
        "  vec4 texel=texture2D(u_Texture,v_Uv);\n"
        "  gl_FragColor=vec4(texel.rgb*u_Tint*(v_Light+u_Emissive),1.0);\n"
        "}\n";

    static const ReverieNativeGlAttributeBindingV1 kBindings[] = {
        {0u, "a_Position"},
        {1u, "a_Uv"}
    };

    return ReverieNativeBuildProgram(
        kVertexShader,
        kFragmentShader,
        kBindings,
        sizeof(kBindings)
            / sizeof(kBindings[0])
    );
}

void GenerateProceduralTexture(
    ModuleState *state
) {
    InitTexgen();

    GenTexture gradient(kTextureSize, 1);
    for (int index = 0; index < kTextureSize; ++index) {
        float t =
            static_cast<float>(index)
            / static_cast<float>(
                kTextureSize - 1
            );

        uint8_t r =
            static_cast<uint8_t>(
                80.0f + t * 146.0f
            );
        uint8_t g =
            static_cast<uint8_t>(
                83.0f + t * 140.0f
            );
        uint8_t b =
            static_cast<uint8_t>(
                87.0f + t * 136.0f
            );

        gradient.Data[index].Init(
            r,
            g,
            b,
            255
        );
    }

    GenTexture generated(
        kTextureSize,
        kTextureSize
    );
    generated.Noise(
        gradient,
        4,
        4,
        5,
        0.57f,
        0x51A7,
        GenTexture::NoiseNormalize
            | GenTexture::NoiseBandlimit
    );

    state->rgba.resize(
        kTextureSize
            * kTextureSize
            * 4
    );

    for (int index = 0;
         index < generated.NPixels;
         ++index) {
        const Pixel &pixel =
            generated.Data[index];
        size_t offset =
            static_cast<size_t>(index)
            * 4u;

        state->rgba[offset + 0] =
            static_cast<uint8_t>(
                pixel.r >> 8
            );
        state->rgba[offset + 1] =
            static_cast<uint8_t>(
                pixel.g >> 8
            );
        state->rgba[offset + 2] =
            static_cast<uint8_t>(
                pixel.b >> 8
            );
        state->rgba[offset + 3] = 255u;
    }
}

static const float kRoomVertices[] = {
    // Floor
    -4.0f,-1.5f,-5.0f, 0.0f,0.0f,
     4.0f,-1.5f,-5.0f, 4.0f,0.0f,
     4.0f,-1.5f,4.5f, 4.0f,4.0f,
    -4.0f,-1.5f,-5.0f, 0.0f,0.0f,
     4.0f,-1.5f,4.5f, 4.0f,4.0f,
    -4.0f,-1.5f,4.5f, 0.0f,4.0f,

    // Ceiling
    -4.0f,2.25f,4.5f, 0.0f,0.0f,
     4.0f,2.25f,4.5f, 4.0f,0.0f,
     4.0f,2.25f,-5.0f, 4.0f,4.0f,
    -4.0f,2.25f,4.5f, 0.0f,0.0f,
     4.0f,2.25f,-5.0f, 4.0f,4.0f,
    -4.0f,2.25f,-5.0f, 0.0f,4.0f,

    // Back wall
    -4.0f,-1.5f,-5.0f, 0.0f,0.0f,
    -4.0f,2.25f,-5.0f, 0.0f,2.0f,
     4.0f,2.25f,-5.0f, 4.0f,2.0f,
    -4.0f,-1.5f,-5.0f, 0.0f,0.0f,
     4.0f,2.25f,-5.0f, 4.0f,2.0f,
     4.0f,-1.5f,-5.0f, 4.0f,0.0f,

    // Front wall
     4.0f,-1.5f,4.5f, 0.0f,0.0f,
     4.0f,2.25f,4.5f, 0.0f,2.0f,
    -4.0f,2.25f,4.5f, 4.0f,2.0f,
     4.0f,-1.5f,4.5f, 0.0f,0.0f,
    -4.0f,2.25f,4.5f, 4.0f,2.0f,
    -4.0f,-1.5f,4.5f, 4.0f,0.0f,

    // Left wall
    -4.0f,-1.5f,4.5f, 0.0f,0.0f,
    -4.0f,2.25f,4.5f, 0.0f,2.0f,
    -4.0f,2.25f,-5.0f, 4.0f,2.0f,
    -4.0f,-1.5f,4.5f, 0.0f,0.0f,
    -4.0f,2.25f,-5.0f, 4.0f,2.0f,
    -4.0f,-1.5f,-5.0f, 4.0f,0.0f,

    // Right wall
     4.0f,-1.5f,-5.0f, 0.0f,0.0f,
     4.0f,2.25f,-5.0f, 0.0f,2.0f,
     4.0f,2.25f,4.5f, 4.0f,2.0f,
     4.0f,-1.5f,-5.0f, 0.0f,0.0f,
     4.0f,2.25f,4.5f, 4.0f,2.0f,
     4.0f,-1.5f,4.5f, 4.0f,0.0f
};

void DestroyGl(ModuleState *state) {
    state->font.Release();
    if (state->texture != 0) {
        glDeleteTextures(
            1,
            &state->texture
        );
        state->texture = 0;
    }

    if (state->vbo != 0) {
        glDeleteBuffers(
            1,
            &state->vbo
        );
        state->vbo = 0;
    }

    if (state->program != 0) {
        glDeleteProgram(state->program);
        state->program = 0;
    }

    state->position_location = -1;
    state->uv_location = -1;
    state->matrix_location = -1;
    state->tint_location = -1;
    state->texture_location = -1;
    state->offset_location = -1;
    state->pivot_location = -1;
    state->spin_location = -1;
    state->light_location = -1;
    state->ambient_location = -1;
    state->emissive_location = -1;
}

void AbandonGl(ModuleState *state) {
    state->font.program=state->font.texture=state->font.vbo=0;
    state->font.upload=true;
    state->program = 0;
    state->vbo = 0;
    state->texture = 0;
    state->position_location = -1;
    state->uv_location = -1;
    state->matrix_location = -1;
    state->tint_location = -1;
    state->texture_location = -1;
}

int32_t InitializeGl(ModuleState *state) {
    ReverieNativeGlStateV1 init_gl_state = {};
    ReverieNativeGlStateCaptureV1(
        &init_gl_state
    );
    auto finish = [&init_gl_state](
        int32_t result
    ) -> int32_t {
        ReverieNativeGlStateRestoreV1(
            &init_gl_state
        );
        return result;
    };

    AbandonGl(state);

    state->program = BuildProgram();
    if (state->program == 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Shader program creation failed."
        );
        return finish(0);
    }

    state->position_location =
        glGetAttribLocation(
            state->program,
            "a_Position"
        );
    state->uv_location =
        glGetAttribLocation(
            state->program,
            "a_Uv"
        );
    state->matrix_location =
        glGetUniformLocation(
            state->program,
            "u_ViewProjection"
        );
    state->tint_location =
        glGetUniformLocation(
            state->program,
            "u_Tint"
        );
    state->texture_location =
        glGetUniformLocation(
            state->program,
            "u_Texture"
        );
    state->offset_location=glGetUniformLocation(state->program,"u_Offset");
    state->pivot_location=glGetUniformLocation(state->program,"u_Pivot");
    state->spin_location=glGetUniformLocation(state->program,"u_Spin");
    state->light_location=glGetUniformLocation(state->program,"u_LightPosition");
    state->ambient_location=glGetUniformLocation(state->program,"u_Ambient");
    state->emissive_location=glGetUniformLocation(state->program,"u_Emissive");

    if (state->position_location < 0
        || state->uv_location < 0
        || state->matrix_location < 0
        || state->tint_location < 0
        || state->texture_location < 0
        || state->offset_location < 0
        || state->pivot_location < 0
        || state->spin_location < 0
        || state->light_location < 0
        || state->ambient_location < 0
        || state->emissive_location < 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Required shader locations are unavailable."
        );
        DestroyGl(state);
        return finish(0);
    }

    glGenBuffers(1, &state->vbo);
    if (state->vbo == 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Room vertex buffer creation failed."
        );
        DestroyGl(state);
        return finish(0);
    }

    glBindBuffer(
        GL_ARRAY_BUFFER,
        state->vbo
    );
    // Same 15-field profile as the Home wardrobe, delivered by the host.
    // Assemble seven stretched spheres once per GL creation, never per eye.
    std::vector<float> figure(REVERIE_NATIVE_AVATAR_FIGURE_FLOATS_V1);
    const bool figure_ok=ReverieNativeWriteAvatarFigureV1(
        &state->avatar,2.45f,-1.5f,-3.85f,
        figure.data(),figure.size())!=0;
    std::vector<float> room_and_avatar(
        kRoomVertices,kRoomVertices+
            sizeof(kRoomVertices)/sizeof(kRoomVertices[0]));
    if(figure_ok) {
        room_and_avatar.reserve(room_and_avatar.size()
            +REVERIE_NATIVE_AVATAR_FIGURE_VERTICES_V1*5u);
        for(size_t i=0;i<figure.size();i+=3u) {
            room_and_avatar.push_back(figure[i]);
            room_and_avatar.push_back(figure[i+1u]);
            room_and_avatar.push_back(figure[i+2u]);
            room_and_avatar.push_back(.5f);
            room_and_avatar.push_back(.5f);
        }
    }
    state->avatar_vertex_count=figure_ok
        ?static_cast<GLsizei>(REVERIE_NATIVE_AVATAR_FIGURE_VERTICES_V1):0;
    // Same selected outfit, hair, hat, accessory and eye style as Home.
    // Prepared once per GL context, never in per-eye rendering.
    std::vector<float> wearables(REVERIE_NATIVE_WARDROBE_FLOATS_V1);
    uint32_t visible=0u;
    const bool wardrobe_ok=figure_ok && ReverieNativeWriteAvatarWardrobeV1(
        &state->avatar,2.45f,-1.5f,-3.85f,
        wearables.data(),wearables.size(),&visible)!=0;
    state->wardrobe_vertex_count=wardrobe_ok
        ?static_cast<GLsizei>(REVERIE_NATIVE_WARDROBE_VERTICES_V1):0;
    state->wardrobe_visible=wardrobe_ok?visible:0u;
    if(wardrobe_ok) {
        room_and_avatar.reserve(room_and_avatar.size()
            +REVERIE_NATIVE_WARDROBE_VERTICES_V1*5u);
        for(size_t i=0;i<wearables.size();i+=3u) {
            room_and_avatar.push_back(wearables[i]);
            room_and_avatar.push_back(wearables[i+1u]);
            room_and_avatar.push_back(wearables[i+2u]);
            room_and_avatar.push_back(.5f);
            room_and_avatar.push_back(.5f);
        }
    }
    // Authored geometry follows room/avatar/wardrobe in this single VBO.
    state->room_avatar_prefix=room_and_avatar;
    room_and_avatar.insert(room_and_avatar.end(),
        state->editor_mesh.begin(),state->editor_mesh.end());
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(room_and_avatar.size()*sizeof(float)),
        room_and_avatar.data(),GL_STATIC_DRAW);

    glGenTextures(1, &state->texture);
    if (state->texture == 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Procedural texture creation failed."
        );
        DestroyGl(state);
        return finish(0);
    }

    glBindTexture(
        GL_TEXTURE_2D,
        state->texture
    );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR
    );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        kTextureSize,
        kTextureSize,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        state->rgba.data()
    );

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "OpenGL ES initialization reported an error."
        );
        DestroyGl(state);
        return finish(0);
    }

    if(!state->font.Create()) {
        Log(state,REVERIE_NATIVE_LOG_ERROR,"Test chamber Lato font unavailable.");
        DestroyGl(state);
        return finish(0);
    }
    UpdateWorkshopHud(state);
    Log(state,REVERIE_NATIVE_LOG_INFO,
        "Test chamber showcase GL, real Lato glyphs and procedural materials ready.");
    return finish(1);
}

void *Create(
    const ReverieNativeHostV1 *host
) {
    if (!ReverieNativeHostSupportsLogV1(host) ||
        !ReverieNativeHostSupportsSaveV1(host)) {
        return nullptr;
    }

    ModuleState *state =
        new (std::nothrow) ModuleState();
    if (state == nullptr) {
        return nullptr;
    }

    state->host = host;
    state->avatar.struct_size=sizeof(ReverieNativeAvatarV1);
    state->avatar.schema_version=REVERIE_NATIVE_AVATAR_SCHEMA_V1;
    const uint32_t defaults[REVERIE_NATIVE_AVATAR_FIELD_COUNT_V1]={
        0u,0u,0u,0u,0u,2u,3u,3u,3u,3u,3u,3u,0u,2u,0u
    };
    std::memcpy(state->avatar.fields,defaults,sizeof(defaults));
    ReverieNativeAvatarV1 selected={};
    if(ReverieNativeReadAvatarV1(host,&selected))
        state->avatar=selected;

    reverie::authoring::Scene seed{"white_room",{}};
    // One exact allowlisted bundled asset. Unavailable keeps original room;
    // present invalid content fails loudly instead of rendering fake props.
    if(host->struct_size>=REVERIE_NATIVE_HOST_V1_ASSET_MIN_SIZE
        && host->read_asset!=nullptr) {
        uint32_t length=0;
        const char *path="native/test-chamber/scene.rvscene";
        const int32_t status=host->read_asset(path,nullptr,0u,&length);
        if(status==REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL &&
            length>0u && length<=reverie::authoring::kSceneMaxBytes) {
            std::vector<char> scene_bytes(length);
            uint32_t consumed=0;
            if(host->read_asset(path,scene_bytes.data(),length,&consumed)
                !=REVERIE_NATIVE_SAVE_OK || consumed!=length) {
                Log(state,REVERIE_NATIVE_LOG_ERROR,
                    "Desktop-authored scene asset read failed.");
                delete state;
                return nullptr;
            }
            try {
                const auto authored=reverie::authoring::ParseScene(
                    scene_bytes.data(),scene_bytes.size());
                seed=authored;
                Log(state,REVERIE_NATIVE_LOG_INFO,
                    "Verified editor-authored primitives loaded for both eyes.");
            } catch(const std::exception &error) {
                Log(state,REVERIE_NATIVE_LOG_ERROR,error.what());
                delete state;
                return nullptr;
            }
        } else if(status!=REVERIE_NATIVE_SAVE_NOT_FOUND) {
            Log(state,REVERIE_NATIVE_LOG_ERROR,
                "Desktop-authored scene missing valid bounded resource.");
            delete state;
            return nullptr;
        }
    }

    if(!state->workshop.Open(seed,WorkshopStore(state))){
        Log(state,REVERIE_NATIVE_LOG_ERROR,
            "VR workshop project save unreadable or exceeds safe limits.");
        delete state;return nullptr;
    }
    try {
        BakeWorkshopGeometry(state);
        GenerateProceduralTexture(state);
    } catch (...) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "OpenKTG texture generation threw an exception."
        );
        delete state;
        return nullptr;
    }

    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Module created; procedural texture recipe expanded on CPU."
    );
    return state;
}

void Destroy(void *instance) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    if (state == nullptr) {
        return;
    }

    if (state->program != 0
        || state->vbo != 0
        || state->texture != 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_WARN,
            "Module destroyed after GL context release was skipped."
        );
        AbandonGl(state);
    }

    if(state->workshop.dirty && !state->workshop.Save(WorkshopStore(state)))
        Log(state,REVERIE_NATIVE_LOG_ERROR,"Workshop save failed on exit.");
    Log(state,REVERIE_NATIVE_LOG_INFO,"Module destroyed.");
    delete state;
}

int32_t OnGlContextCreated(void *instance) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    if (state == nullptr) {
        return 0;
    }
    return InitializeGl(state);
}

void ReleaseGlContext(void *instance) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    if (state == nullptr) {
        return;
    }

    DestroyGl(state);
    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "GL resources released."
    );
}

void Resume(void *instance) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Module resumed."
    );
}

void Pause(void *instance) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    if(state&&state->workshop.dirty&&!state->workshop.Save(WorkshopStore(state)))
        Log(state,REVERIE_NATIVE_LOG_ERROR,"Workshop save failed on pause.");
    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Module paused."
    );
}

void Update(
    void *instance,
    const ReverieNativeInputV1 *input
) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    if (state == nullptr
        || !ReverieNativeInputHasBaseV1(
            input
        )) {
        return;
    }

    const float delta=std::max(0.f,std::min(input->delta_seconds,.05f));
    state->clock=std::fmod(state->clock+delta,3600.f);
    const bool primary=input->primary_down!=0u;
    const bool wasEditing=state->workshop.editing;
    reverie::authoring::VrRay ray;
    if(ReverieNativeInputHasPointerV1(input)&&
       input->pointer_kind!=REVERIE_NATIVE_POINTER_NONE){
        ray={input->pointer_origin[0],input->pointer_origin[1],
             input->pointer_origin[2],input->pointer_direction[0],
             input->pointer_direction[1],input->pointer_direction[2],true};
    }
    const auto event=state->workshop.Step(primary,input->secondary_down!=0u,
                                         ray,WorkshopStore(state));
    if(event!=reverie::authoring::VrEvent::None ||
       wasEditing!=state->workshop.editing)
        UpdateWorkshopHud(state);
    if(event==reverie::authoring::VrEvent::Rejected)
        ReverieNativeRequestFeedbackV1(state->host,REVERIE_NATIVE_FEEDBACK_FAILURE);
    else if(event!=reverie::authoring::VrEvent::None)
        ReverieNativeRequestFeedbackV1(state->host,REVERIE_NATIVE_FEEDBACK_ACTIVATION);
    if(wasEditing||state->workshop.editing){
        state->primary_was_down=primary;
        return; // Never advance the demo while editing or on a play transition.
    }
    if(primary && !state->primary_was_down) {
        state->demo_mode=(state->demo_mode+1)%6;
        state->alternate_tint=(state->demo_mode%2)!=0;
        static const uint32_t kCues[6]={
            REVERIE_NATIVE_SOUND_RED_LEDGER_CLICK,
            REVERIE_NATIVE_SOUND_RED_LEDGER_METAL,
            REVERIE_NATIVE_SOUND_RED_LEDGER_COINS,
            REVERIE_NATIVE_SOUND_RED_LEDGER_BOOK,
            REVERIE_NATIVE_SOUND_RED_LEDGER_DOOR,
            REVERIE_NATIVE_SOUND_RED_LEDGER_DRAWER
        };
        static const char* kNames[6]={
            "POINT LIGHT / UI CLICK",
            "DUSK LIGHT / METAL",
            "BRIGHT STUDIO / COINS",
            "PULSING BEACON / BOOK",
            "MOVING SPOT / DOOR",
            "WARM LIGHT / MECHANISM"
        };
        state->font.Clear();
        state->font.Fit("PROCEDURAL TEST CHAMBER",0.f,1.31f,5.15f,
            .044f,.90f,.97f,.97f,-2.62f);
        state->font.Fit(kNames[state->demo_mode],0.f,-1.12f,4.96f,
            .036f,1.f,.86f,.49f,-2.62f);
        state->font.Fit("PRESS: NEXT LIGHT / ANIMATION / AUDIO",0.f,-1.36f,
            5.15f,.028f,.82f,.95f,.88f,-2.62f);
        if(!ReverieNativeRequestSoundV1(state->host,
            kCues[state->demo_mode])) {
            ReverieNativeRequestFeedbackV1(state->host,
                REVERIE_NATIVE_FEEDBACK_ACTIVATION);
        }
        Log(state,REVERIE_NATIVE_LOG_INFO,
            "Showcase lighting/material/sound selection advanced.");
    }
    state->primary_was_down=primary;
}

int32_t RenderEye(
    void *instance,
    const ReverieNativeEyeV1 *eye
) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    if (state == nullptr
        || !ReverieNativeEyeRenderableV1(
            eye
        )
        || state->program == 0
        || state->vbo == 0
        || state->texture == 0) {
        return 0;
    }

    ReverieNativeGlStateV1 gl_state = {};
    ReverieNativeGlAttribStateV1 position_state = {};
    ReverieNativeGlAttribStateV1 uv_state = {};

    ReverieNativeGlStateCaptureV1(
        &gl_state
    );
    ReverieNativeGlAttribCaptureV1(
        &position_state,
        static_cast<GLuint>(
            state->position_location
        )
    );
    ReverieNativeGlAttribCaptureV1(
        &uv_state,
        static_cast<GLuint>(
            state->uv_location
        )
    );

    float view_projection[16];

    // Shell-owned camera movement is already shared across both eyes.
    ReverieNativeMat4Multiply(
        view_projection,
        eye->projection,
        eye->view
    );

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    glUseProgram(state->program);
    glBindBuffer(
        GL_ARRAY_BUFFER,
        state->vbo
    );
    if(state->workshop.mesh_dirty) {
        // One bounded VBO rebuild for a genuine edit, never one per eye.
        // Persistent room/figure prefix remains byte-identical.
        try {
            BakeWorkshopGeometry(state);
            std::vector<float> complete=state->room_avatar_prefix;
            complete.insert(complete.end(),state->editor_mesh.begin(),
                            state->editor_mesh.end());
            glBufferData(GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(complete.size()*sizeof(float)),
                complete.data(),GL_DYNAMIC_DRAW);
        }catch(const std::exception& error){
            Log(state,REVERIE_NATIVE_LOG_ERROR,error.what());
            ReverieNativeGlStateRestoreV1(&gl_state);
            return 0;
        }
    }

    glEnableVertexAttribArray(
        static_cast<GLuint>(
            state->position_location
        )
    );
    glVertexAttribPointer(
        static_cast<GLuint>(
            state->position_location
        ),
        3,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        reinterpret_cast<const void *>(0)
    );

    glEnableVertexAttribArray(
        static_cast<GLuint>(
            state->uv_location
        )
    );
    glVertexAttribPointer(
        static_cast<GLuint>(
            state->uv_location
        ),
        2,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        reinterpret_cast<const void *>(
            3 * sizeof(float)
        )
    );

    glUniformMatrix4fv(
        state->matrix_location,1,GL_FALSE,view_projection
    );
    // Cheap real vertex-light falloff; one animated light for both stereo eyes.
    const float elapsed=state->clock;
    const float pulse=.5f+.5f*std::sin(elapsed*2.2f);
    const float xLight=(state->demo_mode==4)
        ?1.7f*std::sin(elapsed*.7f):.25f;
    const float yLight=(state->demo_mode==4)
        ?.15f+.65f*std::cos(elapsed*.6f):.70f;
    glUniform3f(state->light_location,xLight,yLight,-3.15f);
    const float ambient=state->demo_mode==1?.18f:
        state->demo_mode==2?1.05f:
        state->demo_mode==3?.18f+.35f*pulse:
        state->demo_mode==5?.62f:.39f;
    glUniform1f(state->ambient_location,ambient);
    glUniform3f(state->pivot_location,0.f,0.f,0.f);
    glUniform3f(state->offset_location,0.f,0.f,0.f);
    glUniform1f(state->spin_location,0.f);
    glUniform1f(state->emissive_location,0.f);

    if (state->alternate_tint) {
        glUniform3f(
            state->tint_location,
            1.0f,
            0.72f,
            0.55f
        );
    } else {
        glUniform3f(
            state->tint_location,
            0.82f,
            1.0f,
            0.92f
        );
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(
        GL_TEXTURE_2D,
        state->texture
    );
    glUniform1i(
        state->texture_location,
        0
    );

    glDrawArrays(
        GL_TRIANGLES,
        0,
        36
    );

    if(state->avatar_vertex_count>0) {
        float red=1.f,green=1.f,blue=1.f;
        ReverieNativeAvatarColorV1(
            state->avatar.fields[REVERIE_NATIVE_AVATAR_ACCENT_COLOR],
            &red,&green,&blue);
        glUniform3f(state->tint_location,red,green,blue);
        // Torso/hips/arms/legs share the selected wardrobe accent.
        glDrawArrays(GL_TRIANGLES,36,
            6*REVERIE_NATIVE_AVATAR_VERTICES_PER_PART_V1);
        // Skin/face portion is a separate mesh region.
        glUniform3f(state->tint_location,.90f,.73f,.62f);
        glDrawArrays(GL_TRIANGLES,
            36+6*REVERIE_NATIVE_AVATAR_VERTICES_PER_PART_V1,
            REVERIE_NATIVE_AVATAR_VERTICES_PER_PART_V1);
    }

    // Six paired material sections: garment, trim, hair, hat, prop,
    // eyes. Draw only visible meshes; absent accessories are not rendered.
    if(state->wardrobe_vertex_count>0) {
        const GLint wardrobe_start=36+state->avatar_vertex_count;
        for(unsigned group=0u;group<6u;group++) {
            const uint32_t mask=3u<<(group*2u);
            if((state->wardrobe_visible&mask)==0u)continue;
            float r=1.f,g=1.f,b=1.f;
            ReverieNativeAvatarWardrobeTintV1(
                &state->avatar,group,&r,&g,&b);
            glUniform3f(state->tint_location,r,g,b);
            glDrawArrays(GL_TRIANGLES,
                wardrobe_start+static_cast<GLint>(
                    group*2u*REVERIE_NATIVE_AVATAR_VERTICES_PER_PART_V1),
                2*REVERIE_NATIVE_AVATAR_VERTICES_PER_PART_V1);
        }
    }

    if(state->editor_vertex_count>0) {
        const GLint editor_start=36+state->avatar_vertex_count
            +state->wardrobe_vertex_count;
        // Actual authored compound objects, each retained in one immutable
        // VBO. Changing illumination and animation touches uniforms only.
        for(size_t i=0;i<state->pieces.size();++i) {
            const auto &part=state->pieces[i];
            const std::string &id=part.id;
            float r=.72f,g=.81f,b=.79f,emissive=0.f;
            if(id.find("wood")!=std::string::npos ||
               id.find("crate")!=std::string::npos ||
               id.find("chair")!=std::string::npos ||
               id.find("table")!=std::string::npos) {
                r=.81f;g=.60f;b=.39f;
            } else if(id.find("metal")!=std::string::npos ||
                      id.find("rotor")!=std::string::npos ||
                      id.find("spinner")!=std::string::npos) {
                r=.57f;g=.78f;b=.88f;
            } else if(id.find("beacon")!=std::string::npos ||
                      id.find("light-test")!=std::string::npos) {
                r=.95f;g=.73f;b=.30f;emissive=.22f+.55f*pulse;
            } else if(id.find("glass")!=std::string::npos ||
                      id.find("moving-orb")!=std::string::npos) {
                r=.46f;g=.92f;b=.92f;emissive=.19f+.20f*pulse;
            } else if(id.find("gold")!=std::string::npos) {
                r=1.f;g=.80f;b=.29f;
            } else if(id.find("stone")!=std::string::npos ||
                      id.find("door")!=std::string::npos) {
                r=.54f;g=.59f;b=.65f;
            }
            if(state->workshop.editing&&i==state->workshop.selected){
                r=1.f;g=.84f;b=.27f;emissive=.12f;
            }
            glUniform3f(state->tint_location,r,g,b);
            glUniform1f(state->emissive_location,emissive);
            glUniform3f(state->offset_location,0.f,0.f,0.f);
            glUniform3f(state->pivot_location,
                static_cast<float>(part.x),
                static_cast<float>(part.y),
                static_cast<float>(part.z));
            glUniform1f(state->spin_location,0.f);
            if(id=="moving-orb") {
                glUniform3f(state->offset_location,
                    .27f*std::sin(elapsed),
                    .25f+.19f*std::sin(elapsed*1.7f),
                    .16f*std::cos(elapsed));
            } else if(id=="spinner-arm" || id=="rotor-tip") {
                glUniform3f(state->pivot_location,.35f,-.78f,-2.71f);
                glUniform1f(state->spin_location,elapsed*.8f);
            } else if(id=="beacon-light") {
                glUniform1f(state->emissive_location,.40f+.65f*pulse);
            }
            glDrawArrays(GL_TRIANGLES,
                editor_start+state->piece_first[i],state->piece_count[i]);
        }
        glUniform3f(state->offset_location,0.f,0.f,0.f);
        glUniform1f(state->spin_location,0.f);
        glUniform1f(state->emissive_location,0.f);
    }

    ReverieNativeGlAttribRestoreV1(
        &uv_state
    );
    ReverieNativeGlAttribRestoreV1(
        &position_state
    );
    ReverieNativeGlStateRestoreV1(&gl_state);
    const bool fontOk=state->font.Draw(view_projection);
    return fontOk && glGetError()==GL_NO_ERROR ? 1 : 0;
}

const ReverieNativeModuleCapabilitiesV1 kCapabilities = {
    sizeof(ReverieNativeModuleCapabilitiesV1),
    REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION,
    1.55f,
    1.55f
};

const ReverieNativeModuleApiV1 kApi = {
    sizeof(ReverieNativeModuleApiV1),
    REVERIE_NATIVE_MODULE_ABI_VERSION,
    {
        sizeof(
            ReverieNativeModuleDescriptorV1
        ),
        REVERIE_NATIVE_MODULE_ABI_VERSION,
        "procedural-test-chamber",
        "Procedural Test Chamber",
        2u,
        0u
    },
    Create,
    Destroy,
    OnGlContextCreated,
    ReleaseGlContext,
    Resume,
    Pause,
    Update,
    RenderEye,
    &kCapabilities
};

}  // namespace

extern "C"
__attribute__((visibility("default")))
const ReverieNativeModuleApiV1 *
reverie_native_module_entry_v1(void) {
    return &kApi;
}
