#pragma once
// Packaged Android assets are outside the host-save test's scope.
#include <sys/types.h>
#include <stddef.h>
struct AAssetManager {};
struct AAsset {};
#define AASSET_MODE_BUFFER 3
#define AASSET_MODE_STREAMING 2
static inline AAsset* AAssetManager_open(AAssetManager*, const char*, int) { return nullptr; }
static inline off64_t AAsset_getLength64(AAsset*) { return 0; }
static inline int AAsset_read(AAsset*, void*, size_t) { return -1; }
static inline void AAsset_close(AAsset*) {}
