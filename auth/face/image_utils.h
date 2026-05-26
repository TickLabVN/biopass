#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "stb_image.h"
#include "stb_image_write.h"

/**
 * Minimal RGB image container replacing cv::Mat.
 * Stores 3-channel (RGB) uint8 data in row-major order.
 */
struct ImageRGB {
  int width = 0;
  int height = 0;
  std::vector<uint8_t> data;  // size = width * height * 3

  static size_t byteSize(int w, int h) {
    if (w <= 0 || h <= 0) return 0;
    const size_t ws = static_cast<size_t>(w);
    const size_t hs = static_cast<size_t>(h);
    if (ws > std::numeric_limits<size_t>::max() / hs / 3) return 0;
    return ws * hs * 3;
  }

  ImageRGB() = default;
  ImageRGB(int w, int h) : width(std::max(0, w)), height(std::max(0, h)), data(byteSize(width, height), 0) {}
  ImageRGB(int w, int h, const uint8_t *src) : width(std::max(0, w)), height(std::max(0, h)) {
    const size_t bytes = byteSize(width, height);
    if (bytes == 0 || !src) {
      width = 0;
      height = 0;
      return;
    }
    data.resize(bytes);
    std::memcpy(data.data(), src, bytes);
  }

  bool empty() const { return data.empty(); }
  uint8_t *ptr() { return data.data(); }
  const uint8_t *ptr() const { return data.data(); }

  uint8_t &at(int y, int x, int c) { return data[(y * width + x) * 3 + c]; }
  const uint8_t &at(int y, int x, int c) const { return data[(y * width + x) * 3 + c]; }

  ImageRGB crop(int x1, int y1, int x2, int y2) const {
    x1 = std::max(0, x1);
    y1 = std::max(0, y1);
    x2 = std::min(width, x2);
    y2 = std::min(height, y2);
    int cw = x2 - x1, ch = y2 - y1;
    if (cw <= 0 || ch <= 0) return {};
    ImageRGB out(cw, ch);
    for (int r = 0; r < ch; r++)
      std::memcpy(&out.data[r * cw * 3], &data[((y1 + r) * width + x1) * 3], cw * 3);
    return out;
  }

  ImageRGB clone() const {
    ImageRGB out;
    out.width = width;
    out.height = height;
    out.data = data;
    return out;
  }
};

/**
 * Bilinear resize.
 */
inline ImageRGB resizeImage(const ImageRGB &src, int tw, int th) {
  if (src.empty()) return {};
  ImageRGB dst(tw, th);
  float sx = (float)src.width / tw;
  float sy = (float)src.height / th;
  for (int y = 0; y < th; y++) {
    float fy = (y + 0.5f) * sy - 0.5f;
    int y0 = (int)std::floor(fy);
    int y1 = y0 + 1;
    float wy = fy - y0;
    y0 = std::max(0, std::min(y0, src.height - 1));
    y1 = std::max(0, std::min(y1, src.height - 1));
    for (int x = 0; x < tw; x++) {
      float fx = (x + 0.5f) * sx - 0.5f;
      int x0 = (int)std::floor(fx);
      int x1_c = x0 + 1;
      float wx = fx - x0;
      x0 = std::max(0, std::min(x0, src.width - 1));
      x1_c = std::max(0, std::min(x1_c, src.width - 1));
      for (int c = 0; c < 3; c++) {
        float v = (1 - wy) * ((1 - wx) * src.at(y0, x0, c) + wx * src.at(y0, x1_c, c)) +
                  wy * ((1 - wx) * src.at(y1, x0, c) + wx * src.at(y1, x1_c, c));
        dst.at(y, x, c) = (uint8_t)std::min(255.0f, std::max(0.0f, v + 0.5f));
      }
    }
  }
  return dst;
}

/**
 * Letterbox resize: scale to fit target keeping aspect ratio, pad with pad_val.
 */
inline ImageRGB imageLetterbox(const ImageRGB &src, int tw, int th, uint8_t pad_val = 114) {
  if (src.empty()) return {};
  float scale = std::min((float)tw / src.width, (float)th / src.height);
  int nw = (int)std::round(src.width * scale);
  int nh = (int)std::round(src.height * scale);
  ImageRGB resized = resizeImage(src, nw, nh);

  ImageRGB out(tw, th);
  std::memset(out.data.data(), pad_val, out.data.size());

  int dx = (tw - nw) / 2;
  int dy = (th - nh) / 2;
  for (int r = 0; r < nh; r++)
    std::memcpy(&out.data[((dy + r) * tw + dx) * 3], &resized.data[r * nw * 3], nw * 3);
  return out;
}

/**
 * Resize with aspect-ratio preserving and zero-padding (for recognition).
 */
inline ImageRGB imageResizePad(const ImageRGB &src, int tw, int th) {
  return imageLetterbox(src, tw, th, 0);
}

/**
 * Unsharp mask sharpening.  Applies a 3-tap separable blur (kernel [1,2,1])
 * then computes: result = input + amount * (input - blurred).
 * Clamped to [0,255].  amount ≈ 3.0 matches x4 PIL sharpness.
 */
inline ImageRGB sharpenImage(const ImageRGB &src, float amount = 3.0f) {
  if (src.empty()) return {};
  int h = src.height, w = src.width;
  ImageRGB tmp(w, h);
  // Horizontal pass: blur rows
  for (int y = 0; y < h; y++) {
    for (int x = 1; x < w - 1; x++) {
      for (int c = 0; c < 3; c++) {
        int v = (int)src.at(y, x - 1, c) + 2 * (int)src.at(y, x, c) + (int)src.at(y, x + 1, c);
        tmp.at(y, x, c) = (uint8_t)((v + 2) >> 2);  // round division by 4
      }
    }
    // Left/right edges: copy from src (unblurred)
    for (int c = 0; c < 3; c++) {
      tmp.at(y, 0, c) = src.at(y, 0, c);
      tmp.at(y, w - 1, c) = src.at(y, w - 1, c);
    }
  }
  ImageRGB blurred(w, h);
  // Vertical pass: blur columns
  for (int x = 0; x < w; x++) {
    for (int y = 1; y < h - 1; y++) {
      for (int c = 0; c < 3; c++) {
        int v = (int)tmp.at(y - 1, x, c) + 2 * (int)tmp.at(y, x, c) + (int)tmp.at(y + 1, x, c);
        blurred.at(y, x, c) = (uint8_t)((v + 2) >> 2);
      }
    }
    for (int c = 0; c < 3; c++) {
      blurred.at(0, x, c) = tmp.at(0, x, c);
      blurred.at(h - 1, x, c) = tmp.at(h - 1, x, c);
    }
  }
  // Unsharp mask: out = src + amount * (src - blurred)
  ImageRGB out(w, h);
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      for (int c = 0; c < 3; c++) {
        int s = (int)src.at(y, x, c);
        int b = (int)blurred.at(y, x, c);
        int v = (int)(s + amount * (s - b) + 0.5f);
        out.at(y, x, c) = (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v);
      }
    }
  }
  return out;
}

/**
 * HWC RGB uint8 -> CHW float, normalized to [0,1].
 */
inline std::vector<float> imageToChw(const ImageRGB &img) {
  int h = img.height, w = img.width;
  std::vector<float> out(3 * h * w);
  for (int c = 0; c < 3; c++)
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++)
        out[c * h * w + y * w + x] = img.at(y, x, c) / 255.0f;
  return out;
}

/**
 * HWC RGB uint8 -> CHW float, with mean/std normalization.
 */
inline std::vector<float> imageToChwNormalized(const ImageRGB &img, const float mean[3],
                                                  const float std_val[3]) {
  int h = img.height, w = img.width;
  std::vector<float> out(3 * h * w);
  for (int c = 0; c < 3; c++)
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++)
        out[c * h * w + y * w + x] = (img.at(y, x, c) / 255.0f - mean[c]) / std_val[c];
  return out;
}

namespace {
inline std::string lowercasePathExtension(const std::string &path) {
  size_t dot = path.rfind('.');
  if (dot == std::string::npos) return "";
  std::string ext = path.substr(dot);
  for (auto &ch : ext) ch = (char)std::tolower((unsigned char)ch);
  return ext;
}
}  // namespace

/**
 * Load image from any supported format (JPEG, PNG, BMP, GIF, TGA, PSD, HDR, PIC, PNM).
 * Automatically detects format from file contents via stb_image.
 */
inline ImageRGB readImage(const std::string &path) {
  int w = 0, h = 0, channels = 0;
  uint8_t *pixels = stbi_load(path.c_str(), &w, &h, &channels, 3);
  if (!pixels) return {};

  ImageRGB img(w, h, pixels);
  stbi_image_free(pixels);
  return img;
}

/**
 * Save image to file. Format is determined by file extension:
 *   .jpg / .jpeg  -> JPEG (quality 95)
 *   .png          -> PNG
 *   .bmp          -> BMP
 *   .tga          -> TGA
 * Returns false on unsupported extension or write failure.
 */
inline bool saveImage(const std::string &path, const ImageRGB &img) {
  if (img.empty()) return false;

  std::string ext = lowercasePathExtension(path);
  int stride = img.width * 3;

  if (ext == ".jpg" || ext == ".jpeg") {
    return stbi_write_jpg(path.c_str(), img.width, img.height, 3, img.ptr(), 95) != 0;
  } else if (ext == ".png") {
    return stbi_write_png(path.c_str(), img.width, img.height, 3, img.ptr(), stride) != 0;
  } else if (ext == ".bmp") {
    return stbi_write_bmp(path.c_str(), img.width, img.height, 3, img.ptr()) != 0;
  } else if (ext == ".tga") {
    return stbi_write_tga(path.c_str(), img.width, img.height, 3, img.ptr()) != 0;
  }

  // Fallback: save as PNG
  return stbi_write_png(path.c_str(), img.width, img.height, 3, img.ptr(), stride) != 0;
}

#endif  // IMAGE_UTILS_H
