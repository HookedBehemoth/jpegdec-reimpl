#include "jpegdec_turbo.hpp"

#include "../capsrv_results.hpp"

/* STB image defines */
#define STBI_NO_STDIO
#define STBI_NEON
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ASSERT(x) AMS_ASSERT(x)
#include "stb_image.h"

#include <stratosphere.hpp>

namespace ams::jpegdec::impl {

    namespace {

        static int parse_entropy_coded_data(stbi__jpeg *z, u8 *buffer) {
            stbi__jpeg_reset(z);
            if (z->progressive || z->scan_n != 3) {
                /* Unsupported */
                return 0;
            }
            static u8 linebuffer[3][4 * 8];

            stbi__resample res_comp[3];
            u8 *coutput[4] = {NULL, NULL, NULL, NULL};
            u16 block_width = 8;
            u16 block_height = 8;

            for (int k = 0; k < 3; k++) {
                stbi__resample *r = &res_comp[k];

                r->hs = z->img_h_max / z->img_comp[k].h;
                r->vs = z->img_v_max / z->img_comp[k].v;
                r->ystep = r->vs >> 1;
                r->w_lores = (8 + r->hs - 1) / r->hs;

                if (r->hs == 1 && r->vs == 1) {
                    r->resample = resample_row_1;
                } else if (r->hs == 1 && r->vs == 2) {
                    r->resample = stbi__resample_row_v_2;
                    block_width = 8;
                    block_height = 16;
                } else if (r->hs == 2 && r->vs == 1) {
                    r->resample = stbi__resample_row_h_2;
                    block_width = 16;
                    block_height = 8;
                    r->w_lores = (16 + r->hs - 1) / r->hs;
                } else if (r->hs == 2 && r->vs == 2) {
                    r->resample = z->resample_row_hv_2_kernel;
                } else {
                    r->resample = stbi__resample_row_generic;
                }
            }

            /* interleaved */
            STBI_SIMD_ALIGN(short, data[64]);
            constexpr const size_t block_count = 4;
            constexpr const size_t block_size = 8 * 8;
            static stbi_uc block[block_count][block_size];

            for (int j = 0; j < z->img_mcu_y; ++j) {
                for (int i = 0; i < z->img_mcu_x; ++i) {
                    // scan an interleaved mcu... process scan_n components in order
                    u8 proc = 0;
                    for (int k = 0; k < 3; ++k) {
                        int n = z->order[k];
                        // scan out an mcu's worth of this component; that's just determined
                        // by the basic H and V specified for the component
                        for (int y = 0; y < z->img_comp[n].v; ++y) {
                            for (int x = 0; x < z->img_comp[n].h; ++x) {
                                int ha = z->img_comp[n].ha;
                                if (!stbi__jpeg_decode_block(z, data, z->huff_dc + z->img_comp[n].hd, z->huff_ac + ha, z->fast_ac[ha], n, z->dequant[z->img_comp[n].tq]))
                                    return 0;
                                z->idct_block_kernel(block[proc], 8, data);
                                proc++;
                                /* Unsupported and unexpected */
                                if (proc > 4) {
                                    return 0;
                                }
                            }
                        }

                        res_comp[k].ypos = 0;
                    }

                    res_comp[0].line0 = res_comp[0].line1 = block[0];
                    res_comp[1].line0 = res_comp[1].line1 = block[2];
                    res_comp[2].line0 = res_comp[2].line1 = block[3];

                    u8 *output = buffer;
                    /* Vertical block offset. */
                    output += j * block_height * 4 * z->s->img_x;
                    /* Horizontal block offset. */
                    output += i * 4 * block_width;

                    for (int s = 0; s < block_height; ++s) {
                        stbi_uc *out = output + 4 * z->s->img_x * s;
                        /* Verify we don't write out of bounds. */
                        if ((out + block_width * 4) > (buffer + z->s->img_x * z->s->img_y * 4)) {
                            break;
                        }
                        for (int k = 0; k < 3; ++k) {
                            stbi__resample *r = &res_comp[k];
                            int y_bot = r->ystep >= (r->vs >> 1);
                            /* At block end don't resample color. */
                            if (s != (block_height - 1) || k == 0) {
                                coutput[k] = r->resample(linebuffer[k],
                                                         y_bot ? r->line1 : r->line0,
                                                         y_bot ? r->line0 : r->line1,
                                                         r->w_lores, r->hs);
                            }
                            if (++r->ystep >= r->vs) {
                                r->ystep = 0;
                                r->line0 = r->line1;
                                if (++r->ypos < static_cast<int>(block_size))
                                    r->line1 += 8;
                            }
                        }
                        u8 rgba[block_width * 4] = {};
                        z->YCbCr_to_RGB_kernel(rgba, coutput[0], coutput[1], coutput[2], block_width, 4);
                        std::memcpy(out, rgba, block_width * 4);
                    }

                    // after all interleaved components, that's an interleaved MCU,
                    // so now count down the restart interval
                    if (--z->todo <= 0) {
                        if (z->code_bits < 24)
                            stbi__grow_buffer_unsafe(z);
                        if (!STBI__RESTART(z->marker))
                            return 1;
                        stbi__jpeg_reset(z);
                    }
                }
            }
            return 1;
        }

        static int process_frame_header(stbi__jpeg *z, int scan) {
            stbi__context *s = z->s;
            int Lf, p, i, q, h_max = 1, v_max = 1, c;
            Lf = stbi__get16be(s);
            if (Lf < 11)
                return stbi__err("bad SOF len", "Corrupt JPEG"); // JPEG
            p = stbi__get8(s);
            if (p != 8)
                return stbi__err("only 8-bit", "JPEG format not supported: 8-bit only"); // JPEG baseline
            s->img_y = stbi__get16be(s);
            if (s->img_y == 0)
                return stbi__err("no header height", "JPEG format not supported: delayed height"); // Legal, but we don't handle it--but neither does IJG
            s->img_x = stbi__get16be(s);
            if (s->img_x == 0)
                return stbi__err("0 width", "Corrupt JPEG"); // JPEG requires
            c = stbi__get8(s);
            if (c != 3 && c != 1 && c != 4)
                return stbi__err("bad component count", "Corrupt JPEG");
            s->img_n = c;
            for (i = 0; i < c; ++i) {
                z->img_comp[i].data = NULL;
                z->img_comp[i].linebuf = NULL;
            }

            if (Lf != 8 + 3 * s->img_n)
                return stbi__err("bad SOF len", "Corrupt JPEG");

            z->rgb = 0;
            for (i = 0; i < s->img_n; ++i) {
                static const unsigned char rgb[3] = {'R', 'G', 'B'};
                z->img_comp[i].id = stbi__get8(s);
                if (s->img_n == 3 && z->img_comp[i].id == rgb[i])
                    ++z->rgb;
                q = stbi__get8(s);
                z->img_comp[i].h = (q >> 4);
                if (!z->img_comp[i].h || z->img_comp[i].h > 4)
                    return stbi__err("bad H", "Corrupt JPEG");
                z->img_comp[i].v = q & 15;
                if (!z->img_comp[i].v || z->img_comp[i].v > 4)
                    return stbi__err("bad V", "Corrupt JPEG");
                z->img_comp[i].tq = stbi__get8(s);
                if (z->img_comp[i].tq > 3)
                    return stbi__err("bad TQ", "Corrupt JPEG");
            }

            if (scan != STBI__SCAN_load)
                return 1;

            if (!stbi__mad3sizes_valid(s->img_x, s->img_y, s->img_n, 0))
                return stbi__err("too large", "Image too large to decode");

            for (i = 0; i < s->img_n; ++i) {
                if (z->img_comp[i].h > h_max)
                    h_max = z->img_comp[i].h;
                if (z->img_comp[i].v > v_max)
                    v_max = z->img_comp[i].v;
            }

            // compute interleaved mcu info
            z->img_h_max = h_max;
            z->img_v_max = v_max;
            z->img_mcu_w = h_max * 8;
            z->img_mcu_h = v_max * 8;
            // these sizes can't be more than 17 bits
            z->img_mcu_x = (s->img_x + z->img_mcu_w - 1) / z->img_mcu_w;
            z->img_mcu_y = (s->img_y + z->img_mcu_h - 1) / z->img_mcu_h;

            for (i = 0; i < s->img_n; ++i) {
                // number of effective pixels (e.g. for non-interleaved MCU)
                z->img_comp[i].x = (s->img_x * z->img_comp[i].h + h_max - 1) / h_max;
                z->img_comp[i].y = (s->img_y * z->img_comp[i].v + v_max - 1) / v_max;
                // to simplify generation, we'll allocate enough memory to decode
                // the bogus oversized data from using interleaved MCUs and their
                // big blocks (e.g. a 16x16 iMCU on an image of width 33); we won't
                // discard the extra data until colorspace conversion
                //
                // img_mcu_x, img_mcu_y: <=17 bits; comp[i].h and .v are <=4 (checked earlier)
                // so these muls can't overflow with 32-bit ints (which we require)
                z->img_comp[i].w2 = z->img_mcu_x * z->img_comp[i].h * 8;
                z->img_comp[i].h2 = z->img_mcu_y * z->img_comp[i].v * 8;
                /* We don't use those. */
                z->img_comp[i].linebuf = nullptr;
                z->img_comp[i].raw_data = z->img_comp[i].data = nullptr;
                z->img_comp[i].raw_coeff = z->img_comp[i].coeff = nullptr;
                if (z->progressive) {
                    return 0;
                }
            }

            return 1;
        }

        static int decode_jpeg_header(stbi__jpeg *z, int scan) {
            int m;
            z->jfif = 0;
            z->app14_color_transform = -1; // valid values are 0,1,2
            z->marker = STBI__MARKER_none; // initialize cached marker to empty
            m = stbi__get_marker(z);
            if (!stbi__SOI(m))
                return stbi__err("no SOI", "Corrupt JPEG");
            if (scan == STBI__SCAN_type)
                return 1;
            m = stbi__get_marker(z);
            while (!stbi__SOF(m)) {
                if (!stbi__process_marker(z, m))
                    return 0;
                m = stbi__get_marker(z);
                while (m == STBI__MARKER_none) {
                    // some files have extra padding after their blocks, so ok, we'll scan
                    if (stbi__at_eof(z->s))
                        return stbi__err("no SOF", "Corrupt JPEG");
                    m = stbi__get_marker(z);
                }
            }
            z->progressive = stbi__SOF_progressive(m);
            if (!process_frame_header(z, scan))
                return 0;
            return 1;
        }

        // decode image to YCbCr format
        static int decode_jpeg_image(stbi__jpeg *j, u8 *buffer) {
            int m;
            for (m = 0; m < 4; m++) {
                j->img_comp[m].raw_data = NULL;
                j->img_comp[m].raw_coeff = NULL;
            }
            j->restart_interval = 0;
            if (!decode_jpeg_header(j, STBI__SCAN_load))
                return 0;
            m = stbi__get_marker(j);
            while (!stbi__EOI(m)) {
                if (stbi__SOS(m)) {
                    if (!stbi__process_scan_header(j))
                        return 0;
                    if (!parse_entropy_coded_data(j, buffer))
                        return 0;
                    if (j->marker == STBI__MARKER_none) {
                        // handle 0s at the end of image data from IP Kamera 9060
                        while (!stbi__at_eof(j->s)) {
                            int x = stbi__get8(j->s);
                            if (x == 255) {
                                j->marker = stbi__get8(j->s);
                                break;
                            }
                        }
                        // if we reach eof without hitting a marker, stbi__get_marker() below will fail and we'll eventually return 0
                    }
                } else if (stbi__DNL(m)) {
                    int Ld = stbi__get16be(j->s);
                    stbi__uint32 NL = stbi__get16be(j->s);
                    if (Ld != 4)
                        return stbi__err("bad DNL len", "Corrupt JPEG");
                    if (NL != j->s->img_y)
                        return stbi__err("bad DNL height", "Corrupt JPEG");
                } else {
                    if (!stbi__process_marker(j, m))
                        return 0;
                }
                m = stbi__get_marker(j);
            }
            if (j->progressive)
                stbi__jpeg_finish(j);
            return 1;
        }

        stbi__context g_ctx;
        stbi__jpeg g_jpeg;

    }

    Result DecodeJpeg(u8 *bmp, size_t bmpSize, const u8 *jpeg, size_t jpegSize, u32 width, u32 height, const CapsScreenShotDecodeOption &opts) {
        std::memset(&g_ctx, 0, sizeof(stbi__context));
        stbi__start_mem(&g_ctx, jpeg, jpegSize);

        std::memset(&g_jpeg, 0, sizeof(stbi__jpeg));
        stbi__setup_jpeg(&g_jpeg);
        ON_SCOPE_EXIT { stbi__cleanup_jpeg(&g_jpeg); };

        g_jpeg.s = &g_ctx;

        R_UNLESS(decode_jpeg_image(&g_jpeg, bmp) == 1, capsrv::ResultInvalidFileData());
        R_UNLESS(g_jpeg.s->img_x == width, capsrv::ResultInvalidFileData());
        R_UNLESS(g_jpeg.s->img_y == height, capsrv::ResultInvalidFileData());

        return 0;
    }

}