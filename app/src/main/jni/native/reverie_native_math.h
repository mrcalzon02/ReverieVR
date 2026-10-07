#ifndef REVERIE_NATIVE_MATH_H
#define REVERIE_NATIVE_MATH_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Column-major 4x4 multiplication used by the GLES2 native modules.
 * The temporary result makes out==a and out==b safe.
 */
static inline void ReverieNativeMat4Multiply(
    float *out,
    const float *a,
    const float *b
) {
    if (out == NULL || a == NULL || b == NULL) {
        return;
    }

    float result[16] = {0.0f};

    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            result[column * 4 + row] =
                a[0 * 4 + row] * b[column * 4 + 0]
                + a[1 * 4 + row] * b[column * 4 + 1]
                + a[2 * 4 + row] * b[column * 4 + 2]
                + a[3 * 4 + row] * b[column * 4 + 3];
        }
    }

    memcpy(out, result, sizeof(result));
}

#ifdef __cplusplus
}
#endif

#endif
