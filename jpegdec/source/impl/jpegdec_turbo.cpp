#include "jpegdec_turbo.hpp"

#include "../capsrv_results.hpp"

#include <jpeglib.h>
#include <stratosphere.hpp>

namespace ams::jpegdec::impl {

    Result DecodeJpeg(u8 *bmp, size_t bmpSize, const u8 *jpeg, size_t jpegSize, u32 width, u32 height, const CapsScreenShotDecodeOption &opts) {
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);

        ON_SCOPE_EXIT {
            jpeg_destroy_decompress(&cinfo);
        };

        jpeg_mem_src(&cinfo, jpeg, jpegSize);

        int res = jpeg_read_header(&cinfo, true);

        R_UNLESS(res == JPEG_HEADER_OK, capsrv::ResultInvalidFileData());
        R_UNLESS(cinfo.image_width == width, capsrv::ResultInvalidFileData());
        R_UNLESS(cinfo.image_height == height, capsrv::ResultInvalidFileData());

        /* N only does RGB and defined pixelwidth to be 4. */
        cinfo.out_color_space = JCS_EXT_RGBA;
        cinfo.dct_method = JDCT_ISLOW;
        cinfo.do_fancy_upsampling = opts.fancy_upsampling;
        cinfo.do_block_smoothing = opts.block_smoothing;

        res = jpeg_start_decompress(&cinfo);

        R_UNLESS(res == TRUE, capsrv::ResultInvalidFileData());
        R_UNLESS(cinfo.output_width == width, capsrv::ResultInvalidArgument());
        R_UNLESS(cinfo.output_height == height, capsrv::ResultInvalidArgument());

        /* four byes per pixel RGBARGBA... */
        int row_stride = width * 4;

        /* N does four lines at once */
        u8 *buffer_array[1];
        while (cinfo.output_scanline < cinfo.output_height) {
            buffer_array[0] = bmp + cinfo.output_scanline * row_stride;
            jpeg_read_scanlines(&cinfo, buffer_array, 1);
        }

        jpeg_finish_decompress(&cinfo);

        return ResultSuccess();
    }

}