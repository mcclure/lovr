#include "filesystem/blob.h"
#include "util.h"
#include <stdint.h>
#include <stdbool.h>

#pragma once

// WEBGL_compressed_texture_s3tc_srgb isn't ratified yet...
#ifdef EMSCRIPTEN
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT 0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif

typedef enum {
  SAMPLE_FILTER_WEIGHTED_AVERAGE,
  SAMPLE_FILTER_MIN,
  SAMPLE_FILTER_MAX,
} SampleFilter;

typedef enum {
  FORMAT_RGB,
  FORMAT_RGBA,
  FORMAT_RGBA16F,
  FORMAT_RGBA32F,
  FORMAT_RG11B10F,
  FORMAT_DXT1,
  FORMAT_DXT3,
  FORMAT_DXT5
} TextureFormat;

typedef struct {
  int width;
  int height;
  void* data;
  size_t size;
} Mipmap;

typedef vec_t(Mipmap) vec_mipmap_t;

typedef struct {
  Ref ref;
  int width;
  int height;
  void* data;
  Blob* blob;
  TextureFormat format;
  bool generateMipmaps;
  vec_mipmap_t mipmaps;
} TextureData;

TextureData* lovrTextureDataGetBlank(int width, int height, uint8_t value, TextureFormat format);
TextureData* lovrTextureDataGetEmpty(int width, int height, TextureFormat format);
TextureData* lovrTextureDataFromBlob(Blob* blob);
void lovrTextureDataDestroy(const Ref* ref);
