#pragma once

#include <vapours/results.hpp>
#include <switch/types.h>

namespace ams::jpegdec::impl {

    Result DecodeJpeg(unsigned char* bmp, const u64 bmpSize, const unsigned char* jpeg, const u64 jpegSize, const u32 width, const u32 height);

}