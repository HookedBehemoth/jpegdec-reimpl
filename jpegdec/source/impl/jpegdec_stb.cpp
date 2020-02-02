#include <stratosphere.hpp>
#include "jpegdec_stb.hpp"

#define STBI_NO_STDIO
#define STBI_NEON
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG

#include "stb_image.h"

namespace ams::jpegdec::stb {

    Result DecodeJpeg(u8* bmp, const u64 bmpSize, const u8* jpeg, const u64 jpegSize, const u32 width, const u32 height, const CapsScreenShotDecodeOption &opts) {
        int x, y, comp;

        stbi_uc *data = stbi_load_from_memory(jpeg, jpegSize, &x, &y, &comp, STBI_rgb_alpha);

        R_UNLESS(data != nullptr && x == width && y == width && comp == 4, capsrv::ResultInvalidFileData());

        std::memcpy(bmp, data, x*y*comp);

        stbi_image_free(data);

        return ResultSuccess();
    }

}