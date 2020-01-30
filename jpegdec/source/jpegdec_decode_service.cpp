#include "jpegdec_decode_service.hpp"
#include "impl/jpegdec_turbo.hpp"

#include <jpeglib.h>

namespace ams::jpegdec {

    Result DecodeService::DecodeJpeg(sf::OutNonSecureBuffer out, sf::InBuffer in, const u32 width, const u32 height, const u64 q, const u64 qw, const u64 qwe, const u64 qwer) {
        unsigned char* bmp = out.GetPointer();
        u64 bmpSize = out.GetSize();
        const unsigned char* jpeg = in.GetPointer();
        u64 jpegSize = in.GetSize();

        memset(bmp, 0, bmpSize);
        
        if (((height & 0x3) | (width & 0xf)) != 0)
            return 0x10ce;
        
        if ((bmp == nullptr) || (bmpSize < width*height*4))
            return 0x3cce;

        if ((jpeg == nullptr) || (jpegSize == 0))
            return 0x30ce;

        Result rc = impl::DecodeJpeg(bmp, bmpSize, jpeg, jpegSize, width, height);
        
        if (rc.IsFailure())
            memset(bmp, 0, bmpSize);

        return rc;
    }
}