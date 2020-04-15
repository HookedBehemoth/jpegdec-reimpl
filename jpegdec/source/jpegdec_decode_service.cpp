#include "jpegdec_decode_service.hpp"

#include "capsrv_results.hpp"
#include "impl/jpegdec_turbo.hpp"

#include <jpeglib.h>
#include <stratosphere.hpp>

namespace ams::jpegdec {

    Result DecodeService::DecodeJpeg(const sf::OutNonSecureBuffer &out, const sf::InBuffer &in, u32 width, u32 height, const CapsScreenShotDecodeOption &opts) {
        u8 *bmp = out.GetPointer();
        size_t bmpSize = out.GetSize();
        const u8 *jpeg = in.GetPointer();
        size_t jpegSize = in.GetSize();

        memset(bmp, 0, bmpSize);

        R_UNLESS(util::IsAligned(width, 16), capsrv::ResultOutOfRange());
        R_UNLESS(util::IsAligned(height, 4), capsrv::ResultOutOfRange());

        R_UNLESS(bmp != nullptr, capsrv::ResultBufferInsufficient());
        R_UNLESS(bmpSize >= 4 * width * height, capsrv::ResultBufferInsufficient());

        R_UNLESS(jpeg != nullptr, capsrv::ResultInvalidFileData());
        R_UNLESS(jpegSize != 0, capsrv::ResultInvalidFileData());

        Result rc = impl::DecodeJpeg(bmp, bmpSize, jpeg, jpegSize, width, height, opts);

        if (rc.IsFailure())
            memset(bmp, 0, bmpSize);

        return rc;
    }
}