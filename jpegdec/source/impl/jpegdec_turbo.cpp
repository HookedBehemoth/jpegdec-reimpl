#include <stratosphere.hpp>
#include "jpegdec_turbo.hpp"

#include <jpeglib.h>

namespace ams::jpegdec::impl {
    
    Result DecodeJpeg(unsigned char* bmp, const u64 bmpSize, const unsigned char* jpeg, const u64 jpegSize, const u32 width, const u32 height) {
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);

        ON_SCOPE_EXIT {
            jpeg_destroy_decompress(&cinfo);
        };

        jpeg_mem_src(&cinfo, jpeg, jpegSize);

        int res = jpeg_read_header(&cinfo, true);

        R_UNLESS(res == JPEG_HEADER_OK, 0x4ce);
        R_UNLESS(cinfo.image_width == width, 0x4ce);
        R_UNLESS(cinfo.image_height == height, 0x4ce);

        /* N only does RGBA */
        cinfo.out_color_space = JCS_EXT_RGBA;
        /* four byes per pixel RGBARGBA... */
        int row_stride = width * 4;

        res = jpeg_start_decompress(&cinfo);

        unsigned char *buffer_array[1];
        while (cinfo.output_scanline < cinfo.output_height) {
            buffer_array[0] = bmp + cinfo.output_scanline * row_stride;
            jpeg_read_scanlines(&cinfo, buffer_array, 1);
        }

        jpeg_finish_decompress(&cinfo);

        return ResultSuccess();
    }

}