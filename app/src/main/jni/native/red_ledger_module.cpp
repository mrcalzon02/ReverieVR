#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include "red_ledger_simulation.h"
#include "procedural_material_atlas.h"
#include "red_ledger_static_geometry.h"
#include "red_ledger_signage.h"
#include "red_ledger_mug_mesh.h"
#include "red_ledger_trade.h"
#include "red_ledger_wholesale.h"
#include "red_ledger_renovations.h"
#include "red_ledger_stock_layout.h"
#include "red_ledger_original_png.h"
#include "red_ledger_original_fbx.h"
#include "red_ledger_product_assets.h"

#include <GLES2/gl2.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <new>
#include <memory>
#include <vector>

namespace {

using reverie::redledger::DrinkState;
using reverie::redledger::EventType;
using reverie::redledger::Simulation;
using reverie::redledger::SupplierItem;
using reverie::procedural::Material;
namespace geo = reverie::redledger::geometry;
namespace signs = reverie::redledger::signage;
namespace mugs = reverie::redledger::mug;
namespace trade = reverie::redledger::trade;
namespace stock = reverie::redledger::stock;
namespace products = reverie::redledger::products;
namespace property = reverie::redledger::property;

// Include the far rear doorway in seated work-ray reach.
constexpr float kMaximumWorkReach = 4.25f;
constexpr float kHeldCupDistance = 0.90f;
constexpr char kSaveSlot[] = "state-v4.bin";

enum class WorkTarget : uint8_t {
    None = 0,
    CupStack,
    Tap,
    Patron,
    Payment,
    WashStation,
    Ledger,
    BeerOrder,
    CupOrder,
    ProtectionEnvelope,
    ExitDoor,
    DoorStock,
    DoorLiving,
    ToBar,
    ToStock,
    StockNext,
    StockBuy,
    StockSku,
    DrinkChoice,
    SnackChoice,
    UpgradeNext,
    UpgradeBuy,
    Bed
};

struct TargetBounds {
    WorkTarget target;
    float center[3];
    float half_extent[3];
};

static const TargetBounds kWorkTargets[] = {
    {
        WorkTarget::CupStack,
        {0.45f, -0.50f, -0.70f},
        {0.48f, 0.25f, 0.26f}
    },
    {
        WorkTarget::Tap,
        {-0.85f, -0.32f, -0.98f},
        {0.28f, 0.35f, 0.28f}
    },
    {
        WorkTarget::Patron,
        {-1.30f, -0.35f, -2.05f},
        {0.52f, 0.90f, 0.45f}
    },
    {
        WorkTarget::Payment,
        {-0.98f, -0.48f, -0.72f},
        {0.28f, 0.15f, 0.20f}
    },
    {
        WorkTarget::WashStation,
        {-0.28f, -0.48f, -1.18f},
        {0.55f, 0.22f, 0.30f}
    },
    {
        WorkTarget::Ledger,
        {0.85f, -0.50f, -0.90f},
        {0.30f, 0.12f, 0.24f}
    },
    {
        WorkTarget::BeerOrder,
        {1.20f, -0.48f, -0.72f},
        {0.24f, 0.14f, 0.15f}
    },
    {
        WorkTarget::CupOrder,
        {1.68f, -0.48f, -0.72f},
        {0.24f, 0.14f, 0.15f}
    },
    {
        WorkTarget::ProtectionEnvelope,
        {-1.58f, -0.48f, -0.72f},
        {0.26f, 0.13f, 0.15f}
    },
    {
        WorkTarget::ExitDoor,
        {-2.55f, -0.30f, -2.97f},
        {0.35f, 0.85f, 0.12f}
    },
    {WorkTarget::DoorStock,{2.53f,-0.20f,-2.93f},{0.38f,0.85f,0.12f}},
    {WorkTarget::DoorLiving,{0.60f,-0.20f,-2.93f},{0.38f,0.85f,0.12f}},
    {WorkTarget::ToBar,{-2.47f,-0.20f,-2.93f},{0.42f,0.85f,0.13f}},
    {WorkTarget::ToStock,{2.47f,-0.20f,-2.93f},{0.42f,0.85f,0.13f}},
    {WorkTarget::StockNext,{-0.70f,-0.48f,-1.02f},{0.35f,0.30f,0.30f}},
    {WorkTarget::StockBuy,{0.40f,-0.48f,-1.02f},{0.35f,0.30f,0.30f}},
    {WorkTarget::DrinkChoice,{1.25f,-0.50f,-1.10f},{0.32f,0.29f,0.25f}},
    {WorkTarget::SnackChoice,{1.73f,-0.50f,-1.10f},{0.27f,0.29f,0.25f}},
    {WorkTarget::Bed,{1.35f,-0.75f,-1.74f},{0.86f,0.32f,0.75f}},
    {WorkTarget::UpgradeNext,{-1.32f,-0.46f,-1.10f},{0.29f,0.18f,0.25f}},
    {WorkTarget::UpgradeBuy,{-0.52f,-0.46f,-1.10f},{0.29f,0.18f,0.25f}}
};

struct ProductGlResource {
    GLuint vbo = 0;
    GLuint texture = 0;
    GLsizei vertex_count = 0;
};

struct ModuleState {
    const ReverieNativeHostV1 *host = nullptr;
    Simulation simulation;
    trade::Trade market;
    property::Renovations renovations;
    int hovered_offer = -1;
    bool ledger_visible = false;

    GLuint program = 0;
    GLuint cube_vbo = 0;
    GLuint mug_vbo = 0;
    GLuint static_vbo = 0;
    GLuint signage_vbo = 0;
    GLint signage_start[3] = {};
    GLsizei signage_count[3] = {};
    GLuint status_vbo = 0;
    GLsizei status_count = 0;
    bool status_dirty = true;
    GLuint atlas_texture = 0;
    GLuint original_icons[24] = {};
    GLuint original_icon_vbo = 0;
    ProductGlResource product_resources[products::kCount] = {};
    int original_icon_count = 0;
    GLint icon_mode_location = -1;
    GLint position_location = -1;
    GLint uv_location = -1;
    GLint color_attribute_location = -1;
    GLint tile_origin_location = -1;
    GLint sampler_location = -1;
    int active_material = -1;
    GLint matrix_location = -1;
    GLint flicker_location = -1;

    float elapsed_seconds = 0.0f;
    float flicker = 0.90f;
    bool primary_was_down = false;

    bool pointer_active = false;
    float pointer_origin[3] = {};
    float pointer_direction[3] = {
        0.0f,
        0.0f,
        -1.0f
    };

    WorkTarget hovered_target =
        WorkTarget::None;
    float hovered_distance = 0.0f;
};

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
        "ReverieRedLedger",
        message
    );
}

const char *WorkTargetName(
    WorkTarget target
) {
    switch (target) {
        case WorkTarget::CupStack:
            return "cup stack";
        case WorkTarget::Tap:
            return "tap";
        case WorkTarget::Patron:
            return "patron";
        case WorkTarget::Payment:
            return "payment";
        case WorkTarget::WashStation:
            return "wash station";
        case WorkTarget::Ledger:
            return "ledger";
        case WorkTarget::BeerOrder:
            return "beer order card";
        case WorkTarget::CupOrder:
            return "cup order card";
        case WorkTarget::ProtectionEnvelope:
            return "protection envelope";
        case WorkTarget::ExitDoor: return "turn away patron";
        case WorkTarget::DoorStock: return "enter stockroom";
        case WorkTarget::DoorLiving: return "enter living quarters";
        case WorkTarget::ToBar: return "return to bar";
        case WorkTarget::ToStock: return "stockroom route";
        case WorkTarget::StockNext: return "next wholesale catalog item";
        case WorkTarget::StockSku: return "select displayed faction product";
        case WorkTarget::StockBuy: return "purchase selected wholesale stock";
        case WorkTarget::DrinkChoice: return "choose next available drink";
        case WorkTarget::SnackChoice: return "choose next available snack";
        case WorkTarget::UpgradeNext: return "next property renovation";
        case WorkTarget::UpgradeBuy: return "purchase property renovation";
        case WorkTarget::Bed: return "rest and close the day";
        case WorkTarget::None:
        default:
            return "nothing";
    }
}

bool IsTargetAvailable(
    const ModuleState *state,
    WorkTarget target
) {
    if (state == nullptr) {
        return false;
    }

    const Simulation &simulation =
        state->simulation;
    const DrinkState drink_state =
        simulation.work_drink_state();
    const bool no_payment =
        simulation.pending_payment_cents()
            == 0;
    const bool patron_waiting =
        simulation.current_patron()
            != nullptr;

    const trade::Room room=state->market.room();
    const bool hands_free=no_payment && drink_state==DrinkState::None;
    switch (target) {
        case WorkTarget::DoorStock:
        case WorkTarget::DoorLiving:
            return room==trade::Room::Bar && hands_free;
        case WorkTarget::ToBar:
            return room!=trade::Room::Bar && hands_free;
        case WorkTarget::ToStock:
            return room==trade::Room::Living && hands_free;
        case WorkTarget::StockNext:
        case WorkTarget::StockSku:
            return room==trade::Room::Stockroom && hands_free;
        case WorkTarget::StockBuy:
            return room==trade::Room::Stockroom && hands_free &&
                simulation.current_event()!=EventType::SupplyInterruption &&
                (state->market.offer()==0 ||
                    state->market.CanAcceptStock());
        case WorkTarget::DrinkChoice:
            return room==trade::Room::Bar && no_payment && patron_waiting &&
                drink_state!=DrinkState::FilledCup;
        case WorkTarget::SnackChoice:
            return room==trade::Room::Bar && no_payment && patron_waiting;
        case WorkTarget::Bed:
            return room==trade::Room::Living && hands_free &&
                simulation.patrons_remaining()==0;
        case WorkTarget::CupStack:
            return room==trade::Room::Bar && (
                drink_state
                    == DrinkState::None
                && no_payment
                && patron_waiting
                && simulation.clean_cups() > 0
            )
                || (
                    room==trade::Room::Bar &&
                    drink_state==DrinkState::EmptyCup &&
                    no_payment
                );

        case WorkTarget::Tap:
            return room==trade::Room::Bar &&
                state->market.DrinkAvailable(simulation.beer_units()) &&
                drink_state
                    == DrinkState::EmptyCup
                && no_payment;

        case WorkTarget::Patron:
            return room==trade::Room::Bar &&
                state->market.SnackAvailable() && patron_waiting
                && no_payment
                && drink_state
                    == DrinkState::FilledCup;

        case WorkTarget::Payment:
            return room==trade::Room::Bar && simulation.pending_payment_cents()
                > 0;

        case WorkTarget::WashStation:
            return room==trade::Room::Bar && no_payment
                && drink_state
                    == DrinkState::None
                && simulation.dirty_cups() > 0;

        case WorkTarget::Ledger:
        case WorkTarget::UpgradeNext:
        case WorkTarget::UpgradeBuy:
            return room==trade::Room::Living && hands_free;

        case WorkTarget::BeerOrder:
        case WorkTarget::CupOrder:
            return room==trade::Room::Stockroom && no_payment
                && drink_state
                    == DrinkState::None
                && simulation.current_event()
                    != EventType::SupplyInterruption;

        case WorkTarget::ProtectionEnvelope:
            return room==trade::Room::Bar && no_payment
                && drink_state
                    == DrinkState::None
                && simulation.current_event()
                    == EventType::ProtectionDemand
                && !simulation.protection_paid();

        case WorkTarget::ExitDoor:
            return room==trade::Room::Bar && patron_waiting
                && no_payment
                && drink_state
                    == DrinkState::None;

        case WorkTarget::None:
        default:
            return false;
    }
}

float RayAabbDistance(
    const float *origin,
    const float *direction,
    const TargetBounds &bounds
) {
    float near_distance = 0.0f;
    float far_distance =
        kMaximumWorkReach;

    for (int axis = 0;
         axis < 3;
         ++axis) {
        const float minimum =
            bounds.center[axis]
                - bounds.half_extent[axis];
        const float maximum =
            bounds.center[axis]
                + bounds.half_extent[axis];
        const float component =
            direction[axis];

        if (std::abs(component)
            < 0.000001f) {
            if (origin[axis] < minimum
                || origin[axis] > maximum) {
                return -1.0f;
            }
            continue;
        }

        float first =
            (minimum - origin[axis])
                / component;
        float second =
            (maximum - origin[axis])
                / component;

        if (first > second) {
            std::swap(
                first,
                second
            );
        }

        near_distance =
            std::max(
                near_distance,
                first
            );
        far_distance =
            std::min(
                far_distance,
                second
            );

        if (near_distance
            > far_distance) {
            return -1.0f;
        }
    }

    if (far_distance < 0.0f
        || near_distance
            > kMaximumWorkReach) {
        return -1.0f;
    }

    return std::max(
        0.0f,
        near_distance
    );
}

WorkTarget FindWorkTarget(
    const ModuleState *state,
    const ReverieNativeInputV1 *input,
    float *out_distance,
    int *out_offer
) {
    if (out_distance != nullptr) {
        *out_distance = 0.0f;
    }
    if (out_offer != nullptr) *out_offer = -1;

    if (state == nullptr
        || input == nullptr
        || input->pointer_kind
            == REVERIE_NATIVE_POINTER_NONE) {
        return WorkTarget::None;
    }

    WorkTarget best =
        WorkTarget::None;
    float best_distance =
        kMaximumWorkReach + 1.0f;

    for (const TargetBounds &bounds
         : kWorkTargets) {
        if (!IsTargetAvailable(
                state,
                bounds.target
            )) {
            continue;
        }

        const float distance =
            RayAabbDistance(
                input->pointer_origin,
                input->pointer_direction,
                bounds
            );

        if (distance >= 0.0f
            && distance < best_distance) {
            best = bounds.target;
            best_distance = distance;
        }
    }

    // Physical product cards share their selection ray boxes with the
    // signs rendered in the single baked stockroom signage draw.
    if (IsTargetAvailable(state,WorkTarget::StockSku)) {
        for (int slot=0;slot<stock::kCells;++slot) {
            const stock::Cell cell=stock::At(slot);
            const TargetBounds card={
                WorkTarget::StockSku,
                {cell.x,cell.y,cell.z},
                {stock::kHalfWidth,stock::kHalfHeight,0.065f}
            };
            const float distance=RayAabbDistance(
                input->pointer_origin,input->pointer_direction,card);
            if(distance>=0.0f && distance<best_distance) {
                best=WorkTarget::StockSku;
                best_distance=distance;
                if(out_offer!=nullptr)*out_offer=cell.offer;
            }
        }
    }
    if (best != WorkTarget::None
        && out_distance != nullptr) {
        *out_distance =
            best_distance;
    }

    return best;
}

bool SaveState(ModuleState *state) {
    if (state==nullptr || !ReverieNativeHostSupportsSaveV1(state->host))
        return false;
    uint8_t bytes[Simulation::kSerializedSize+trade::Trade::kSerializedSize+
                  property::Renovations::kSerializedSize]={};
    size_t first=0u;
    if (!state->simulation.Serialize(bytes,Simulation::kSerializedSize,&first)
        || first!=Simulation::kSerializedSize
        || !state->market.Serialize(bytes+first,trade::Trade::kSerializedSize)
        || !state->renovations.serialize(
            bytes+first+trade::Trade::kSerializedSize,
            property::Renovations::kSerializedSize)) {
        Log(state,REVERIE_NATIVE_LOG_ERROR,"Red Ledger composite save encoding failed.");
        return false;
    }
    const int32_t result=state->host->write_save(
        kSaveSlot,bytes,static_cast<uint32_t>(sizeof(bytes)));
    if (result!=REVERIE_NATIVE_SAVE_OK) {
        Log(state,REVERIE_NATIVE_LOG_WARN,"Host rejected Red Ledger save write.");
        return false;
    }
    return true;
}

void LoadState(ModuleState *state) {
    if (state==nullptr || !ReverieNativeHostSupportsSaveV1(state->host))
        return;
    uint8_t bytes[Simulation::kSerializedSize+trade::Trade::kSerializedSize+
                  property::Renovations::kSerializedSize]={};
    uint32_t size=0u;
    const int32_t result=state->host->read_save(
        kSaveSlot,bytes,static_cast<uint32_t>(sizeof(bytes)),&size);
    if (result==REVERIE_NATIVE_SAVE_NOT_FOUND) return;
    if (result!=REVERIE_NATIVE_SAVE_OK || size!=sizeof(bytes)) {
        Log(state,REVERIE_NATIVE_LOG_WARN,"Red Ledger pre-1.0 save rejected (v4 required).");
        return;
    }
    // Keep active state atomic: corrupted or incompatible development saves
    // are rejected, not migrated, and do not alter a running game.
    Simulation base=state->simulation;
    trade::Trade market=state->market;
    property::Renovations renovations=state->renovations;
    if (!base.Deserialize(bytes,Simulation::kSerializedSize,true)
        || !market.Deserialize(bytes+Simulation::kSerializedSize,
            trade::Trade::kSerializedSize)
        || !renovations.deserialize(
            bytes+Simulation::kSerializedSize+trade::Trade::kSerializedSize,
            property::Renovations::kSerializedSize)) {
        Log(state,REVERIE_NATIVE_LOG_WARN,
            "Red Ledger rejected corrupt v4 game state; fresh state preserved.");
        return;
    }
    state->simulation=base;
    state->market=market;
    state->renovations=renovations;
    Log(state,REVERIE_NATIVE_LOG_INFO,
        "Red Ledger six-supplier stock and eight renovations restored.");
}

bool ActivateWorkTarget(
    ModuleState *state,
    WorkTarget target
) {
    if (state == nullptr
        || !IsTargetAvailable(
            state,
            target
        )) {
        return false;
    }

    Simulation &simulation =
        state->simulation;
    bool changed = false;

    switch (target) {
        case WorkTarget::CupStack:
            if (simulation.work_drink_state()
                    == DrinkState::None) {
                changed =
                    simulation.TakeCleanCup();
            } else {
                changed =
                    simulation.ReturnHeldCup();
            }
            break;

        case WorkTarget::Tap:
            if (state->market.DrinkAvailable(simulation.beer_units())) {
                changed = state->market.drink()==0
                    ? simulation.FillHeldCup()
                    : simulation.FillSpecialDrink();
                if (changed && state->market.drink()!=0)
                    changed=state->market.ConsumeDrink();
            }
            break;

        case WorkTarget::Patron: {
            const auto *patron=simulation.current_patron();
            if (patron && state->market.SnackAvailable()) {
                const auto bonus=trade::RateService(patron->kind,
                    state->market.drink(),state->market.snack());
                // Stage both service and premium calculations before touching
                // live state; never collect a snack on a failed sale.
                Simulation staged=simulation;
                changed=staged.ServeHeldCup() &&
                    staged.ApplyServicePremium(
                        bonus.cents+state->renovations.service_premium(
                            state->market.drink(),state->market.snack()),
                        std::min(4,bonus.reputation+
                            state->renovations.reputation_bonus()));
                if (changed) {
                    simulation=staged;
                    state->market.ConsumeSnack();
                    state->market.RecordService(patron->kind);
                }
            }
            break;
        }

        case WorkTarget::Payment:
            changed =
                simulation.CollectPayment();
            break;

        case WorkTarget::WashStation:
            changed =
                simulation.WashOneCup();
            break;

        case WorkTarget::Ledger:
            state->ledger_visible=!state->ledger_visible;
            changed=true;
            break;
        case WorkTarget::UpgradeNext:
            changed=state->renovations.next();
            state->ledger_visible=false;
            break;
        case WorkTarget::UpgradeBuy:
            changed=state->renovations.buy(&simulation,state->market);
            if(changed)state->ledger_visible=false;
            break;
        case WorkTarget::Bed: {
            const int32_t today=simulation.day();
            simulation.CloseDay();
            changed=simulation.day()!=today;
            if (changed) state->ledger_visible=true;
            break;
        }
        case WorkTarget::DoorStock:
        case WorkTarget::ToStock:
            changed=state->market.Enter(trade::Room::Stockroom);
            break;
        case WorkTarget::DoorLiving:
            changed=state->market.Enter(trade::Room::Living);
            break;
        case WorkTarget::ToBar:
            changed=state->market.Enter(trade::Room::Bar);
            break;
        case WorkTarget::StockNext:
            changed=state->market.CycleOffer();
            break;
        case WorkTarget::StockSku:
            changed=state->market.SelectOffer(state->hovered_offer);
            break;
        case WorkTarget::StockBuy:
            changed=state->market.offer()==0
                ? simulation.BuySupply(SupplierItem::BeerCrate)
                : trade::BuyWholesale(&simulation,&state->market);
            break;
        case WorkTarget::DrinkChoice:
            changed=state->market.SelectDrink(simulation.beer_units());
            break;
        case WorkTarget::SnackChoice:
            changed=state->market.SelectSnack();
            break;

        case WorkTarget::BeerOrder:
            changed =
                simulation.BuySupply(
                    SupplierItem::BeerCrate
                );
            break;

        case WorkTarget::CupOrder:
            changed =
                simulation.BuySupply(
                    SupplierItem::CupSet
                );
            break;

        case WorkTarget::ProtectionEnvelope:
            changed =
                simulation.PayProtection();
            break;

        case WorkTarget::ExitDoor:
            changed =
                simulation.TurnAwayCurrentPatron();
            break;

        case WorkTarget::None:
        default:
            break;
    }

    if (!changed) {
        Log(
            state,
            REVERIE_NATIVE_LOG_WARN,
            "Targeted bar interaction could not complete."
        );
        ReverieNativeRequestFeedbackV1(
            state->host,
            REVERIE_NATIVE_FEEDBACK_FAILURE
        );
        return false;
    }

    Log(
        state,
        REVERIE_NATIVE_LOG_DEBUG,
        WorkTargetName(target)
    );
    ReverieNativeRequestFeedbackV1(
        state->host,
        REVERIE_NATIVE_FEEDBACK_ACTIVATION
    );
    uint32_t sound=REVERIE_NATIVE_SOUND_RED_LEDGER_CLICK;
    switch(target) {
        case WorkTarget::Tap:
        case WorkTarget::WashStation:
            sound=REVERIE_NATIVE_SOUND_RED_LEDGER_METAL;break;
        case WorkTarget::Payment:
        case WorkTarget::StockBuy:
        case WorkTarget::BeerOrder:
        case WorkTarget::CupOrder:
        case WorkTarget::UpgradeBuy:
            sound=REVERIE_NATIVE_SOUND_RED_LEDGER_COINS;break;
        case WorkTarget::Ledger:
        case WorkTarget::Bed:
            sound=REVERIE_NATIVE_SOUND_RED_LEDGER_BOOK;break;
        case WorkTarget::DoorStock:
        case WorkTarget::DoorLiving:
        case WorkTarget::ToStock:
        case WorkTarget::ToBar:
        case WorkTarget::ExitDoor:
            sound=REVERIE_NATIVE_SOUND_RED_LEDGER_DOOR;break;
        case WorkTarget::StockSku:
        case WorkTarget::StockNext:
            sound=REVERIE_NATIVE_SOUND_RED_LEDGER_DRAWER;break;
        default:break;
    }
    ReverieNativeRequestSoundV1(state->host,sound);
    state->status_dirty=true;
    SaveState(state);
    return true;
}

GLuint BuildProgram() {
    static const char *kVertexShader =
        "uniform mat4 u_Mvp;\n"
        "uniform float u_IconMode;\n"
        "attribute vec3 a_Position;\n"
        "attribute vec2 a_Uv;\n"
        "attribute vec3 a_Color;\n"
        "attribute vec2 a_TileOrigin;\n"
        "varying vec2 v_Uv;\n"
        "varying vec3 v_Color;\n"
        "void main() {\n"
        "  v_Uv = u_IconMode > 0.5 ? a_Uv : a_TileOrigin + a_Uv * 0.4921875;\n"
        "  v_Color = a_Color;\n"
        "  gl_Position = u_Mvp * vec4(a_Position, 1.0);\n"
        "}\n";

    static const char *kFragmentShader =
        "precision mediump float;\n"
        "uniform float u_Flicker;\n"
        "uniform sampler2D u_Atlas;\n"
        "varying vec2 v_Uv;\n"
        "varying vec3 v_Color;\n"
        "void main() {\n"
        "  vec4 material = texture2D(u_Atlas, v_Uv);\n"
        "  gl_FragColor = vec4(material.rgb * v_Color * u_Flicker, material.a);\n"
        "}\n";

    static const ReverieNativeGlAttributeBindingV1 kBindings[] = {
        {0u, "a_Position"},
        {1u, "a_Uv"},
        {2u, "a_Color"},
        {3u, "a_TileOrigin"}
    };

    return ReverieNativeBuildProgram(
        kVertexShader,
        kFragmentShader,
        kBindings,
        sizeof(kBindings)
            / sizeof(kBindings[0])
    );
}

void MakeTransform(
    float *out,
    float x,
    float y,
    float z,
    float sx,
    float sy,
    float sz
) {
    const float result[16] = {
        sx, 0.0f, 0.0f, 0.0f,
        0.0f, sy, 0.0f, 0.0f,
        0.0f, 0.0f, sz, 0.0f,
        x, y, z, 1.0f
    };

    std::memcpy(
        out,
        result,
        sizeof(result)
    );
}

// UVs are baked once per context, never recomputed per eye or draw call.
void BuildCubeVertices(float *out) {
    for (int index = 0; index < 36; ++index) {
        const int face = index / 6;
        const float *position = &geo::kUnitCubeVertices[index * 3];
        const float u = (face < 2 || face >= 4)
            ? position[0] + 0.5f
            : position[2] + 0.5f;
        const float v = (face >= 4)
            ? position[2] + 0.5f
            : position[1] + 0.5f;
        for (int axis = 0; axis < 3; ++axis) {
            out[index * 5 + axis] = position[axis];
        }
        out[index * 5 + 3] = u;
        out[index * 5 + 4] = v;
    }
}

void DestroyGl(
    ModuleState *state
) {
    glDeleteTextures(24,state->original_icons);
    for(GLuint &icon:state->original_icons)icon=0;
    state->original_icon_count=0;
    if(state->original_icon_vbo!=0){
        glDeleteBuffers(1,&state->original_icon_vbo);
        state->original_icon_vbo=0;
    }
    for(ProductGlResource &resource:state->product_resources) {
        if(resource.vbo!=0)glDeleteBuffers(1,&resource.vbo);
        if(resource.texture!=0)glDeleteTextures(1,&resource.texture);
        resource=ProductGlResource{};
    }
    if (state->atlas_texture != 0) {
        glDeleteTextures(1, &state->atlas_texture);
        state->atlas_texture = 0;
    }
    if (state->mug_vbo != 0) {
        glDeleteBuffers(1, &state->mug_vbo);
        state->mug_vbo = 0;
    }
    if (state->signage_vbo != 0) {
        glDeleteBuffers(1, &state->signage_vbo);
        state->signage_vbo = 0;
    }
    if (state->status_vbo != 0) {
        glDeleteBuffers(1, &state->status_vbo);
        state->status_vbo = 0;
    }
    for (int i=0;i<3;++i) {
        state->signage_start[i]=0;
        state->signage_count[i]=0;
    }
    state->status_count=0;
    state->status_dirty=true;
    if (state->static_vbo != 0) {
        glDeleteBuffers(1, &state->static_vbo);
        state->static_vbo = 0;
    }
    if (state->cube_vbo != 0) {
        glDeleteBuffers(
            1,
            &state->cube_vbo
        );
        state->cube_vbo = 0;
    }

    if (state->program != 0) {
        glDeleteProgram(state->program);
        state->program = 0;
    }

    state->position_location = -1;
    state->uv_location = -1;
    state->color_attribute_location = -1;
    state->tile_origin_location = -1;
    state->sampler_location = -1;
    state->icon_mode_location = -1;
    state->active_material = -1;
    state->matrix_location = -1;
    state->flicker_location = -1;
}

void AbandonGl(
    ModuleState *state
) {
    for(GLuint &icon:state->original_icons)icon=0;
    state->original_icon_count=0;
    state->original_icon_vbo=0;
    for(ProductGlResource &resource:state->product_resources)
        resource=ProductGlResource{};
    state->icon_mode_location=-1;
    state->cube_vbo = 0;
    state->mug_vbo = 0;
    state->static_vbo = 0;
    state->signage_vbo = 0;
    state->status_vbo = 0;
    state->status_count = 0;
    state->status_dirty = true;
    for (int i=0;i<3;++i){
        state->signage_start[i]=0;
        state->signage_count[i]=0;
    }
    state->atlas_texture = 0;
    state->program = 0;
    state->position_location = -1;
    state->uv_location = -1;
    state->color_attribute_location = -1;
    state->tile_origin_location = -1;
    state->sampler_location = -1;
    state->icon_mode_location = -1;
    state->active_material = -1;
    state->matrix_location = -1;
    state->flicker_location = -1;
}

int32_t InitializeGl(
    ModuleState *state
) {
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

    state->program =
        BuildProgram();

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
        glGetAttribLocation(state->program, "a_Uv");
    state->color_attribute_location =
        glGetAttribLocation(state->program, "a_Color");
    state->tile_origin_location =
        glGetAttribLocation(state->program, "a_TileOrigin");
    state->sampler_location =
        glGetUniformLocation(state->program, "u_Atlas");
    state->icon_mode_location =
        glGetUniformLocation(state->program,"u_IconMode");
    state->matrix_location =
        glGetUniformLocation(
            state->program,
            "u_Mvp"
        );
    state->flicker_location =
        glGetUniformLocation(
            state->program,
            "u_Flicker"
        );

    if (state->position_location < 0
        || state->uv_location < 0
        || state->color_attribute_location < 0
        || state->tile_origin_location < 0
        || state->sampler_location < 0
        || state->matrix_location < 0
        || state->flicker_location < 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Required shader locations are unavailable."
        );
        DestroyGl(state);
        return finish(0);
    }

    glGenBuffers(
        1,
        &state->cube_vbo
    );

    if (state->cube_vbo == 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Cube vertex buffer creation failed."
        );
        DestroyGl(state);
        return finish(0);
    }

    glBindBuffer(
        GL_ARRAY_BUFFER,
        state->cube_vbo
    );
    float cube_vertices[36 * 5];
    BuildCubeVertices(cube_vertices);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(cube_vertices),
        cube_vertices,
        GL_STATIC_DRAW
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // One reusable, hollow, twelve-sided mug rather than a cuboid vessel.
    std::unique_ptr<float[]> mug_vertices(
        new (std::nothrow) float[mugs::kFloatCount]);
    if (!mug_vertices || !mugs::Build(mug_vertices.get(),mugs::kFloatCount)) {
        Log(state, REVERIE_NATIVE_LOG_ERROR,"Mug mesh generation failed.");
        DestroyGl(state);
        return finish(0);
    }
    glGenBuffers(1, &state->mug_vbo);
    if (state->mug_vbo == 0) {
        Log(state, REVERIE_NATIVE_LOG_ERROR,"Mug VBO creation failed.");
        DestroyGl(state);
        return finish(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER,state->mug_vbo);
    glBufferData(GL_ARRAY_BUFFER,mugs::kFloatCount*sizeof(float),
        mug_vertices.get(),GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    mug_vertices.reset();

    // One context-lifetime world-space batch replaces 11 static draw calls.
    std::unique_ptr<float[]> room_vertices(
        new (std::nothrow) float[geo::kStaticVertexFloats]
    );
    if (!room_vertices || !geo::BuildStaticRoomVertices(
            room_vertices.get(), geo::kStaticVertexFloats)) {
        Log(state, REVERIE_NATIVE_LOG_ERROR,
            "Static room batch generation failed.");
        DestroyGl(state);
        return finish(0);
    }
    glGenBuffers(1, &state->static_vbo);
    if (state->static_vbo == 0) {
        Log(state, REVERIE_NATIVE_LOG_ERROR,
            "Static room VBO allocation failed.");
        DestroyGl(state);
        return finish(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, state->static_vbo);
    glBufferData(GL_ARRAY_BUFFER, geo::kStaticVertexBytes,
        room_vertices.get(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    room_vertices.reset();

    // Bake real, front-facing glyph geometry once per GL context. These
    // share the paper material and static attribute layout (no extra shader).
    size_t counts[3]={};
    size_t sign_count=0u;
    for (int room=0;room<3;++room){
        counts[room]=signs::VertexCount(room);
        sign_count+=counts[room];
    }
    std::unique_ptr<float[]> sign_vertices(
        new (std::nothrow) float[sign_count * signs::kVertexStride]
    );
    if(!sign_vertices || sign_count==0u){
        Log(state, REVERIE_NATIVE_LOG_ERROR,
            "Room lettering allocation failed.");
        DestroyGl(state);
        return finish(0);
    }
    size_t start_at=0u;
    for(int room=0;room<3;++room){
        size_t generated=0u;
        if(!signs::Build(sign_vertices.get()+
                start_at*signs::kVertexStride,
                counts[room]*signs::kVertexStride,&generated,room)
            ||generated!=counts[room]){
            Log(state,REVERIE_NATIVE_LOG_ERROR,
                "Room lettering generation failed.");
            DestroyGl(state);
            return finish(0);
        }
        state->signage_start[room]=static_cast<GLint>(start_at);
        state->signage_count[room]=static_cast<GLsizei>(counts[room]);
        start_at+=counts[room];
    }
    glGenBuffers(1,&state->signage_vbo);
    glGenBuffers(1,&state->status_vbo);
    if(state->signage_vbo==0||state->status_vbo==0){
        Log(state,REVERIE_NATIVE_LOG_ERROR,
            "Room text buffers unavailable.");
        DestroyGl(state);
        return finish(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER,state->signage_vbo);
    glBufferData(GL_ARRAY_BUFFER,sign_count*signs::kVertexStride*sizeof(float),
        sign_vertices.get(),GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    sign_vertices.reset();
    state->status_count=0;
    state->status_dirty=true;


    // Exact original product icon cards: one immutable GPU quad per stock
    // cell. The slot order matches the existing controller hit boxes.
    float icon_quads[stock::kCells*6*geo::kStaticVertexStride]={};
    for(int slot=0;slot<stock::kCells;++slot) {
        const stock::Cell cell=stock::At(slot);
        const float x0=cell.x-.098f,x1=cell.x+.098f;
        const float y0=cell.y-.026f,y1=cell.y+.154f;
        const float z=cell.z+.028f;
        auto vertex=[&](int i,float x,float y,float u,float v) {
            float *dst=icon_quads+
                (slot*6+i)*geo::kStaticVertexStride;
            dst[0]=x;dst[1]=y;dst[2]=z;
            dst[3]=u;dst[4]=v;
            dst[5]=1.f;dst[6]=1.f;dst[7]=1.f;
            dst[8]=0.f;dst[9]=0.f;
        };
        // PNG memory begins with its TOP row, so world-space top uses v=0.
        vertex(0,x0,y0,0.f,1.f);
        vertex(1,x1,y0,1.f,1.f);
        vertex(2,x1,y1,1.f,0.f);
        vertex(3,x0,y0,0.f,1.f);
        vertex(4,x1,y1,1.f,0.f);
        vertex(5,x0,y1,0.f,0.f);
    }
    glGenBuffers(1,&state->original_icon_vbo);
    if(state->original_icon_vbo!=0) {
        glBindBuffer(GL_ARRAY_BUFFER,state->original_icon_vbo);
        glBufferData(GL_ARRAY_BUFFER,sizeof(icon_quads),
                     icon_quads,GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER,0);
    }

    // Generate only on GL-context creation/recreation. The temporary CPU
    // buffer is freed immediately after upload; no runtime bitmap asset.
    // RGB565 is 32 KiB instead of 64 KiB RGBA8, and needs no
    // intermediate bitmap or conversion allocation.
    std::unique_ptr<uint16_t[]> pixels(
        new (std::nothrow) uint16_t[reverie::procedural::kAtlasPixelCount]
    );
    if (!pixels || !reverie::procedural::GenerateMaterialAtlasRgb565(
            reverie::procedural::kRedLedgerMaterialSeed,
            pixels.get(),
            reverie::procedural::kAtlasRgb565Bytes)) {
        Log(state, REVERIE_NATIVE_LOG_ERROR,
            "Procedural atlas allocation/generation failed.");
        DestroyGl(state);
        return finish(0);
    }
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &state->atlas_texture);
    if (state->atlas_texture != 0) {
        glBindTexture(GL_TEXTURE_2D, state->atlas_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB,
            reverie::procedural::kAtlasWidth,
            reverie::procedural::kAtlasHeight,
            0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels.get()
        );
    }
    pixels.reset();

    // Only required room/shader/atlas resources can prevent launch.
    if (state->atlas_texture == 0 || glGetError()
        != GL_NO_ERROR) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "OpenGL ES initialization reported an error."
        );
        DestroyGl(state);
        return finish(0);
    }


    // Load the owner's exported 300x300 transparent PNGs once per GL
    // context. No thumbnail substitution, per-frame decoding or JNI calls.
    const bool can_read_assets=state->host &&
        state->host->struct_size>=REVERIE_NATIVE_HOST_V1_ASSET_MIN_SIZE &&
        state->host->read_asset!=nullptr;
    if(can_read_assets && state->original_icon_vbo!=0) {
        for(int index=0;index<24;++index) {
            char asset[96]={};
            std::snprintf(asset,sizeof(asset),
                "native/red-ledger/icons/icon_%02d_300_v2_rt_final.png",
                index+1);
            uint32_t bytes=0;
            const int32_t need=state->host->read_asset(asset,nullptr,0,&bytes);
            if(need!=REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL ||
               bytes<64u || bytes>1024u*1024u)continue;
            std::vector<uint8_t> png(bytes);
            uint32_t got=0;
            if(state->host->read_asset(asset,png.data(),bytes,&got)!=
                    REVERIE_NATIVE_SAVE_OK || got!=bytes)continue;
            std::vector<uint8_t> rgba;
            if(!reverie::redledger::original_png::Decode300Rgba(
                    png.data(),png.size(),&rgba))continue;
            glGenTextures(1,&state->original_icons[index]);
            if(state->original_icons[index]==0)continue;
            glBindTexture(GL_TEXTURE_2D,state->original_icons[index]);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,300,300,0,
                         GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
            if(glGetError()==GL_NO_ERROR) {
                ++state->original_icon_count;
            } else {
                glDeleteTextures(1,&state->original_icons[index]);
                state->original_icons[index]=0;
            }
        }
        glBindTexture(GL_TEXTURE_2D,state->atlas_texture);
    }
    char report[96]={};
    std::snprintf(report,sizeof(report),
        "Original Unreal shop icons loaded: %d/24",
        state->original_icon_count);
    Log(state,state->original_icon_count==24 ?
        REVERIE_NATIVE_LOG_INFO:REVERIE_NATIVE_LOG_WARN,report);



    // Original Unreal merchandise is decoded and uploaded once per GL context.
    // Four representative product classes are currently packaged; missing
    // products never substitute guessed geometry or the UCX collision meshes.
    int original_product_count=0;
    if(can_read_assets) {
        auto load_asset=[state](const char *path,uint32_t limit,
                                std::vector<uint8_t> *buffer)->bool {
            uint32_t bytes=0;
            if(state->host->read_asset(path,nullptr,0,&bytes)!=
                REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL ||
                bytes==0 || bytes>limit)return false;
            buffer->resize(bytes);
            uint32_t received=0;
            return state->host->read_asset(path,buffer->data(),bytes,
                        &received)==REVERIE_NATIVE_SAVE_OK &&
                   received==bytes;
        };
        for(int i=0;i<products::kCount;++i) {
            const products::Entry &entry=products::kCatalog[i];
            std::vector<uint8_t> fbx_file,texture_file,rgba;
            std::vector<float> geometry;
            if(!load_asset(entry.mesh_path,3u*1024u*1024u,&fbx_file) ||
               !load_asset(entry.texture_path,3u*1024u*1024u,
                           &texture_file) ||
               !fbx::BuildWorldTriangles(fbx_file.data(),fbx_file.size(),
                                         &geometry,entry.model_name) ||
               !original_png::DecodeRgba(texture_file.data(),
                                         texture_file.size(),1024u,&rgba) ||
               geometry.size()%geo::kStaticVertexStride!=0u ||
               geometry.size()/geo::kStaticVertexStride>90000u)
                continue;
            ProductGlResource &resource=state->product_resources[i];
            glGenBuffers(1,&resource.vbo);
            glGenTextures(1,&resource.texture);
            if(resource.vbo!=0 && resource.texture!=0) {
                glBindBuffer(GL_ARRAY_BUFFER,resource.vbo);
                glBufferData(GL_ARRAY_BUFFER,geometry.size()*sizeof(float),
                             geometry.data(),GL_STATIC_DRAW);
                glBindBuffer(GL_ARRAY_BUFFER,0);
                glBindTexture(GL_TEXTURE_2D,resource.texture);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1024,1024,0,
                             GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
                if(glGetError()==GL_NO_ERROR) {
                    resource.vertex_count=static_cast<GLsizei>(
                        geometry.size()/geo::kStaticVertexStride);
                    ++original_product_count;
                    continue;
                }
            }
            if(resource.vbo!=0)glDeleteBuffers(1,&resource.vbo);
            if(resource.texture!=0)glDeleteTextures(1,&resource.texture);
            resource=ProductGlResource{};
        }
        glBindTexture(GL_TEXTURE_2D,state->atlas_texture);
    }
    char model_report[96]={};
    std::snprintf(model_report,sizeof(model_report),
                  "Original Unreal product models loaded: %d/%d",
                  original_product_count,products::kCount);
    Log(state,original_product_count==products::kCount?
        REVERIE_NATIVE_LOG_INFO:REVERIE_NATIVE_LOG_WARN,model_report);

    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Red Ledger GL: 32 KiB RGB565 atlas and one static room VBO."
    );
    return finish(1);
}

void DrawCube(
    ModuleState *state,
    const float *view_projection,
    float flicker,
    float x,
    float y,
    float z,
    float sx,
    float sy,
    float sz,
    float r,
    float g,
    float b,
    Material material = Material::Stone
) {
    float model[16];
    float mvp[16];

    MakeTransform(
        model,
        x,
        y,
        z,
        sx,
        sy,
        sz
    );
    ReverieNativeMat4Multiply(
        mvp,
        view_projection,
        model
    );

    glUniformMatrix4fv(
        state->matrix_location,
        1,
        GL_FALSE,
        mvp
    );
    glVertexAttrib3f(
        static_cast<GLuint>(state->color_attribute_location),
        r, g, b
    );
    glUniform1f(
        state->flicker_location,
        flicker
    );
    const int material_index = static_cast<int>(material);
    if (state->active_material != material_index) {
        state->active_material = material_index;
        glVertexAttrib2f(
            static_cast<GLuint>(state->tile_origin_location),
            0.00390625f + (material_index & 1) * 0.5f,
            0.00390625f + (material_index >> 1) * 0.5f
        );
    }
    glDrawArrays(
        GL_TRIANGLES,
        0,
        36
    );
}

// Switch to the immutable faceted mesh for the mug body, then restore the
// ordinary cube VBO used for handles, taps, NPCs and all other props.
void DrawMugBody(ModuleState *state,const float *vp,float flicker,
                 float x,float y,float z,float sx,float sy,float sz,
                 float r,float g,float b,Material material) {
    float model[16],mvp[16];
    MakeTransform(model,x,y,z,sx,sy,sz);
    ReverieNativeMat4Multiply(mvp,vp,model);
    glUniformMatrix4fv(state->matrix_location,1,GL_FALSE,mvp);
    glVertexAttrib3f(static_cast<GLuint>(state->color_attribute_location),
                     r,g,b);
    glUniform1f(state->flicker_location,flicker);
    const int id=static_cast<int>(material);
    state->active_material=id;
    glVertexAttrib2f(static_cast<GLuint>(state->tile_origin_location),
        0.00390625f+(id&1)*0.5f,
        0.00390625f+(id>>1)*0.5f);
    glBindBuffer(GL_ARRAY_BUFFER,state->mug_vbo);
    glVertexAttribPointer(static_cast<GLuint>(state->position_location),
        3,GL_FLOAT,GL_FALSE,5*sizeof(float),
        reinterpret_cast<const void *>(0));
    glVertexAttribPointer(static_cast<GLuint>(state->uv_location),
        2,GL_FLOAT,GL_FALSE,5*sizeof(float),
        reinterpret_cast<const void *>(3u*sizeof(float)));
    glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(mugs::kVertexCount));
    glBindBuffer(GL_ARRAY_BUFFER,state->cube_vbo);
    glVertexAttribPointer(static_cast<GLuint>(state->position_location),
        3,GL_FLOAT,GL_FALSE,5*sizeof(float),
        reinterpret_cast<const void *>(0));
    glVertexAttribPointer(static_cast<GLuint>(state->uv_location),
        2,GL_FLOAT,GL_FALSE,5*sizeof(float),
        reinterpret_cast<const void *>(3u*sizeof(float)));
}
void DrawCup(
    ModuleState *state,
    const float *view_projection,
    float flicker,
    float x,
    float y,
    float z,
    bool filled,
    bool highlighted
) {
    float r = filled ? 0.72f : 0.52f;
    float g = filled ? 0.43f : 0.54f;
    float b = filled ? 0.16f : 0.50f;
    if (highlighted) {
        r=std::min(1.0f,r+0.24f);
        g=std::min(1.0f,g+0.24f);
        b=std::min(1.0f,b+0.18f);
    }
    DrawMugBody(state,view_projection,flicker,
        x,y,z,0.15f,0.20f,0.15f,r,g,b,Material::Metal);
    // Side grip is attached outside the round vessel and stays visible.
    DrawCube(state,view_projection,flicker,
        x+0.096f,y,z,0.042f,0.115f,0.052f,
        0.78f,0.76f,0.66f,Material::Metal);
    if (filled) {
        // A small dark/gold insert reads as liquid through the open rim.
        DrawMugBody(state,view_projection,flicker,
            x,y+0.100f,z,0.118f,0.010f,0.118f,
            0.68f,0.39f,0.10f,Material::Wood);
    }
}

// Build a room's current economic information as REAL glyph geometry.
// Called on an actual state change, never for every frame or both eyes.
void RefreshStatusBoard(ModuleState *state) {
    if(!state->status_dirty || state->status_vbo==0) return;
    char rows[6][96]={};
    const Simulation &sim=state->simulation;
    const trade::Trade &market=state->market;
    const auto room=market.room();
    if(room==trade::Room::Bar) {
        const auto *patron=sim.current_patron();
        if(patron) {
            const int kind=trade::FactionFor(patron->kind);
            std::snprintf(rows[0],sizeof(rows[0]),"FACTION: %s",
                trade::kFactionNames[kind]);
            std::snprintf(rows[1],sizeof(rows[1]),"WANTS: %s",
                trade::kDrinkCatalog[trade::FavoriteDrink(patron->kind)].name);
            std::snprintf(rows[2],sizeof(rows[2]),"SNACK: %s",
                trade::kSnackCatalog[trade::FavoriteSnack(patron->kind)].name);
        } else {
            std::snprintf(rows[0],sizeof(rows[0]),"NO CUSTOMERS");
            std::snprintf(rows[1],sizeof(rows[1]),"REST IN LIVING QUARTERS");
        }
        std::snprintf(rows[3],sizeof(rows[3]),"POUR: %s",
            trade::kDrinkCatalog[market.drink()].name);
        std::snprintf(rows[4],sizeof(rows[4]),"WITH: %s",
            market.snack()<0?"NONE":trade::kSnackCatalog[market.snack()].name);
        std::snprintf(rows[5],sizeof(rows[5]),"CASH: %d  BEER: %d  CUPS: %d",
            sim.cash_cents(),sim.beer_units(),sim.clean_cups());
    } else if(room==trade::Room::Stockroom) {
        const int sku=market.offer();
        const int stock=sku<trade::kDrinks?
            (sku==0?sim.beer_units():market.stock_drink(sku)):
            market.stock_snack(sku-trade::kDrinks);
        std::snprintf(rows[0],sizeof(rows[0]),"PRODUCT: %s",
            market.offer_name());
        std::snprintf(rows[1],sizeof(rows[1]),"COST: %d  UNITS: %d",
            sku==0?900:market.offer_cost(),
            sku==0?8:(sku<trade::kDrinks?
                trade::kDrinkCatalog[sku].units:
                trade::kSnackCatalog[sku-trade::kDrinks].units));
        std::snprintf(rows[2],sizeof(rows[2]),"ON HAND: %d",stock);
        std::snprintf(rows[3],sizeof(rows[3]),"CASH: %d",sim.cash_cents());
        std::snprintf(rows[4],sizeof(rows[4]),"SELECT CARD THEN BUY");
        if(sim.current_event()==EventType::SupplyInterruption)
            std::snprintf(rows[5],sizeof(rows[5]),"SUPPLIER BLOCKED TODAY");
        else
            std::snprintf(rows[5],sizeof(rows[5]),"SUPPLY FACTION: %s",
                market.offer_faction());
    } else {
        std::snprintf(rows[0],sizeof(rows[0]),"DAY: %d  CASH: %d",
            sim.day(),sim.cash_cents());
        std::snprintf(rows[1],sizeof(rows[1]),"DEBT: %d  REP: %d",
            sim.debt_cents(),sim.reputation());
        if(state->ledger_visible) {
            std::snprintf(rows[2],sizeof(rows[2]),"101:%d 202:%d",
                market.favor(0),market.favor(1));
            std::snprintf(rows[3],sizeof(rows[3]),"303:%d 404:%d",
                market.favor(2),market.favor(3));
            std::snprintf(rows[4],sizeof(rows[4]),"505:%d 606:%d",
                market.favor(4),market.favor(5));
            const auto &ledger=sim.last_ledger();
            std::snprintf(rows[5],sizeof(rows[5]),"DAY %d SALES %d LOST %d",
                ledger.day,ledger.sales_cents,ledger.patrons_lost);
        } else {
            const int category=state->renovations.category();
            const int tier=state->renovations.tier(category);
            std::snprintf(rows[2],sizeof(rows[2]),"RENOVATE: %s %d/5",
                property::kNames[category],tier);
            std::snprintf(rows[3],sizeof(rows[3]),
                tier==5?"FULLY IMPROVED":"NEXT TIER COST: %d",
                state->renovations.cost(market));
            std::snprintf(rows[4],sizeof(rows[4]),"VENDOR: %s",
                market.offer_faction());
            std::snprintf(rows[5],sizeof(rows[5]),
                "LEFT: CATEGORY  RIGHT: BUY  LEDGER: ACCOUNTS");
        }
    }
    size_t count=0u;
    for(size_t i=0;i<6u;++i)
        if(rows[i][0]!=0)
            count=signs::EmitLine(nullptr,count,rows[i],0.0f,
                1.09f-static_cast<float>(i)*0.22f,-2.80f,0.014f);
    if(count==0u || count>20000u)return;
    std::unique_ptr<float[]> vertices(
        new (std::nothrow) float[count*signs::kVertexStride]);
    if(!vertices)return;
    size_t made=0u;
    for(size_t i=0;i<6u;++i)
        if(rows[i][0]!=0)
            made=signs::EmitLine(vertices.get(),made,rows[i],0.0f,
                1.09f-static_cast<float>(i)*0.22f,-2.80f,0.014f);
    if(made!=count)return;
    glBindBuffer(GL_ARRAY_BUFFER,state->status_vbo);
    glBufferData(GL_ARRAY_BUFFER,count*signs::kVertexStride*sizeof(float),
        vertices.get(),GL_DYNAMIC_DRAW);
    state->status_count=static_cast<GLsizei>(count);
    state->status_dirty=false;
}

// Immutable world-space room: one GLES2 draw per eye, no per-frame upload.
void DrawStaticRoom(ModuleState *state, const float *vp, float flicker) {
    const GLsizei stride = static_cast<GLsizei>(
        geo::kStaticVertexStride * sizeof(float)
    );
    glBindBuffer(GL_ARRAY_BUFFER, state->static_vbo);
    glVertexAttribPointer(
        static_cast<GLuint>(state->position_location),
        3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void *>(0)
    );
    glVertexAttribPointer(
        static_cast<GLuint>(state->uv_location),
        2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void *>(3u * sizeof(float))
    );
    glEnableVertexAttribArray(
        static_cast<GLuint>(state->color_attribute_location)
    );
    glVertexAttribPointer(
        static_cast<GLuint>(state->color_attribute_location),
        3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void *>(5u * sizeof(float))
    );
    glEnableVertexAttribArray(
        static_cast<GLuint>(state->tile_origin_location)
    );
    glVertexAttribPointer(
        static_cast<GLuint>(state->tile_origin_location),
        2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void *>(8u * sizeof(float))
    );
    glUniformMatrix4fv(state->matrix_location, 1, GL_FALSE, vp);
    glUniform1f(state->flicker_location, flicker);
    const GLint room_start=static_cast<GLint>(state->market.room()) *
        static_cast<GLint>(geo::kRoomVertexCount);
    glDrawArrays(GL_TRIANGLES,room_start,
        static_cast<GLsizei>(geo::kRoomVertexCount));

    // Stationary, legible lettering: one extra draw per eye, no per-frame
    // font rasterization, atlas upload, or individual character calls.
    glBindBuffer(GL_ARRAY_BUFFER, state->signage_vbo);
    glVertexAttribPointer(static_cast<GLuint>(state->position_location),
        3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void *>(0));
    glVertexAttribPointer(static_cast<GLuint>(state->uv_location),
        2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void *>(3u * sizeof(float)));
    glVertexAttribPointer(static_cast<GLuint>(state->color_attribute_location),
        3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void *>(5u * sizeof(float)));
    glVertexAttribPointer(static_cast<GLuint>(state->tile_origin_location),
        2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void *>(8u * sizeof(float)));
    glUniform1f(state->flicker_location, 1.0f);
    const int room_index=static_cast<int>(state->market.room());
    glDrawArrays(GL_TRIANGLES,state->signage_start[room_index],
        state->signage_count[room_index]);
    // State-dependent text is refreshed only after simulation transitions.
    if(state->status_count>0){
        glBindBuffer(GL_ARRAY_BUFFER,state->status_vbo);
        glVertexAttribPointer(static_cast<GLuint>(state->position_location),
            3,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void *>(0));
        glVertexAttribPointer(static_cast<GLuint>(state->uv_location),
            2,GL_FLOAT,GL_FALSE,stride,
            reinterpret_cast<const void *>(3u*sizeof(float)));
        glVertexAttribPointer(static_cast<GLuint>(state->color_attribute_location),
            3,GL_FLOAT,GL_FALSE,stride,
            reinterpret_cast<const void *>(5u*sizeof(float)));
        glVertexAttribPointer(static_cast<GLuint>(state->tile_origin_location),
            2,GL_FLOAT,GL_FALSE,stride,
            reinterpret_cast<const void *>(8u*sizeof(float)));
        glDrawArrays(GL_TRIANGLES,0,state->status_count);
    }


    // Render the actual source artwork on each of the 24 physical stock
    // cards; the ray-selection geometry remains unchanged.
    if(room_index==1 && state->original_icon_vbo!=0 &&
       state->original_icon_count>0) {
        const GLboolean blended=glIsEnabled(GL_BLEND);
        GLint src_rgb=GL_ONE,dst_rgb=GL_ZERO;
        GLint src_alpha=GL_ONE,dst_alpha=GL_ZERO;
        glGetIntegerv(GL_BLEND_SRC_RGB,&src_rgb);
        glGetIntegerv(GL_BLEND_DST_RGB,&dst_rgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA,&src_alpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA,&dst_alpha);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glUniform1f(state->icon_mode_location,1.f);
        glUniform1f(state->flicker_location,1.f);
        glBindBuffer(GL_ARRAY_BUFFER,state->original_icon_vbo);
        glVertexAttribPointer(static_cast<GLuint>(state->position_location),
            3,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void *>(0));
        glVertexAttribPointer(static_cast<GLuint>(state->uv_location),
            2,GL_FLOAT,GL_FALSE,stride,
            reinterpret_cast<const void *>(3u*sizeof(float)));
        glVertexAttribPointer(static_cast<GLuint>(state->color_attribute_location),
            3,GL_FLOAT,GL_FALSE,stride,
            reinterpret_cast<const void *>(5u*sizeof(float)));
        glVertexAttribPointer(static_cast<GLuint>(state->tile_origin_location),
            2,GL_FLOAT,GL_FALSE,stride,
            reinterpret_cast<const void *>(8u*sizeof(float)));
        for(int slot=0;slot<stock::kCells;++slot) {
            const stock::Cell cell=stock::At(slot);
            const int source_icon=cell.faction*stock::kGroups+cell.group;
            const GLuint texture=state->original_icons[source_icon];
            if(texture==0)continue;
            glBindTexture(GL_TEXTURE_2D,texture);
            glDrawArrays(GL_TRIANGLES,slot*6,6);
        }
        glUniform1f(state->icon_mode_location,0.f);
        glBindTexture(GL_TEXTURE_2D,state->atlas_texture);
        glBlendFuncSeparate(src_rgb,dst_rgb,src_alpha,dst_alpha);
        if(blended==GL_FALSE)glDisable(GL_BLEND);
    }



    // The physical stockroom display follows the selected purchase card.
    // In the bar, selecting a branded drink or snack also displays its actual
    // packaged model on the counter. House lager has no source-packaged mesh.
    int displayed_offer=-1;
    if(room_index==static_cast<int>(trade::Room::Stockroom))
        displayed_offer=state->market.offer();
    else if(room_index==static_cast<int>(trade::Room::Bar)) {
        if(state->market.drink()>0)
            displayed_offer=state->market.drink();
        else if(state->market.snack()>=0)
            displayed_offer=trade::kDrinks+state->market.snack();
    }
    const int product_index=products::IndexForOffer(displayed_offer);
    if(product_index>=0) {
        const ProductGlResource &resource=
            state->product_resources[product_index];
        if(resource.vbo!=0 && resource.texture!=0 &&
           resource.vertex_count>0) {
            const GLboolean blended=glIsEnabled(GL_BLEND);
            GLint rgb_src=GL_ONE,rgb_dst=GL_ZERO;
            GLint alpha_src=GL_ONE,alpha_dst=GL_ZERO;
            glGetIntegerv(GL_BLEND_SRC_RGB,&rgb_src);
            glGetIntegerv(GL_BLEND_DST_RGB,&rgb_dst);
            glGetIntegerv(GL_BLEND_SRC_ALPHA,&alpha_src);
            glGetIntegerv(GL_BLEND_DST_ALPHA,&alpha_dst);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            glBindTexture(GL_TEXTURE_2D,resource.texture);
            glUniform1f(state->icon_mode_location,1.f);
            glUniform1f(state->flicker_location,1.f);
            glBindBuffer(GL_ARRAY_BUFFER,resource.vbo);
            glVertexAttribPointer(static_cast<GLuint>(state->position_location),
                3,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void *>(0));
            glVertexAttribPointer(static_cast<GLuint>(state->uv_location),
                2,GL_FLOAT,GL_FALSE,stride,
                reinterpret_cast<const void *>(3u*sizeof(float)));
            glVertexAttribPointer(static_cast<GLuint>(state->color_attribute_location),
                3,GL_FLOAT,GL_FALSE,stride,
                reinterpret_cast<const void *>(5u*sizeof(float)));
            glVertexAttribPointer(static_cast<GLuint>(state->tile_origin_location),
                2,GL_FLOAT,GL_FALSE,stride,
                reinterpret_cast<const void *>(8u*sizeof(float)));
            glDrawArrays(GL_TRIANGLES,0,resource.vertex_count);
            glBindTexture(GL_TEXTURE_2D,state->atlas_texture);
            glUniform1f(state->icon_mode_location,0.f);
            glBlendFuncSeparate(rgb_src,rgb_dst,alpha_src,alpha_dst);
            if(blended==GL_FALSE)glDisable(GL_BLEND);
        }
    }

    // Dynamic props still use the unit-cube VBO and constant attributes.
    glDisableVertexAttribArray(
        static_cast<GLuint>(state->color_attribute_location)
    );
    glDisableVertexAttribArray(
        static_cast<GLuint>(state->tile_origin_location)
    );
    glBindBuffer(GL_ARRAY_BUFFER, state->cube_vbo);
    glVertexAttribPointer(
        static_cast<GLuint>(state->position_location),
        3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
        reinterpret_cast<const void *>(0)
    );
    glVertexAttribPointer(
        static_cast<GLuint>(state->uv_location),
        2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
        reinterpret_cast<const void *>(3u * sizeof(float))
    );
    state->active_material = -1;
}

void DrawRoom(
    ModuleState *state,
    const float *view_projection,
    float flicker
) {
    DrawStaticRoom(state, view_projection, flicker);
    if (state->market.room()==trade::Room::Bar) {
        // Actual purchased improvements alter the furnished 3D room.
        // Nothing here is claimed to be exported Unreal artwork.
        if(state->renovations.tier(property::Walls)>0)
            DrawCube(state,view_projection,flicker,
                0.f,0.17f,-2.91f,3.90f,0.12f,0.09f,
                .53f,.35f,.22f,Material::Wood);
        if(state->renovations.tier(property::Floor)>0)
            DrawCube(state,view_projection,flicker,
                0.f,-1.07f,-2.15f,3.55f,.07f,.12f,
                .37f,.31f,.24f,Material::Wood);
        if(state->renovations.tier(property::Ceiling)>0)
            DrawCube(state,view_projection,flicker,
                .38f,.90f,-1.85f,.54f,.09f,.42f,
                .78f,.69f,.45f,Material::Metal);
        for(int i=0;i<state->renovations.tier(property::Seating);++i)
            DrawCube(state,view_projection,flicker,
                1.0f+static_cast<float>(i%3)*.34f,
                -.75f,-1.70f-static_cast<float>(i/3)*.42f,
                .23f,.27f,.25f,.42f,.28f,.20f,Material::Wood);
    }
    if (state->market.room()!=trade::Room::Bar) {
        const bool stock=state->market.room()==trade::Room::Stockroom;
        // Navigation is a real room transition with room-specific models.
        DrawCube(state,view_projection,flicker,
            -2.47f,-0.20f,-2.94f,0.70f,1.52f,0.07f,
            state->hovered_target==WorkTarget::ToBar?0.60f:0.32f,
            0.28f,0.19f,Material::Wood);
        if (stock) {
            // Baked card backgrounds and lettering cost one draw/eye.
            // Only the hovered selection border is dynamic.
            if(state->hovered_target==WorkTarget::StockSku &&
               state->hovered_offer>=1) {
                for(int slot=0;slot<stock::kCells;++slot) {
                    const stock::Cell cell=stock::At(slot);
                    if(cell.offer!=state->hovered_offer)continue;
                    DrawCube(state,view_projection,1.0f,
                        cell.x,cell.y+stock::kHalfHeight,cell.z+0.045f,
                        stock::kHalfWidth*2.1f,0.022f,0.018f,
                        0.90f,0.72f,0.26f,Material::Metal);
                    DrawCube(state,view_projection,1.0f,
                        cell.x,cell.y-stock::kHalfHeight,cell.z+0.045f,
                        stock::kHalfWidth*2.1f,0.022f,0.018f,
                        0.90f,0.72f,0.26f,Material::Metal);
                    break;
                }
            }
            DrawCube(state,view_projection,flicker,
                -0.70f,-0.54f,-1.02f,0.53f,0.19f,0.48f,
                state->hovered_target==WorkTarget::StockNext?0.70f:0.25f,
                0.37f,0.17f,Material::Wood);
            DrawCube(state,view_projection,flicker,
                0.40f,-0.54f,-1.02f,0.53f,0.19f,0.48f,
                state->hovered_target==WorkTarget::StockBuy?0.64f:0.20f,
                0.45f,0.24f,Material::Metal);
            // Real purchasable legacy cartons: eight-lager case, four-cup kit.
            DrawCube(state,view_projection,flicker,
                1.20f,-0.48f,-0.72f,0.40f,0.20f,0.24f,
                state->hovered_target==WorkTarget::BeerOrder?0.67f:0.36f,
                0.30f,0.17f,Material::Wood);
            DrawCube(state,view_projection,flicker,
                1.68f,-0.48f,-0.72f,0.40f,0.20f,0.24f,
                state->hovered_target==WorkTarget::CupOrder?0.76f:0.38f,
                0.56f,0.26f,Material::Paper);
            // Storage cases and distinguishable casks, not floating cards.
            for(int i=0;i<3;++i) {
                DrawCube(state,view_projection,flicker,
                    -1.93f+static_cast<float>(i)*0.42f,
                    -0.52f,-2.28f,0.32f,0.52f,0.34f,
                    0.34f,0.25f,0.15f,Material::Wood);
                DrawCube(state,view_projection,flicker,
                    -1.93f+static_cast<float>(i)*0.42f,
                    -0.25f,-2.28f,0.34f,0.08f,0.36f,
                    0.50f,0.43f,0.27f,Material::Metal);
            }
        } else {
            DrawCube(state,view_projection,flicker,
                2.47f,-0.20f,-2.94f,0.70f,1.52f,0.07f,
                state->hovered_target==WorkTarget::ToStock?0.62f:0.31f,
                0.32f,0.21f,Material::Wood);
            // Physical writing surface: selecting it opens a ledger view.
            DrawCube(state,view_projection,flicker,
                0.85f,-0.50f,-0.90f,0.52f,0.08f,0.37f,
                state->hovered_target==WorkTarget::Ledger?0.85f:0.59f,
                0.57f,0.42f,Material::Paper);
            DrawCube(state,view_projection,flicker,
                -1.32f,-0.49f,-1.10f,0.52f,0.18f,0.39f,
                state->hovered_target==WorkTarget::UpgradeNext?.78f:.42f,
                .45f,.25f,Material::Wood);
            DrawCube(state,view_projection,flicker,
                -0.52f,-0.49f,-1.10f,0.52f,0.18f,0.39f,
                state->hovered_target==WorkTarget::UpgradeBuy?.80f:.35f,
                .61f,.25f,Material::Metal);
            DrawCube(state,view_projection,flicker,
                1.35f,-0.95f,-1.74f,1.53f,0.08f,1.16f,
                state->hovered_target==WorkTarget::Bed?0.49f:0.28f,
                0.36f,0.30f,Material::Paper);
        }
        return;
    }
    // Painted physical doors, matching their controller-ray selection boxes.
    DrawCube(state,view_projection,flicker,
        2.53f,-0.20f,-2.94f,0.68f,1.55f,0.07f,
        state->hovered_target==WorkTarget::DoorStock?0.63f:0.32f,
        0.27f,0.16f,Material::Wood);
    DrawCube(state,view_projection,flicker,
        0.60f,-0.20f,-2.94f,0.68f,1.55f,0.07f,
        state->hovered_target==WorkTarget::DoorLiving?0.64f:0.28f,
        0.34f,0.23f,Material::Wood);
    DrawCube(state,view_projection,flicker,
        1.25f,-0.53f,-1.10f,0.32f,0.11f,0.40f,
        state->hovered_target==WorkTarget::DrinkChoice?0.81f:0.30f,
        0.49f,0.26f,Material::Wood);
    DrawCube(state,view_projection,flicker,
        1.73f,-0.53f,-1.10f,0.29f,0.11f,0.40f,
        state->hovered_target==WorkTarget::SnackChoice?0.76f:0.37f,
        0.43f,0.16f,Material::Wood);
    const bool tap_hover =
        state->hovered_target
            == WorkTarget::Tap;

    DrawCube(
        state,
        view_projection,
        flicker,
        -0.85f,-0.45f,-0.98f,
        0.18f,0.40f,0.18f,
        tap_hover ? 0.76f : 0.36f,
        tap_hover ? 0.62f : 0.34f,
        tap_hover ? 0.24f : 0.30f,
        Material::Metal
    );
    DrawCube(
        state,
        view_projection,
        flicker,
        -0.85f,-0.20f,-0.98f,
        0.34f,0.12f,0.22f,
        tap_hover ? 0.82f : 0.25f,
        tap_hover ? 0.70f : 0.24f,
        tap_hover ? 0.28f : 0.22f,
        Material::Metal
    );

    DrawCube(state,view_projection,flicker,
        -0.85f,-0.28f,-0.79f,0.075f,0.075f,0.32f,
        0.72f,0.67f,0.50f,Material::Metal);
    DrawCube(state,view_projection,flicker,
        -0.85f,-0.10f,-0.98f,0.10f,0.14f,0.10f,
        0.73f,0.28f,0.18f,Material::Wood);

    const bool wash_hover =
        state->hovered_target
            == WorkTarget::WashStation;

    DrawCube(
        state,
        view_projection,
        flicker,
        -0.28f,-0.50f,-1.18f,
        0.92f,0.12f,0.48f,
        wash_hover ? 0.30f : 0.18f,
        wash_hover ? 0.58f : 0.30f,
        wash_hover ? 0.72f : 0.34f,
        Material::Metal
    );

    DrawCube(state,view_projection,flicker,
        -0.28f,-0.425f,-1.18f,0.74f,0.012f,0.30f,
        0.045f,0.11f,0.13f,Material::Metal);
    if (state->simulation.current_event()
            == EventType::ProtectionDemand
        && !state->simulation
            .protection_paid()) {
        const bool protection_hover =
            state->hovered_target
                == WorkTarget::ProtectionEnvelope;

        DrawCube(
            state,
            view_projection,
            flicker,
            -1.58f,-0.48f,-0.72f,
            0.44f,0.05f,0.24f,
            protection_hover ? 0.86f : 0.50f,
            protection_hover ? 0.30f : 0.14f,
            protection_hover ? 0.24f : 0.12f,
            Material::Paper
        );
    }

    const bool cup_stack_hover =
        state->hovered_target
            == WorkTarget::CupStack;

    DrawCube(
        state,
        view_projection,
        flicker,
        0.45f,-0.61f,-0.70f,
        0.82f,0.06f,0.34f,
        cup_stack_hover ? 0.58f : 0.24f,
        cup_stack_hover ? 0.56f : 0.23f,
        cup_stack_hover ? 0.48f : 0.20f,
        Material::Wood
    );

    const int clean =
        std::min(
            3,
            state->simulation.clean_cups()
        );

    for (int index = 0;
         index < clean;
         ++index) {
        DrawCup(
            state,
            view_projection,
            flicker,
            0.25f
                + static_cast<float>(
                    index
                ) * 0.22f,
            -0.50f,
            -0.70f,
            false,
            cup_stack_hover
        );
    }

    const int dirty =
        std::min(
            3,
            state->simulation.dirty_cups()
        );

    for (int index = 0;
         index < dirty;
         ++index) {
        DrawCup(
            state,
            view_projection,
            flicker,
            -0.05f
                - static_cast<float>(
                    index
                ) * 0.18f,
            -0.50f,
            -1.18f,
            false,
            wash_hover
        );
    }

    if (state->simulation.current_patron()
        != nullptr) {
        const bool patron_hover =
            state->hovered_target
                == WorkTarget::Patron;
        // Uniform silhouette vocabulary, distinct faction clothing colors.
        // The four archetypes differ without external skins or extra shaders.
        static constexpr float kFactionCloth[trade::kFactions][3] = {
            {0.27f,0.43f,0.60f}, // 101 Concordant blue
            {0.64f,0.56f,0.22f}, // 202 UED yellow
            {0.55f,0.29f,0.25f}, // 303 Brewers Union red
            {0.39f,0.29f,0.19f}, // 404 Salihorsk brown
            {0.64f,0.39f,0.18f}, // 505 Mozoli orange
            {0.26f,0.43f,0.28f}  // 606 Mazr green
        };
        const int faction=trade::FactionFor(
            state->simulation.current_patron()->kind);
        const float body_r=std::min(1.0f,kFactionCloth[faction][0]+
            (patron_hover?0.16f:0.0f));
        const float body_g=std::min(1.0f,kFactionCloth[faction][1]+
            (patron_hover?0.16f:0.0f));
        const float body_b=std::min(1.0f,kFactionCloth[faction][2]+
            (patron_hover?0.16f:0.0f));

        DrawCube(
            state,
            view_projection,
            flicker,
            -1.30f,-0.52f,-2.05f,
            0.52f,1.05f,0.36f,
            body_r,body_g,body_b
        );
        DrawCube(
            state,
            view_projection,
            flicker,
            -1.30f,0.18f,-2.05f,
            0.34f,0.34f,0.34f,
            patron_hover ? 0.70f : 0.52f,
            patron_hover ? 0.58f : 0.43f,
            patron_hover ? 0.44f : 0.34f
        );
        // Broad shoulders, arms, hair and face details establish a
        // readable customer silhouette at seated VR distance.
        DrawCube(state,view_projection,flicker,
            -1.30f,-0.06f,-2.05f,0.66f,0.16f,0.40f,
            body_r,body_g,body_b,Material::Wood);
        DrawCube(state,view_projection,flicker,
            -1.62f,-0.46f,-2.03f,0.16f,0.72f,0.23f,
            body_r,body_g,body_b,Material::Wood);
        DrawCube(state,view_projection,flicker,
            -0.98f,-0.46f,-2.03f,0.16f,0.72f,0.23f,
            body_r,body_g,body_b,Material::Wood);
        DrawCube(state,view_projection,flicker,
            -1.30f,0.355f,-2.05f,0.37f,0.10f,0.34f,
            0.16f,0.13f,0.12f,Material::Wood);
        // A faction-colored chest badge and short hat brim make the
        // customer identifiable without relying on color alone.
        DrawCube(state,view_projection,flicker,
            -1.48f,-0.23f,-1.86f,0.10f,0.16f,0.025f,
            0.83f,0.75f,0.45f,Material::Metal);
        DrawCube(state,view_projection,flicker,
            -1.30f,0.40f,-1.90f,0.50f,0.055f,0.16f,
            body_r,body_g,body_b,Material::Wood);
        DrawCube(state,view_projection,1.0f,
            -1.40f,0.20f,-1.868f,0.044f,0.035f,0.015f,
            0.09f,0.08f,0.07f,Material::Metal);
        DrawCube(state,view_projection,1.0f,
            -1.20f,0.20f,-1.868f,0.044f,0.035f,0.015f,
            0.09f,0.08f,0.07f,Material::Metal);
    }

    if (state->simulation.pending_payment_cents()
        > 0) {
        const bool payment_hover =
            state->hovered_target
                == WorkTarget::Payment;

        DrawCube(
            state,
            view_projection,
            flicker,
            -0.98f,-0.47f,-0.72f,
            0.12f,0.04f,0.12f,
            payment_hover ? 0.95f : 0.72f,
            payment_hover ? 0.82f : 0.62f,
            payment_hover ? 0.28f : 0.16f,
            Material::Metal
        );
        DrawCube(
            state,
            view_projection,
            flicker,
            -0.82f,-0.47f,-0.72f,
            0.10f,0.04f,0.10f,
            payment_hover ? 0.94f : 0.68f,
            payment_hover ? 0.80f : 0.58f,
            payment_hover ? 0.26f : 0.14f,
            Material::Metal
        );
    }

    if (state->simulation.current_patron()
        != nullptr
        && state->simulation.pending_payment_cents()
            == 0
        && state->simulation.work_drink_state()
            == DrinkState::None) {
        const bool exit_hover =
            state->hovered_target
                == WorkTarget::ExitDoor;

        DrawCube(
            state,
            view_projection,
            flicker,
            -2.55f,-0.30f,-2.97f,
            0.58f,1.50f,0.05f,
            exit_hover ? 0.68f : 0.33f,
            exit_hover ? 0.28f : 0.18f,
            exit_hover ? 0.22f : 0.16f,
            Material::Wood
        );
    }

    if (state->pointer_active
        && state->simulation.work_drink_state()
            != DrinkState::None) {
        const float cup_x =
            state->pointer_origin[0]
            + state->pointer_direction[0]
                * kHeldCupDistance;
        const float cup_y =
            state->pointer_origin[1]
            + state->pointer_direction[1]
                * kHeldCupDistance;
        const float cup_z =
            state->pointer_origin[2]
            + state->pointer_direction[2]
                * kHeldCupDistance;

        DrawCup(
            state,
            view_projection,
            1.0f,
            cup_x,
            cup_y,
            cup_z,
            state->simulation.work_drink_state()
                == DrinkState::FilledCup,
            true
        );
    }

    if (state->pointer_active
        && state->hovered_target
            != WorkTarget::None
        && state->hovered_distance
            > 0.0f) {
        const float contact_x =
            state->pointer_origin[0]
            + state->pointer_direction[0]
                * state->hovered_distance;
        const float contact_y =
            state->pointer_origin[1]
            + state->pointer_direction[1]
                * state->hovered_distance;
        const float contact_z =
            state->pointer_origin[2]
            + state->pointer_direction[2]
                * state->hovered_distance;

        DrawCube(
            state,
            view_projection,
            1.0f,
            contact_x,
            contact_y,
            contact_z,
            0.055f,
            0.055f,
            0.055f,
            0.95f,
            0.90f,
            0.28f
        );
    }

    float marker_r = 0.28f;
    float marker_g = 0.36f;
    float marker_b = 0.30f;

    switch (
        state->simulation.current_event()
    ) {
        case EventType::ProtectionDemand:
            marker_r = 0.48f;
            marker_g = 0.18f;
            marker_b = 0.16f;
            break;

        case EventType::Inspection:
            marker_r = 0.24f;
            marker_g = 0.32f;
            marker_b = 0.52f;
            break;

        case EventType::SupplyInterruption:
            marker_r = 0.50f;
            marker_g = 0.38f;
            marker_b = 0.12f;
            break;

        case EventType::None:
        default:
            break;
    }

    DrawCube(
        state,
        view_projection,
        flicker,
        1.80f,0.45f,-2.98f,
        0.75f,0.46f,0.04f,
        marker_r,marker_g,marker_b,
        Material::Paper
    );

}

void *Create(
    const ReverieNativeHostV1 *host
) {
    if (!ReverieNativeHostSupportsSaveV1(
            host
        )) {
        return nullptr;
    }

    ModuleState *state =
        new (std::nothrow)
            ModuleState();

    if (state == nullptr) {
        return nullptr;
    }

    state->host = host;
    LoadState(state);

    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Red Ledger module created with manual cup/service/payment loop."
    );
    return state;
}

void Destroy(
    void *instance
) {
    ModuleState *state =
        static_cast<ModuleState *>(
            instance
        );

    if (state == nullptr) {
        return;
    }

    if (state->program != 0
        || state->cube_vbo != 0
        || state->mug_vbo != 0
        || state->static_vbo != 0
        || state->signage_vbo != 0
        || state->status_vbo != 0
        || state->atlas_texture != 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_WARN,
            "Module destroyed after GL context release was skipped."
        );
        AbandonGl(state);
    }

    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Red Ledger module destroyed."
    );
    delete state;
}

int32_t OnGlContextCreated(
    void *instance
) {
    ModuleState *state =
        static_cast<ModuleState *>(
            instance
        );

    return state != nullptr
        ? InitializeGl(state)
        : 0;
}

void ReleaseGlContext(
    void *instance
) {
    ModuleState *state =
        static_cast<ModuleState *>(
            instance
        );

    if (state == nullptr) {
        return;
    }

    DestroyGl(state);
    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Red Ledger GL resources released."
    );
}

void Resume(
    void *instance
) {
    Log(
        static_cast<ModuleState *>(
            instance
        ),
        REVERIE_NATIVE_LOG_INFO,
        "Red Ledger module resumed."
    );
}

void Pause(
    void *instance
) {
    Log(
        static_cast<ModuleState *>(
            instance
        ),
        REVERIE_NATIVE_LOG_INFO,
        "Red Ledger module paused."
    );
}

void Update(
    void *instance,
    const ReverieNativeInputV1 *input
) {
    ModuleState *state =
        static_cast<ModuleState *>(
            instance
        );

    if (state == nullptr
        || !ReverieNativeInputHasPointerV1(
            input
        )) {
        return;
    }

    ReverieNativeInputV1 effective_input = *input;
    // Validate a writable copy even with an alternate ABI-compatible host.
    ReverieNativeSanitizeBaseInputV1(&effective_input);
    ReverieNativeSanitizePointerV1(&effective_input);

    const float dt = effective_input.delta_seconds;
    state->elapsed_seconds += dt;
    // Calculate lighting once per simulation update, not separately for
    // the two eyes. Both stereo views receive identical illumination.
    const float wave =
        std::sin(state->elapsed_seconds * 7.0f)
        + 0.35f * std::sin(state->elapsed_seconds * 19.0f);
    state->flicker = std::max(0.72f,
        std::min(1.0f, 0.90f + wave * 0.06f));

    state->pointer_active =
        effective_input.pointer_kind
            != REVERIE_NATIVE_POINTER_NONE;

    if (state->pointer_active) {
        for (int axis = 0;
             axis < 3;
             ++axis) {
            state->pointer_origin[axis] =
                effective_input.pointer_origin[axis];
            state->pointer_direction[axis] =
                effective_input.pointer_direction[axis];
        }
    }

    state->hovered_target =
        FindWorkTarget(
            state,
            &effective_input,
            &state->hovered_distance,
            &state->hovered_offer
        );

    const bool primary =
        effective_input.primary_down != 0u;

    if (primary
        && !state->primary_was_down) {
        if (state->hovered_target
            == WorkTarget::None) {
            Log(
                state,
                REVERIE_NATIVE_LOG_WARN,
                "Primary action had no reachable bar target."
            );
        } else {
            ActivateWorkTarget(
                state,
                state->hovered_target
            );
        }
    }

    state->primary_was_down =
        primary;
}

int32_t RenderEye(
    void *instance,
    const ReverieNativeEyeV1 *eye
) {
    ModuleState *state =
        static_cast<ModuleState *>(
            instance
        );

    if (state == nullptr
        || !ReverieNativeEyeRenderableV1(
            eye
        )
        || state->program == 0
        || state->cube_vbo == 0
        || state->mug_vbo == 0
        || state->static_vbo == 0
        || state->signage_vbo == 0
        || state->status_vbo == 0
        || state->atlas_texture == 0) {
        return 0;
    }

    ReverieNativeGlStateV1 gl_state = {};
    ReverieNativeGlAttribStateV1 position_state = {};
    ReverieNativeGlAttribStateV1 uv_state = {};
    ReverieNativeGlAttribStateV1 color_state = {};
    ReverieNativeGlAttribStateV1 tile_state = {};

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
    ReverieNativeGlAttribCaptureV1(
        &color_state,
        static_cast<GLuint>(
            state->color_attribute_location
        )
    );
    ReverieNativeGlAttribCaptureV1(
        &tile_state,
        static_cast<GLuint>(
            state->tile_origin_location
        )
    );

    float view_projection[16];
    ReverieNativeMat4Multiply(
        view_projection,
        eye->projection,
        eye->view
    );

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    glActiveTexture(GL_TEXTURE0);
    glUseProgram(
        state->program
    );
    glUniform1f(state->icon_mode_location,0.f);
    glBindTexture(GL_TEXTURE_2D, state->atlas_texture);
    glUniform1i(state->sampler_location, 0);
    state->active_material = -1;
    glBindBuffer(
        GL_ARRAY_BUFFER,
        state->cube_vbo
    );
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
        reinterpret_cast<
            const void *
        >(0)
    );
    glEnableVertexAttribArray(
        static_cast<GLuint>(state->uv_location)
    );
    glVertexAttribPointer(
        static_cast<GLuint>(state->uv_location),
        2, GL_FLOAT, GL_FALSE,
        5 * sizeof(float),
        reinterpret_cast<const void *>(3 * sizeof(float))
    );

    RefreshStatusBoard(state);
    const float flicker = state->flicker;

    DrawRoom(
        state,
        view_projection,
        flicker
    );

    ReverieNativeGlAttribRestoreV1(
        &tile_state
    );
    ReverieNativeGlAttribRestoreV1(
        &color_state
    );
    ReverieNativeGlAttribRestoreV1(
        &uv_state
    );
    ReverieNativeGlAttribRestoreV1(
        &position_state
    );
    ReverieNativeGlStateRestoreV1(
        &gl_state
    );

    return glGetError()
            == GL_NO_ERROR
        ? 1
        : 0;
}

const ReverieNativeModuleCapabilitiesV1 kCapabilities = {
    sizeof(ReverieNativeModuleCapabilitiesV1),
    REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION,
    0.70f,
    0.32f
};

const ReverieNativeModuleApiV1 kApi = {
    sizeof(
        ReverieNativeModuleApiV1
    ),
    REVERIE_NATIVE_MODULE_ABI_VERSION,
    {
        sizeof(
            ReverieNativeModuleDescriptorV1
        ),
        REVERIE_NATIVE_MODULE_ABI_VERSION,
        "between-deliveries-red-ledger",
        "Between Deliveries: The Red Ledger VR",
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
