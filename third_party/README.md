# Third-party dependencies

## Cardboard SDK

ReverieVR pins the open-source Google Cardboard SDK as a Git submodule:

- upstream: `https://github.com/googlevr/cardboard`
- release: `v1.35.0`
- pinned commit: `5969239e7c87f4cd64c8ec170ce1e7f4eb559e37`
- primary license: Apache License 2.0, except separately licensed upstream third-party directories.

ReverieVR configures the Cardboard native build with:

- `CARDBOARDSDK_UNITY_PLUGIN=OFF`
- `CARDBOARDSDK_RENDERING_VULKAN=OFF`
- OpenGL ES rendering retained.

The Unity plugin API is therefore not part of ReverieVR's intended native build.

The Cardboard SDK Gradle module expects `proto/cardboard_device.proto` at the consuming Gradle root. ReverieVR carries that single Apache-licensed upstream proto file at the required root path with its original copyright/license header intact.

Clone ReverieVR with submodules or run:

    git submodule update --init --recursive

The application must not silently substitute an unpinned moving Cardboard revision.
