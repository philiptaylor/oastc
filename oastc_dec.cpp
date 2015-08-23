/*
 * Copyright (c) 2015 Philip Taylor
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <fstream>

#include "oastc.h"

#include "optionparser.h"

enum OptionId
{
    UNKNOWN,
    HELP,

    INPUT,
    OUTPUT,
};

static const option::Descriptor usage[] =
{
    { UNKNOWN,  0, "",  "",          Arg::Unknown,  "Options:" },
    { HELP,     0, "",  "help",      Arg::None,     "  --help  \tPrint usage and exit" },
    { INPUT,    0, "i", "input",     Arg::Required, "  -i --input FILENAME  \tInput filename (supported formats: .astc)" },
    { OUTPUT,   0, "o", "output",    Arg::Required, "  -o --output FILENAME  \tOutput filename (supported formats: .tga)" },
    { 0,0,0,0,0,0 }
};

// .astc file format described at http://malideveloper.arm.com/downloads/Stacy_ASTC_white%20paper.pdf
struct astc_header
{
    uint32_t magic;
    uint8_t blockdim_x;
    uint8_t blockdim_y;
    uint8_t blockdim_z;
    uint8_t xsize[3];
    uint8_t ysize[3];
    uint8_t zsize[3];
};
static_assert(sizeof(astc_header) == 16, "no unexpected padding in astc_header");

int main(int argc, char **argv)
{
    const char *program_name = nullptr;
    if (argc > 0) {
        program_name = argv[0];
        argc--;
        argv++;
    }
    option::Stats stats(usage, argc, argv);
    std::vector<option::Option> options(stats.options_max), buffer(stats.buffer_max);
    option::Parser parse(usage, argc, argv, options.data(), buffer.data());

    if (parse.error()) {
        std::cerr << "Run '" << program_name << " --help' for supported options\n";
        return 1;
    }

    if (parse.nonOptionsCount()) {
        std::cerr << "Unknown option '" << parse.nonOption(0) << "'\n";
        std::cerr << "Run '" << program_name << " --help' for supported options\n";
        return 1;
    }

    if (options[HELP] || !options[INPUT] || !options[OUTPUT]) {
        std::cout << "USAGE: " << program_name << " --input FILENAME --output FILENAME [options]\n\n";
        option::printUsage(std::cout, usage);
        return 0;
    }

    const char *input_fn = options[INPUT].arg;
    const char *output_fn = options[OUTPUT].arg;

    std::ifstream input(input_fn, std::ios_base::binary | std::ios_base::in);
    if (!input) {
        fprintf(stderr, "Failed to open \"%s\" for input\n", input_fn);
        return 1;
    }

    astc_header header{};

    input.read((char *)&header, sizeof(astc_header));

    if (header.magic != 0x5ca1ab13) {
        fprintf(stderr, "Invalid header magic 0x%08x - input must be a valid .astc file\n", header.magic);
        return 1;
    }

    int block_w = header.blockdim_x;
    int block_h = header.blockdim_y;
    int block_d = header.blockdim_z;

    int image_w = (header.xsize[0] + (header.xsize[1] << 8) + (header.xsize[2] << 16));
    int image_h = (header.ysize[0] + (header.ysize[1] << 8) + (header.ysize[2] << 16));
    int image_d = (header.zsize[0] + (header.zsize[1] << 8) + (header.zsize[2] << 16));

    fprintf(stderr, "Decoding '%s' (image size %dx%dx%d, block size %dx%dx%d)\n",
            input_fn,
            image_w, image_h, image_d,
            block_w, block_h, block_d);

    std::vector<oastc::fp16> block_out(block_w * block_h * block_d * 4);
    std::vector<uint8_t> block_out_unorm8(block_w * block_h * block_d * 4);
    std::vector<uint8_t> image_out(image_w * image_h * image_d * 4);
    oastc::Decoder dec(block_w, block_h, block_d);

    for (int z = 0; z < (image_d + block_d - 1) / block_d; ++z) {
        for (int y = 0; y < (image_h + block_h - 1) / block_h; ++y) {
            for (int x = 0; x < (image_w + block_w - 1) / block_w; ++x) {
                uint8_t block[16];
                input.read((char *)block, 16);

                oastc::decode_error err = dec.decode(block, block_out.data());
                if (err != oastc::decode_error::ok)
                    printf("Decode error %d\n", err);

                for (int i = 0; i < block_w * block_h * block_d * 4; i += 4) {
                    block_out_unorm8[i+0] = block_out[i+0].to_unorm8();
                    block_out_unorm8[i+1] = block_out[i+1].to_unorm8();
                    block_out_unorm8[i+2] = block_out[i+2].to_unorm8();
                    block_out_unorm8[i+3] = block_out[i+3].to_unorm8();
                }

                for (int bz = 0; bz < std::min(block_d, image_d - z*block_d); ++bz) {
                    for (int by = 0; by < std::min(block_h, image_h - y*block_h); ++by) {
                        int image_idx = x*block_w + (y*block_h+by) * image_w + (z*block_d+bz) * image_w * image_h;
                        int block_idx = by * block_w + bz * block_w * block_h;
                        memcpy(&image_out[image_idx*4], &block_out_unorm8[block_idx*4], std::min(block_w, image_w - x*block_w)*4);
                    }
                }
            }
        }
    }

    std::ofstream output(output_fn, std::ios_base::binary | std::ios_base::out);
    if (!output) {
        fprintf(stderr, "Failed to open \"%s\" for output\n", output_fn);
        return 1;
    }

    bool has_alpha = false;
    for (size_t i = 0; i < image_out.size(); i += 4) {
        if (image_out[i+3] != 255) {
            has_alpha = true;
            break;
        }
    }

    const uint8_t tga_header[18] = {
        0, 0, 2,
        0, 0, 0, 0, 0,
        0, 0, 0, 0,
        (uint8_t)(image_w & 0xff), (uint8_t)(image_w >> 8),
        (uint8_t)(image_h & 0xff), (uint8_t)(image_h >> 8),
        (uint8_t)(has_alpha ? 32 : 24), 0,
    };
    output.write((const char *)tga_header, sizeof(tga_header));

    static const size_t output_buffer_px = 4096;
    std::vector<uint8_t> output_buffer(output_buffer_px * 4);

    for (size_t offset = 0; offset < image_out.size(); offset += output_buffer_px * 4) {
        uint8_t *p = output_buffer.data();
        size_t num_px = std::min(output_buffer_px, (image_out.size() - offset) / 4);
        if (has_alpha) {
            for (size_t i = 0; i < num_px; ++i) {
                *p++ = image_out[offset + i*4+2];
                *p++ = image_out[offset + i*4+1];
                *p++ = image_out[offset + i*4+0];
                *p++ = image_out[offset + i*4+3];
            }
        } else {
            for (size_t i = 0; i < num_px; ++i) {
                *p++ = image_out[offset + i*4+2];
                *p++ = image_out[offset + i*4+1];
                *p++ = image_out[offset + i*4+0];
            }
        }
        output.write((const char *)output_buffer.data(), num_px * (has_alpha ? 4 : 3));
    }

    fprintf(stderr, "Wrote '%s'\n", output_fn);
}
