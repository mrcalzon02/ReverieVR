#!/usr/bin/env python3
"""Verify ReverieVR native implementation/documentation synchronization.

This guard is intentionally dependency-free. It validates factual anchors in the
current tree and can also require mapped implementation + documentation files to
change together.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

API_HEADER = Path("app/src/main/jni/native/reverie_native_module.h")
SDK_HEADER = Path("app/src/main/jni/native/reverie_native_sdk.h")
GL_STATE_HEADER = Path("app/src/main/jni/native/reverie_native_gl_state.h")
MATH_HEADER = Path("app/src/main/jni/native/reverie_native_math.h")
GL_UTILS_HEADER = Path("app/src/main/jni/native/reverie_native_gl_utils.h")
HOST_CPP = Path("app/src/main/jni/native/reverie_native_host.cpp")
RUNTIME_JAVA = Path(
    "app/src/main/java/io/github/mrcalzon02/reverievr/NativeModuleRuntime.java"
)
TEST_CHAMBER_CPP = Path("app/src/main/jni/native/test_chamber_module.cpp")
RED_LEDGER_CPP = Path("app/src/main/jni/native/red_ledger_module.cpp")
ABI_CONTRACT_TEST = Path("app/src/test/native/native_abi_contract_test.cpp")
ABI_CONTRACT_RUNNER = Path("scripts/verify-native-abi-contract.py")
MATERIAL_H = Path("app/src/main/jni/native/procedural_material_atlas.h")
MATERIAL_CPP = Path("app/src/main/jni/native/procedural_material_atlas.cpp")
STATIC_H = Path("app/src/main/jni/native/red_ledger_static_geometry.h")
STATIC_CPP = Path("app/src/main/jni/native/red_ledger_static_geometry.cpp")
LOCOMOTION_JAVA = Path(
    "app/src/main/java/io/github/mrcalzon02/reverievr/BoundedViewRelativeLocomotion.java"
)
LOCOMOTION_GATE_JAVA = Path(
    "app/src/main/java/io/github/mrcalzon02/reverievr/TouchpadLocomotionGate.java"
)
RENDERER_JAVA = Path(
    "app/src/main/java/io/github/mrcalzon02/reverievr/VrShellRenderer.java"
)
CONTROLLER_GHOST_TETHER_JAVA = Path(
    "app/src/main/java/io/github/mrcalzon02/reverievr/ControllerGhostTether.java"
)
CONTROLLER_SPRING_WAVEFORM_JAVA = Path(
    "app/src/main/java/io/github/mrcalzon02/reverievr/ControllerSpringWaveform.java"
)
ACTIVITY_JAVA = Path(
    "app/src/main/java/io/github/mrcalzon02/reverievr/VrActivity.java"
)
UI_FEEDBACK_JAVA = Path(
    "app/src/main/java/io/github/mrcalzon02/reverievr/UiFeedback.java"
)

API_DOC = Path("docs/native/API_V1.md")
SDK_DOC = Path("docs/native/SDK_HELPERS.md")
GL_RENDERING_DOC = Path("docs/native/GL_RENDERING_STANDARD.md")
CORE_UTILITIES_DOC = Path("docs/native/CORE_UTILITIES.md")
RUNTIME_SERVICES_DOC = Path("docs/native/RUNTIME_SERVICES.md")
PROCEDURAL_DOC = Path("docs/native/PROCEDURAL_CONTENT_STANDARD.md")
SYSTEMS_CATALOG_DOC = Path("docs/native/SYSTEMS_CATALOG.md")
CONTRACT_INDEX = Path("docs/native/CONTRACT_INDEX.md")
BACKLOG = Path("docs/project/BACKLOG.md")

SYNC_MAP = {
    API_HEADER: {API_DOC},
    SDK_HEADER: {SDK_DOC},
    GL_STATE_HEADER: {GL_RENDERING_DOC},
    MATH_HEADER: {CORE_UTILITIES_DOC},
    GL_UTILS_HEADER: {CORE_UTILITIES_DOC},
    HOST_CPP: {API_DOC},
    RUNTIME_JAVA: {API_DOC},
    MATERIAL_H: {PROCEDURAL_DOC},
    MATERIAL_CPP: {PROCEDURAL_DOC},
    STATIC_H: {PROCEDURAL_DOC},
    STATIC_CPP: {PROCEDURAL_DOC},
    LOCOMOTION_JAVA: {RUNTIME_SERVICES_DOC},
    LOCOMOTION_GATE_JAVA: {RUNTIME_SERVICES_DOC},
}


class ContractError(RuntimeError):
    pass


def read(path: Path) -> str:
    full = ROOT / path
    if not full.is_file():
        raise ContractError(f"required file is missing: {path}")
    return full.read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def regex_value(pattern: str, text: str, label: str) -> str:
    match = re.search(pattern, text, flags=re.MULTILINE | re.DOTALL)
    if not match:
        raise ContractError(f"could not derive {label} from authoritative source")
    return match.group(1)


def check_current_content() -> None:
    header = read(API_HEADER)
    sdk_header = read(SDK_HEADER)
    gl_state_header = read(GL_STATE_HEADER)
    math_header = read(MATH_HEADER)
    gl_utils_header = read(GL_UTILS_HEADER)
    host = read(HOST_CPP)
    runtime = read(RUNTIME_JAVA)
    test_chamber_cpp = read(TEST_CHAMBER_CPP)
    red_ledger_cpp = read(RED_LEDGER_CPP)
    abi_contract_test = read(ABI_CONTRACT_TEST)
    abi_contract_runner = read(ABI_CONTRACT_RUNNER)
    material_h = read(MATERIAL_H)
    static_h = read(STATIC_H)
    locomotion_java = read(LOCOMOTION_JAVA)
    locomotion_gate_java = read(LOCOMOTION_GATE_JAVA)
    renderer_java = read(RENDERER_JAVA)
    controller_ghost_tether_java = read(
        CONTROLLER_GHOST_TETHER_JAVA
    )
    controller_spring_waveform_java = read(
        CONTROLLER_SPRING_WAVEFORM_JAVA
    )
    activity_java = read(ACTIVITY_JAVA)
    ui_feedback_java = read(UI_FEEDBACK_JAVA)
    api_doc = read(API_DOC)
    sdk_doc = read(SDK_DOC)
    gl_rendering_doc = read(GL_RENDERING_DOC)
    core_utilities_doc = read(CORE_UTILITIES_DOC)
    runtime_services_doc = read(RUNTIME_SERVICES_DOC)
    procedural_doc = read(PROCEDURAL_DOC)
    systems_catalog = read(SYSTEMS_CATALOG_DOC)
    contract_index = read(CONTRACT_INDEX)
    backlog = read(BACKLOG)

    abi_version = regex_value(
        r"^#define\s+REVERIE_NATIVE_MODULE_ABI_VERSION\s+(\d+)u\s*$",
        header,
        "native ABI version",
    )
    entry_symbol = regex_value(
        r'^#define\s+REVERIE_NATIVE_MODULE_ENTRY_SYMBOL\s+"([^"]+)"\s*$',
        header,
        "native entry symbol",
    )
    save_max = regex_value(
        r"^#define\s+REVERIE_NATIVE_SAVE_MAX_BYTES\s+(\d+)u\s*$",
        header,
        "maximum save bytes",
    )

    require(
        f"REVERIE_NATIVE_MODULE_ABI_VERSION == {abi_version}" in api_doc,
        "API_V1.md does not report the current ABI version",
    )
    require(
        f"`{entry_symbol}`" in api_doc,
        "API_V1.md does not report the current module entry symbol",
    )
    require(
        f"`{save_max}` bytes" in api_doc,
        "API_V1.md does not report the current save-size limit",
    )

    for token in (
        "REVERIE_NATIVE_POINTER_NONE",
        "REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER",
        "REVERIE_NATIVE_POINTER_VIRTUAL_CONTROLLER",
        "read_save",
        "write_save",
        "on_gl_context_created",
        "release_gl_context",
        "render_eye",
    ):
        require(token in header, f"authoritative ABI source lost expected token: {token}")
        require(
            f"`{token}`" in api_doc or token in api_doc,
            f"API_V1.md does not mention current ABI token: {token}",
        )


    prefix_tokens = (
        "REVERIE_NATIVE_HOST_V1_LOG_MIN_SIZE",
        "REVERIE_NATIVE_HOST_V1_SAVE_MIN_SIZE",
        "REVERIE_NATIVE_HOST_V1_FEEDBACK_MIN_SIZE",
        "REVERIE_NATIVE_DESCRIPTOR_V1_SIZE",
        "REVERIE_NATIVE_DESCRIPTOR_V1_MIN_SIZE",
        "REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE",
        "REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE",
        "REVERIE_NATIVE_EYE_V1_MIN_SIZE",
        "REVERIE_NATIVE_MODULE_API_V1_MIN_SIZE",
        "REVERIE_NATIVE_MODULE_API_V1_CAPABILITIES_MIN_SIZE",
        "REVERIE_NATIVE_CAPABILITIES_V1_LOCOMOTION_MIN_SIZE",
    )
    for token in prefix_tokens:
        require(token in header, f"ABI header lost minimum-prefix constant: {token}")
        require(
            f"`{token}`" in api_doc,
            f"API_V1.md does not document minimum-prefix constant: {token}",
        )


    require(
        "descriptor->struct_size\n            == REVERIE_NATIVE_DESCRIPTOR_V1_SIZE"
        in sdk_header,
        "SDK helper no longer enforces the frozen ABI-v1 descriptor size",
    )
    require(
        "layout-frozen" in api_doc
        and "tail of `ReverieNativeModuleApiV1`" in api_doc,
        "API_V1.md does not explain the embedded descriptor extension boundary",
    )

    helper_tokens = (
        "ReverieNativeHostSupportsLogV1",
        "ReverieNativeHostSupportsSaveV1",
        "ReverieNativeHostSupportsFeedbackV1",
        "ReverieNativeRequestFeedbackV1",
        "ReverieNativeDescriptorHasMandatoryV1",
        "ReverieNativeGlesRequirementSupportedV1",
        "ReverieNativeInputHasBaseV1",
        "ReverieNativeInputHasPointerV1",
        "ReverieNativeEyeHasMatricesV1",
        "ReverieNativeApiHasMandatoryV1",
        "ReverieNativeApiCapabilitiesV1",
        "ReverieNativeCapabilitiesHasShellLocomotionV1",
    )
    for token in helper_tokens:
        require(token in sdk_header, f"native SDK helper header lost function: {token}")
        require(
            f"`{token}`" in sdk_doc,
            f"SDK_HELPERS.md does not document current helper: {token}",
        )


    gl_tokens = (
        "ReverieNativeGlStateV1",
        "ReverieNativeGlStateCaptureV1",
        "ReverieNativeGlStateRestoreV1",
        "ReverieNativeGlAttribStateV1",
        "ReverieNativeGlAttribCaptureV1",
        "ReverieNativeGlAttribRestoreV1",
    )
    for token in gl_tokens:
        require(token in gl_state_header, f"native GL guard lost symbol: {token}")
        require(
            f"`{token}`" in gl_rendering_doc,
            f"GL_RENDERING_STANDARD.md does not document current GL symbol: {token}",
        )


    core_utilities = (
        (
            "ReverieNativeMat4Multiply",
            math_header,
            "matrix utility",
        ),
        (
            "ReverieNativeCompileShader",
            gl_utils_header,
            "shader utility",
        ),
    )
    for token, source, label in core_utilities:
        require(token in source, f"native {label} header lost function: {token}")
        require(
            f"`{token}`" in core_utilities_doc,
            f"CORE_UTILITIES.md does not document current utility: {token}",
        )
        for module_source, module_label in (
            (test_chamber_cpp, "test chamber"),
            (red_ledger_cpp, "Red Ledger"),
        ):
            require(
                token in module_source,
                f"{module_label} no longer consumes shared {label}: {token}",
            )

    for module_source, module_label in (
        (test_chamber_cpp, "test chamber"),
        (red_ledger_cpp, "Red Ledger"),
    ):
        require(
            "GLuint CompileShader(" not in module_source,
            f"{module_label} regressed to a private shader compiler",
        )
        require(
            "void MultiplyMatrix(" not in module_source,
            f"{module_label} regressed to a private matrix multiply",
        )

    for source, label, expected_attribs in (
        (test_chamber_cpp, "test chamber", 2),
        (red_ledger_cpp, "Red Ledger", 4),
    ):
        require(
            '#include "reverie_native_gl_state.h"' in source,
            f"{label} does not include the shared GL state guard",
        )
        require(
            source.count("ReverieNativeGlStateCaptureV1(") == 2
            and source.count("ReverieNativeGlStateRestoreV1(") == 2,
            f"{label} does not isolate shared GL state during initialization and rendering",
        )
        require(
            source.count("ReverieNativeGlAttribCaptureV1(") == expected_attribs
            and source.count("ReverieNativeGlAttribRestoreV1(") == expected_attribs,
            f"{label} does not preserve every expected vertex attribute",
        )
        require(
            "previous_program" not in source
            and "previous_array_buffer" not in source
            and "previous_active_texture" not in source
            and "previous_texture" not in source,
            f"{label} still contains the superseded partial handwritten GL snapshot",
        )

    require(
        "ReverieNativeApiHasMandatoryV1(api)" in host
        and "ReverieNativeDescriptorHasMandatoryV1(" in host
        and "ReverieNativeGlesRequirementSupportedV1(" in host,
        "native host no longer consumes the shared ABI validation helpers",
    )
    require(
        "2u,\n            0u" in host
        and "host support 2.0" in host,
        "native host GLES admission version changed without contract update",
    )

    consumer_contracts = (
        (
            test_chamber_cpp,
            "test chamber",
            (
                "ReverieNativeHostSupportsLogV1",
                "ReverieNativeInputHasBaseV1",
                "ReverieNativeEyeHasMatricesV1",
            ),
        ),
        (
            red_ledger_cpp,
            "Red Ledger",
            (
                "ReverieNativeHostSupportsLogV1",
                "ReverieNativeHostSupportsSaveV1",
                "ReverieNativeInputHasPointerV1",
                "ReverieNativeEyeHasMatricesV1",
            ),
        ),
    )
    for source, label, required_tokens in consumer_contracts:
        require(
            '#include "reverie_native_sdk.h"' in source,
            f"{label} does not include the shared native SDK helper header",
        )
        for token in required_tokens:
            require(token in source, f"{label} lost shared SDK helper use: {token}")

    forbidden_full_size = (
        "struct_size < sizeof(ReverieNativeHostV1)",
        "struct_size < sizeof(ReverieNativeInputV1)",
        "struct_size < sizeof(ReverieNativeEyeV1)",
        "struct_size < sizeof(ReverieNativeModuleApiV1)",
        "< sizeof(ReverieNativeModuleDescriptorV1)",
    )
    for source, label in (
        (host.replace("\n", " "), "native host"),
        (test_chamber_cpp.replace("\n", " "), "test chamber"),
        (red_ledger_cpp.replace("\n", " "), "Red Ledger"),
    ):
        compact = re.sub(r"\s+", " ", source)
        for pattern in forbidden_full_size:
            require(
                pattern not in compact,
                f"{label} regressed to newest-struct sizeof validation: {pattern}",
            )

    abi_test_tokens = (
        "REVERIE_NATIVE_DESCRIPTOR_V1_SIZE",
        "REVERIE_NATIVE_MODULE_API_V1_MIN_SIZE",
        "REVERIE_NATIVE_MODULE_API_V1_CAPABILITIES_MIN_SIZE",
        "ReverieNativeHostSupportsLogV1",
        "ReverieNativeHostSupportsSaveV1",
        "ReverieNativeHostSupportsFeedbackV1",
        "ReverieNativeRequestFeedbackV1",
        "REVERIE_NATIVE_FEEDBACK_ACTIVATION",
        "feedback helper must strip unknown flag bits",
        "ReverieNativeGlesRequirementSupportedV1",
        "ReverieNativeInputHasBaseV1",
        "ReverieNativeInputHasPointerV1",
        "ReverieNativeEyeHasMatricesV1",
        "ReverieNativeApiCapabilitiesV1",
        "ReverieNativeCapabilitiesHasShellLocomotionV1",
        "old mandatory v1 API prefix must remain valid",
        "old v1 API prefix must not expose unseen capability tail",
    )
    for token in abi_test_tokens:
        require(
            token in abi_contract_test,
            f"native ABI contract test lost regression anchor: {token}",
        )
    require(
        "c++" in abi_contract_runner
        and "g++" in abi_contract_runner
        and "clang++" in abi_contract_runner
        and "native_abi_contract_test.cpp" in abi_contract_runner,
        "native ABI contract runner no longer compiles the canonical C++ test",
    )
    require(
        str(ABI_CONTRACT_TEST) in contract_index
        and str(ABI_CONTRACT_RUNNER) in contract_index,
        "contract index does not name the executable ABI regression gate",
    )

    require(
        "HostRequestFeedback" in host
        and "pending_feedback_flags" in host
        and "std::memory_order_relaxed" in host
        and "JNIEXPORT jint JNICALL" in host,
        "native host lost coalesced feedback queue/update return path",
    )
    for token in (
        "REVERIE_NATIVE_FEEDBACK_FOCUS",
        "REVERIE_NATIVE_FEEDBACK_ACTIVATION",
        "REVERIE_NATIVE_FEEDBACK_FAILURE",
    ):
        require(token in header, f"ABI header lost feedback flag: {token}")
        require(
            f"`{token}`" in api_doc,
            f"API_V1.md does not document feedback flag: {token}",
        )
    require(
        "static final int FEEDBACK_FOCUS" in runtime
        and "static final int FEEDBACK_ACTIVATION" in runtime
        and "static final int FEEDBACK_FAILURE" in runtime
        and "private static native int nativeUpdate(" in runtime,
        "NativeModuleRuntime feedback mask/JNI return contract drifted",
    )
    require(
        "onNativeModuleFeedbackRequested" in renderer_java
        and "nativeModuleRuntime.update(" in renderer_java,
        "renderer no longer forwards native feedback masks",
    )
    require(
        "onNativeModuleFeedbackRequested" in activity_java
        and "uiFeedback.focus()" in activity_java
        and "uiFeedback.activation()" in activity_java
        and "uiFeedback.failure()" in activity_java,
        "activity no longer maps native feedback to implemented shell cues",
    )
    for token in ("void focus()", "void activation()", "void failure()"):
        require(
            token in ui_feedback_java,
            f"UiFeedback lost implemented native-cue target: {token}",
        )
    require(
        "Haptics are also not implemented" in runtime_services_doc
        and "not an arbitrary audio submission/mixing API" in api_doc,
        "native feedback documentation overstates audio/haptics capability",
    )

    for token in ("HostReadSave", "HostWriteSave", "IsSafeSaveSlot"):
        require(token in host, f"native host lost expected service implementation: {token}")

    require(
        "value.length() > 80" in runtime,
        "NativeModuleRuntime module-id length policy changed; update verifier/docs",
    )
    require(
        "maximum length of 80 characters" in api_doc,
        "API_V1.md does not report the current module-id length policy",
    )

    tile_size = int(
        regex_value(
            r"constexpr\s+int\s+kTileSize\s*=\s*(\d+)\s*;",
            material_h,
            "procedural material tile size",
        )
    )
    static_cube_count = int(
        regex_value(
            r"constexpr\s+size_t\s+kStaticCubeCount\s*=\s*(\d+)u\s*;",
            static_h,
            "static cube count",
        )
    )
    cube_vertex_count = int(
        regex_value(
            r"constexpr\s+size_t\s+kCubeVertexCount\s*=\s*(\d+)u\s*;",
            static_h,
            "cube vertex count",
        )
    )
    stride = int(
        regex_value(
            r"constexpr\s+size_t\s+kStaticVertexStride\s*=\s*(\d+)u\s*;",
            static_h,
            "static vertex stride",
        )
    )

    atlas_side = tile_size * 2
    rgb565_bytes = atlas_side * atlas_side * 2
    static_bytes = static_cube_count * cube_vertex_count * stride * 4

    require(
        f"{atlas_side}x{atlas_side}" in procedural_doc,
        "procedural standard does not report the current atlas dimensions",
    )
    require(
        f"{rgb565_bytes // 1024} KiB" in procedural_doc,
        "procedural standard does not report the current RGB565 atlas footprint",
    )
    require(
        f"{static_cube_count} static" in procedural_doc,
        "procedural standard does not report the current static cube count",
    )
    require(
        f"{static_bytes:,}-byte" in procedural_doc,
        "procedural standard does not report the current static VBO size",
    )
    for token in (
        "GenerateMaterialAtlas",
        "GenerateMaterialAtlasRgb565",
        "BuildStaticRoomVertices",
    ):
        source = material_h if "MaterialAtlas" in token else static_h
        require(token in source, f"authoritative utility source lost expected function: {token}")
        require(
            f"`{token}`" in procedural_doc,
            f"procedural standard does not name current utility: {token}",
        )


    deadzone = float(
        regex_value(
            r"private\s+static\s+final\s+float\s+DEADZONE\s*=\s*([0-9.]+)f\s*;",
            locomotion_java,
            "native locomotion deadzone",
        )
    )
    speed = float(
        regex_value(
            r"private\s+static\s+final\s+float\s+SPEED_METERS_PER_SECOND\s*=\s*([0-9.]+)f\s*;",
            locomotion_java,
            "native locomotion speed",
        )
    )
    frame_cap_seconds = float(
        regex_value(
            r"Math\.min\(deltaSeconds,\s*([0-9.]+)f\)",
            locomotion_java,
            "native locomotion frame-delta cap",
        )
    )
    stale_nanos = int(
        regex_value(
            r"poseAge\s*>\s*(\d+)L",
            renderer_java,
            "native locomotion stale-input threshold",
        )
    )
    red_capability = re.search(
        r"const\s+ReverieNativeModuleCapabilitiesV1\s+kCapabilities\s*=\s*\{"
        r".*?REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION\s*,"
        r"\s*([0-9.]+)f\s*,\s*([0-9.]+)f",
        red_ledger_cpp,
        flags=re.MULTILINE | re.DOTALL,
    )
    chamber_capability = re.search(
        r"const\s+ReverieNativeModuleCapabilitiesV1\s+kCapabilities\s*=\s*\{"
        r".*?REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION\s*,"
        r"\s*([0-9.]+)f\s*,\s*([0-9.]+)f",
        test_chamber_cpp,
        flags=re.MULTILINE | re.DOTALL,
    )
    require(red_capability is not None, "Red Ledger lost shell-locomotion capability")
    require(chamber_capability is not None, "Test Chamber lost shell-locomotion capability")
    red_limit_x = float(red_capability.group(1))
    red_limit_z = float(red_capability.group(2))
    chamber_limit_x = float(chamber_capability.group(1))
    chamber_limit_z = float(chamber_capability.group(2))

    require(
        "nativeGetShellLocomotionBounds" in host
        and "ReverieNativeApiCapabilitiesV1" in host
        and "ReverieNativeCapabilitiesHasShellLocomotionV1" in host,
        "native host no longer validates/exposes module locomotion capabilities",
    )
    require(
        "nativeGetShellLocomotionBounds" in runtime
        and "copyShellLocomotionBounds" in runtime,
        "NativeModuleRuntime no longer caches/exposes shell locomotion capability",
    )
    require(
        "copyShellLocomotionBounds" in renderer_java,
        "renderer no longer consumes cached module locomotion capability",
    )
    require(
        "limitX = 0.70f" not in renderer_java
        and "limitX = 1.55f" not in renderer_java
        and '"procedural-test-chamber".equals(moduleId)' not in renderer_java,
        "renderer regressed to hard-coded module-id locomotion policy",
    )

    require(
        "REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION" in header
        and "`REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION`" in api_doc
        and "`REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION`" in runtime_services_doc,
        "shell locomotion capability is not synchronized across header/API/runtime docs",
    )
    require(
        f"**{deadzone:.2f}**" in runtime_services_doc,
        "runtime services doc does not report current locomotion deadzone",
    )
    require(
        f"**{speed:.2f} m/s**" in runtime_services_doc,
        "runtime services doc does not report current locomotion speed",
    )
    require(
        f"**{int(round(frame_cap_seconds * 1000.0))} ms**" in runtime_services_doc,
        "runtime services doc does not report current frame-delta cap",
    )
    require(
        f"**{int(round(stale_nanos / 1_000_000.0))} ms**" in runtime_services_doc,
        "runtime services doc does not report current stale-input cutoff",
    )
    require(
        f"**±{chamber_limit_x:.2f} m X/Z**" in runtime_services_doc
        and abs(chamber_limit_x - chamber_limit_z) < 0.0001,
        "runtime services doc does not report current Test Chamber bounds",
    )
    require(
        f"**±{red_limit_x:.2f} m lateral, ±{red_limit_z:.2f} m depth**"
        in runtime_services_doc,
        "runtime services doc does not report current Red Ledger bounds",
    )
    require(
        "TouchpadLocomotionGate" in locomotion_gate_java
        and "`TouchpadLocomotionGate`" in runtime_services_doc,
        "runtime services doc does not name the current locomotion gate",
    )
    for token in ("blocked || clicked", "!touching", "armed = true"):
        require(
            token in locomotion_gate_java,
            f"locomotion gate semantics changed near: {token}",
        )
    require(
        "current packed path is RV-0613" in backlog
        and "live Red Ledger upload path is direct RGB565" in backlog,
        "backlog Red Ledger atlas record is stale relative to the current packed path",
    )
    require(
        str(RENDERER_JAVA) in contract_index,
        "contract index missing renderer semantic-review source",
    )
    require(
        str(SYSTEMS_CATALOG_DOC) in contract_index,
        "contract index missing living systems catalog",
    )
    require(
        str(RENDERER_JAVA) in systems_catalog,
        "systems catalog missing renderer integration source",
    )
    for mapped_source, mapped_docs in SYNC_MAP.items():
        require(
            str(mapped_source) in systems_catalog,
            f"systems catalog missing standardized source: {mapped_source}",
        )
        for mapped_doc in mapped_docs:
            require(
                str(mapped_doc) in systems_catalog,
                f"systems catalog missing standardized documentation: {mapped_doc}",
            )

    for source_path, label in (
        (CONTROLLER_GHOST_TETHER_JAVA, "controller ghost tether"),
        (CONTROLLER_SPRING_WAVEFORM_JAVA, "controller spring waveform"),
    ):
        require(
            str(source_path) in systems_catalog,
            f"systems catalog missing shell-only reference: {label}",
        )

    ghost_vertices = int(
        regex_value(
            r"static\s+final\s+int\s+VERTEX_COUNT\s*=\s*(\d+)\s*;",
            controller_ghost_tether_java,
            "controller ghost tether vertex count",
        )
    )
    spring_rate = int(
        regex_value(
            r"static\s+final\s+int\s+SAMPLE_RATE\s*=\s*(\d+)\s*;",
            controller_spring_waveform_java,
            "controller spring waveform sample rate",
        )
    )
    spring_duration = int(
        regex_value(
            r"static\s+final\s+int\s+DURATION_MILLIS\s*=\s*(\d+)\s*;",
            controller_spring_waveform_java,
            "controller spring waveform duration",
        )
    )
    require(
        f"{ghost_vertices}-vertex" in systems_catalog,
        "systems catalog does not report current controller ghost tether size",
    )
    require(
        f"{spring_rate:,} Hz" in systems_catalog
        and f"{spring_duration} ms" in systems_catalog,
        "systems catalog does not report current generated spring waveform dimensions",
    )
    for planned_phrase in (
        "Arbitrary PCM/spatial audio | **Planned**",
        "Haptics | **Planned**",
        "Primitive-first model recipe direction | **Planned shared API**",
    ):
        require(
            planned_phrase in systems_catalog,
            f"systems catalog lost explicit planned maturity boundary: {planned_phrase}",
        )

    for mapped_source, mapped_docs in SYNC_MAP.items():
        require(str(mapped_source) in contract_index, f"contract index missing {mapped_source}")
        for mapped_doc in mapped_docs:
            require(str(mapped_doc) in contract_index, f"contract index missing {mapped_doc}")


def git_changed_files(args: argparse.Namespace) -> set[Path]:
    if args.staged:
        command = ["git", "diff", "--cached", "--name-only", "--diff-filter=ACMR"]
    elif args.base:
        command = ["git", "diff", "--name-only", "--diff-filter=ACMR", f"{args.base}...HEAD"]
    else:
        return set()

    try:
        output = subprocess.check_output(command, cwd=ROOT, text=True)
    except (OSError, subprocess.CalledProcessError) as exc:
        raise ContractError(f"could not inspect git changes: {exc}") from exc

    return {Path(line.strip()) for line in output.splitlines() if line.strip()}


def check_same_change(changed: set[Path]) -> None:
    if not changed:
        return

    failures: list[str] = []
    for source, docs in SYNC_MAP.items():
        if source not in changed:
            continue
        if not any(doc in changed for doc in docs):
            failures.append(
                f"{source} changed without mapped documentation update "
                f"({', '.join(str(doc) for doc in sorted(docs, key=str))})"
            )

    if failures:
        raise ContractError("; ".join(failures))


def main() -> int:
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--base",
        help="git base revision for same-change validation, e.g. HEAD^",
    )
    group.add_argument(
        "--staged",
        action="store_true",
        help="validate currently staged changes as one scoped change",
    )
    args = parser.parse_args()

    try:
        check_current_content()
        check_same_change(git_changed_files(args))
    except ContractError as exc:
        print(f"NATIVE DOC CONTRACT: FAIL: {exc}", file=sys.stderr)
        return 1

    print("NATIVE DOC CONTRACT: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
