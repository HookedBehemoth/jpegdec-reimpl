#include <stratosphere.hpp>
#include "jpegdec_decode_service.hpp"
#include "impl/jpegdec_turbo.hpp"

#include <jpeglib.h>

namespace ams::jpegdec {

    Result DecodeService::DecodeJpeg(const sf::OutNonSecureBuffer &out, const sf::InBuffer &in, const u32 width, const u32 height, const ScreenShotDecodeOption &opts) {
        unsigned char* bmp = out.GetPointer();
        u64 bmpSize = out.GetSize();
        const unsigned char* jpeg = in.GetPointer();
        u64 jpegSize = in.GetSize();

        memset(bmp, 0, bmpSize);
        
        R_UNLESS(util::IsAligned(width, 16), 0x10ce);
        R_UNLESS(util::IsAligned(height, 4), 0x10ce);

        R_UNLESS(bmp != nullptr, 0x3cce);
        R_UNLESS(bmpSize >= 4 * width * height, 0x3cce);

        R_UNLESS(jpeg != nullptr, 0x30ce);
        R_UNLESS(jpegSize > 0, 0x30ce);

        Result rc = impl::DecodeJpeg(bmp, bmpSize, jpeg, jpegSize, width, height);
        
        if (rc.IsFailure())
            memset(bmp, 0, bmpSize);

        return rc;
    }
}