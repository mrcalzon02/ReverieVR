#ifndef REVERIE_NATIVE_VECTOR_TEXT_H
#define REVERIE_NATIVE_VECTOR_TEXT_H
// Shared native GLES2 XYZRGB geometry-backed vector font for all native apps.
// Traces connected glyph outlines instead of 3x5 square-pixel bitmaps.
// No shader, font file, texture atlas or network dependency.
#include "classic_vector_art.h"
namespace reverie { namespace nativeart {
using classicart::StrokeText;
}}
#endif
