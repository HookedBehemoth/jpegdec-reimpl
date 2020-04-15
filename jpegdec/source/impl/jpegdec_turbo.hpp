#pragma once
#include <stratosphere.hpp>

namespace ams::jpegdec::impl {

    Result DecodeJpeg(u8 *bmp, size_t bmpSize, const u8 *jpeg, size_t jpegSize, u32 width, u32 height, const CapsScreenShotDecodeOption &opts);

}