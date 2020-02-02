#pragma once
#include <stratosphere.hpp>

namespace ams::jpegdec::stb {

    Result DecodeJpeg(u8* bmp, const u64 bmpSize, const u8* jpeg, const u64 jpegSize, const u32 width, const u32 height, const CapsScreenShotDecodeOption &opts);

}