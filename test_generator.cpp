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

#include "oastc.h"

#include <sstream>
#include <fstream>

using namespace oastc;

static bool VERBOSE_TEST = false;
static bool TEST_GENERATE_INVALID_BLOCKS = false;

class TestGenerator
{
public:
    TestGenerator();

    void seed(uint32_t val) { m_rng.seed(val); }

    void generate_with_block_size(const Encoder &encoder);
    bool write_output_file(const Encoder &encoder);

private:
    std::mt19937 m_rng;

    bool m_allow_hdr;
    int m_count;

    std::vector<OutputBitVector> m_output_blocks;

    void write_encoded_block(const OutputBitVector &data);
    void generate_with_cems(const Encoder &encoder, Block blk, bool is_multi_cem, int base, int cem0, int cem1, int cem2, int cem3);
    void generate_with_block_mode(const Encoder &encoder, Block blk, int wt_w, int wt_h, int wt_d);
};

TestGenerator::TestGenerator()
    : m_allow_hdr(false), m_count(0)
{
}

void TestGenerator::write_encoded_block(const OutputBitVector &data)
{
    m_output_blocks.push_back(data);
}

bool TestGenerator::write_output_file(const Encoder &encoder)
{
    uint32_t magic = 0x5ca1ab13;
    uint8_t block_w = encoder.block_w;
    uint8_t block_h = encoder.block_h;
    uint8_t block_d = encoder.block_d;

    size_t block_start = 0;
    int idx = 0;

    while (block_start < m_output_blocks.size()) {
        std::stringstream filename;
        filename << "testgen_" << (int)block_w << "x" << (int)block_h << "x" << (int)block_d << "-" << idx++ << ".astc";
        std::ofstream out(filename.str(), std::ios::binary);
        if (!out) {
            fprintf(stderr, "Failed to open output file '%s'\n", filename.str().c_str());
            return false;
        }

        int image_w = block_w * (4096 / block_w);
        int image_h = block_h * std::min((image_w / block_w) + 1, 4096 / block_h);
        int image_d = block_d;

        uint8_t header[16];
        memcpy(&header[0], &magic, 4);
        header[4] = block_w;
        header[5] = block_h;
        header[6] = block_d;
        header[7] = image_w & 0xff;
        header[8] = (image_w >> 8) & 0xff;
        header[9] = (image_w >> 16) & 0xff;
        header[10] = image_h & 0xff;
        header[11] = (image_h >> 8) & 0xff;
        header[12] = (image_h >> 16) & 0xff;
        header[13] = image_d & 0xff;
        header[14] = (image_d >> 8) & 0xff;
        header[15] = (image_d >> 16) & 0xff;

        out.write((const char *)&header, 16);

        size_t num_blocks = (image_w / block_w) * (image_h / block_h) * (image_d / block_d);

        size_t i;
        for (i = 0; i < num_blocks && i + block_start < m_output_blocks.size(); ++i) {
            out.write((const char *)m_output_blocks[i + block_start].data, 16);
        }
        for (; i < num_blocks; ++i) {
            // Void extent, black
            uint8_t dummy[16] = { 0b11111100, 0b11111101, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0 };
            out.write((const char *)dummy, 16);
        }

        block_start += num_blocks;
    }

    m_output_blocks.clear();

    return true;
}

void TestGenerator::generate_with_cems(const Encoder &encoder, Block blk, bool is_multi_cem, int base, int cem0, int cem1, int cem2, int cem3)
{
    if (blk.dual_plane && blk.num_parts == 4) {
        blk.is_error = true;
        if (!TEST_GENERATE_INVALID_BLOCKS)
            return;
    }

    if (!m_allow_hdr) {
        int hdr_only[] = { 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1 };
        if (hdr_only[cem0+1] || hdr_only[cem1+1] || hdr_only[cem2+1] || hdr_only[cem3+1])
            return;
    }

    blk.is_multi_cem = is_multi_cem;
    blk.cem_base_class = base;
    blk.cems[0] = cem0;
    blk.cems[1] = cem1;
    blk.cems[2] = cem2;
    blk.cems[3] = cem3;

    int num_cem_pairs = (cem0 >> 2) + 1;
    if (blk.num_parts > 1)
        num_cem_pairs += (cem1 >> 2) + 1;
    if (blk.num_parts > 2)
        num_cem_pairs += (cem2 >> 2) + 1;
    if (blk.num_parts > 3)
        num_cem_pairs += (cem3 >> 2) + 1;

    blk.num_cem_values = num_cem_pairs * 2;

    // Specified as illegal
    if (blk.num_cem_values > 18) {
        blk.is_error = true;
        blk.bogus_colour_endpoints = true;
        if (!TEST_GENERATE_INVALID_BLOCKS)
            return;
    }

    blk.calculate_remaining_bits();

    // Specified as illegal
    if (blk.remaining_bits < (13 * blk.num_cem_values + 4) / 5) {
        blk.is_error = true;
        if (!TEST_GENERATE_INVALID_BLOCKS)
            return;
    }

    decode_error err = blk.calculate_colour_endpoints_size();
    if (err != decode_error::ok)
        blk.bogus_colour_endpoints = true;

    if (!TEST_GENERATE_INVALID_BLOCKS)
        ASSERT(!blk.is_error && !blk.bogus_colour_endpoints && !blk.bogus_weights);


    // Now we just need to pick some weights and colours.
    // We know the vector lengths and ranges, so just do random numbers in those ranges.

    if (!blk.bogus_weights) {
        memset(blk.weights_quant, 0, sizeof(blk.weights_quant));
        std::uniform_int_distribution<> weight_dist(0, blk.wt_max);

        ASSERT(blk.num_weights <= ARRAY_SIZE(blk.weights_quant));
        for (int i = 0; i < blk.num_weights; ++i)
            blk.weights_quant[i] = weight_dist(m_rng);

        blk.unquantise_weights();
    }

    if (!blk.bogus_colour_endpoints) {
        memset(blk.colour_endpoints_quant, 0, sizeof(blk.colour_endpoints_quant));
        std::uniform_int_distribution<> ce_dist(0, blk.ce_max);

        ASSERT(blk.num_cem_values <= ARRAY_SIZE(blk.colour_endpoints_quant));
        for (int i = 0; i < blk.num_cem_values; ++i)
            blk.colour_endpoints_quant[i] = ce_dist(m_rng);

        blk.unquantise_colour_endpoints();
    }

    if (blk.num_parts > 1) {
        // TODO: loop over some different partition indexes (probably not all of them)
        std::uniform_int_distribution<> part_dist(0, 1023);
        blk.partition_index = part_dist(m_rng);
    } else {
        blk.partition_index = -1;
    }

    if (VERBOSE_TEST) {
        printf("Test case:\n");
        blk.print();
        printf("\n");
    }

    OutputBitVector encoded = blk.encode(encoder);

    write_encoded_block(encoded);

    // Verify our encoder and decoder by checking that the decoded output
    // matches the data we tried to encode

    InputBitVector block;
    memcpy(block.data, encoded.data, sizeof(block.data));
    Block decoded;

    Decoder decoder(encoder.block_w, encoder.block_h, encoder.block_d);
    err = decoded.decode(decoder, block);
    if (blk.is_error) {
        ASSERT(err != decode_error::ok);

        if (VERBOSE_TEST) {
            printf("Decoded: error (as expected)\n");
        }
    } else {
        ASSERT(err == decode_error::ok);

        if (VERBOSE_TEST) {
            printf("Decoded:\n");
            decoded.print();
            printf("\n");
        }

        ASSERT(blk.high_prec == decoded.high_prec);
        ASSERT(blk.dual_plane == decoded.dual_plane);
        if (blk.dual_plane)
            ASSERT(blk.colour_component_selector == decoded.colour_component_selector);
        ASSERT(blk.wt_range == decoded.wt_range);
        ASSERT(blk.wt_w == decoded.wt_w);
        ASSERT(blk.wt_h == decoded.wt_h);
        ASSERT(blk.wt_d == decoded.wt_d);
        ASSERT(blk.num_parts == decoded.num_parts);
        if (blk.num_parts > 1)
            ASSERT(blk.partition_index == decoded.partition_index);
        ASSERT(blk.is_void_extent == decoded.is_void_extent);
        if (blk.is_void_extent) {
            // TODO: test
        }
        ASSERT(blk.is_multi_cem == decoded.is_multi_cem);
        ASSERT(blk.cem_base_class == decoded.cem_base_class);
        ASSERT(blk.cems[0] == decoded.cems[0]);
        ASSERT(blk.cems[1] == decoded.cems[1]);
        ASSERT(blk.cems[2] == decoded.cems[2]);
        ASSERT(blk.cems[3] == decoded.cems[3]);

        ASSERT(blk.num_weights == decoded.num_weights);

        for (int i = 0; i < blk.num_weights; ++i)
            ASSERT(blk.weights[i] == decoded.weights[i]);

        for (int i = 0; i < blk.num_cem_values; ++i)
            ASSERT(blk.colour_endpoints[i] == decoded.colour_endpoints[i]);

        ASSERT(blk.wt_w <= encoder.block_w);
        ASSERT(blk.wt_h <= encoder.block_h);
        ASSERT(blk.wt_d <= encoder.block_d);
        ASSERT(decoded.wt_w <= encoder.block_w);
        ASSERT(decoded.wt_h <= encoder.block_h);
        ASSERT(decoded.wt_d <= encoder.block_d);
    }

    ++m_count;

    if (m_count % 10000 == 0)
        fprintf(stderr, "%d...\n", m_count);
}

void TestGenerator::generate_with_block_mode(const Encoder &encoder, Block blk, int wt_w, int wt_h, int wt_d)
{
    // Specified as illegal
    if (wt_w > encoder.block_w || wt_h > encoder.block_h || wt_d > encoder.block_d) {
        blk.is_error = true;
        if (!TEST_GENERATE_INVALID_BLOCKS)
            return;
    }

    // Specified as illegal
    if (wt_w * wt_h * wt_d * (blk.dual_plane ? 2 : 1) > 64) {
        blk.is_error = true;
        blk.bogus_weights = true;
        if (!TEST_GENERATE_INVALID_BLOCKS)
            return;
    }

    blk.wt_w = wt_w;
    blk.wt_h = wt_h;
    blk.wt_d = wt_d;

    blk.calculate_from_weights();

    // Specified as illegal
    if (blk.weight_bits < 24) {
        blk.is_error = true;
        if (!TEST_GENERATE_INVALID_BLOCKS)
            return;
    }

    // Illegal, and we need to be careful not to write too many bits
    // since we'll corrupt the block mode fields
    if (blk.weight_bits > 96) {
        blk.is_error = true;
        blk.bogus_weights = true;
        if (!TEST_GENERATE_INVALID_BLOCKS)
            return;
    }

    for (int p = 1; p <= 4; ++p) {
        blk.num_parts = p;

        if (blk.dual_plane && p == 4 && !TEST_GENERATE_INVALID_BLOCKS)
            continue;

        for (int cem = 0; cem < 16; ++cem)
            generate_with_cems(encoder, blk, false, cem >> 2, cem, p > 1 ? cem : -1, p > 2 ? cem : -1, p > 3 ? cem : -1);

        if (blk.num_parts > 1) {
            for (int cem_base_class = 0; cem_base_class < 3; ++cem_base_class) {

                for (int c3 = 0; c3 < (p > 3 ? 8 : 1); ++c3)
                for (int c2 = 0; c2 < (p > 2 ? 8 : 1); ++c2)
                for (int c1 = 0; c1 < (p > 1 ? 8 : 1); ++c1)
                for (int c0 = 0; c0 < 8; ++c0)
                    generate_with_cems(encoder, blk, true, cem_base_class,
                            cem_base_class * 4 + c0,
                            p > 1 ? cem_base_class * 4 + c1 : -1,
                            p > 2 ? cem_base_class * 4 + c2 : -1,
                            p > 3 ? cem_base_class * 4 + c3 : -1);
            }
        }
    }
}

void TestGenerator::generate_with_block_size(const Encoder &encoder)
{
    // TODO: void extents
    // TODO: test illegal combinations

    Block blk;
    blk.is_error = false;
    blk.bogus_colour_endpoints = false;
    blk.bogus_weights = false;
    blk.is_void_extent = false;

    for (blk.dual_plane = 0; blk.dual_plane <= 1; ++blk.dual_plane) {
        int max_ccs = (blk.dual_plane ? 4 : 1);
        for (blk.colour_component_selector = 0; blk.colour_component_selector < max_ccs; ++blk.colour_component_selector) {
            for (blk.high_prec = 0; blk.high_prec <= 1; ++blk.high_prec) {
                for (blk.wt_range = 2; blk.wt_range < 8; ++blk.wt_range) {

                    if (encoder.block_d == 1) {
                        generate_with_block_mode(encoder, blk, 6, 10, 1);
                        generate_with_block_mode(encoder, blk, 10, 6, 1);

                        for (int b = 0; b < 4; ++b) {
                            for (int a = 0; a < 4; ++a) {
                                generate_with_block_mode(encoder, blk, b+4, a+2, 1);
                                generate_with_block_mode(encoder, blk, b+8, a+2, 1);
                                generate_with_block_mode(encoder, blk, a+2, b+8, 1);
                                if (b < 2) {
                                    generate_with_block_mode(encoder, blk, a+2, b+6, 1);
                                    generate_with_block_mode(encoder, blk, b+2, a+2, 1);
                                }
                                if (b == 0) {
                                    generate_with_block_mode(encoder, blk, 12, a+2, 1);
                                    generate_with_block_mode(encoder, blk, a+2, 12, 1);
                                }
                                if (blk.dual_plane == 0 && blk.high_prec == 0) {
                                    generate_with_block_mode(encoder, blk, a+6, b+6, 1);
                                }
                            }
                        }
                    } else {
                        generate_with_block_mode(encoder, blk, 6, 2, 2);
                        generate_with_block_mode(encoder, blk, 2, 6, 2);
                        generate_with_block_mode(encoder, blk, 2, 2, 6);

                        if (blk.dual_plane == 0 && blk.high_prec == 0) {
                            for (int b = 0; b < 4; ++b) {
                                for (int a = 0; a < 4; ++a) {
                                    generate_with_block_mode(encoder, blk, 6, b+2, a+2);
                                    generate_with_block_mode(encoder, blk, a+2, 6, b+2);
                                    generate_with_block_mode(encoder, blk, a+2, b+2, 6);
                                }
                            }
                        }

                        for (int c = 0; c < 4; ++c) {
                            for (int b = 0; b < 4; ++b) {
                                for (int a = 0; a < 4; ++a) {
                                    generate_with_block_mode(encoder, blk, a+2, b+2, c+2);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static bool generate_test_vectors()
{
    int block_sizes[][3] = {
        { 4, 4, 1 },
#if 1
        { 5, 4, 1 },
        { 5, 5, 1 },
        { 6, 5, 1 },
        { 6, 6, 1 },
        { 8, 5, 1 },
        { 8, 6, 1 },
        { 10, 5, 1 },
        { 10, 6, 1 },
        { 8, 8, 1 },
        { 10, 8, 1 },
        { 10, 10, 1 },
        { 12, 10, 1 },
        { 12, 12, 1 },
#endif
#if 0
        { 3, 3, 3 },
        { 4, 3, 3 },
        { 4, 4, 3 },
        { 4, 4, 4 },
        { 5, 4, 4 },
        { 5, 5, 4 },
        { 5, 5, 5 },
        { 6, 5, 5 },
        { 6, 6, 5 },
        { 6, 6, 6 },
#endif
    };

    // TODO: add a test with every shade of unorm16 colour, to test output conversions
    // (using void extents?)

    TestGenerator gen;

    gen.seed(1);

    for (int i = 0; i < ARRAY_SIZE(block_sizes); ++i) {
        oastc::Encoder encoder(block_sizes[i][0], block_sizes[i][1], block_sizes[i][2]);
        fprintf(stderr, "Block size %dx%dx%d (%d of %d)...\n",
                encoder.block_w, encoder.block_h, encoder.block_d,
                i+1, ARRAY_SIZE(block_sizes));
        gen.generate_with_block_size(encoder);
        if (!gen.write_output_file(encoder))
            return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    if (!generate_test_vectors())
        return -1;
    return 0;
}
