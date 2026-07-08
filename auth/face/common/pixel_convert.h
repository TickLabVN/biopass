#pragma once

#include <cstddef>
#include <cstdint>

#include "image_utils.h"

namespace biopass {

// Converts a packed YUYV (YUY2) buffer into RGB8. `stride` is the number of
// bytes per row (may exceed width * 2 due to padding). Returns false if
// `size` is too small for the declared width/height/stride.
bool yuyvToRgb(const uint8_t* src, size_t size, int width, int height, int stride, ImageRGB& out);

// Expands a packed single-channel GREY/R8 buffer into RGB8 by replicating
// the channel. `stride` is the number of bytes per row.
bool greyToRgb(const uint8_t* src, size_t size, int width, int height, int stride, ImageRGB& out);

// Decodes a JPEG/MJPEG buffer (`bytes_used` valid bytes at `src`) into RGB8
// via libjpeg-turbo. Returns false on decode failure or a non-JPEG buffer.
bool mjpegToRgb(const uint8_t* src, size_t bytes_used, ImageRGB& out);

}  // namespace biopass
