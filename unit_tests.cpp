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

#include <iostream>

using namespace oastc;

static int test_failures = 0;
#define TEST_FAIL(msg) (++test_failures, std::cout << __FILE__ << ":" << __LINE__ << " Test failed: " << msg)
#define TEST_ASSERT_EQ(x, y) do { if ((x) != (y)) TEST_FAIL(#x " == " #y) << " (got " << (x) << ", expected " << (y) << ")\n"; } while (0)

static void test_get_bits()
{
    InputBitVector block;
    block.data[0] = 0x11223344;
    block.data[1] = 0x2468ace1;
    block.data[2] = 0x9abcdef0;
    block.data[3] = 0x12345678;
    TEST_ASSERT_EQ(block.get_bits(0, 28),   0x01223344);
    TEST_ASSERT_EQ(block.get_bits(4, 28),   0x01122334);
    TEST_ASSERT_EQ(block.get_bits(8, 28),   0x01112233);
    TEST_ASSERT_EQ(block.get_bits(28, 28),  0x068ace11);
    TEST_ASSERT_EQ(block.get_bits(32, 28),  0x0468ace1);
    TEST_ASSERT_EQ(block.get_bits(36, 28),  0x02468ace);
    TEST_ASSERT_EQ(block.get_bits(60, 28),  0x0bcdef02);
    TEST_ASSERT_EQ(block.get_bits(64, 28),  0x0abcdef0);
    TEST_ASSERT_EQ(block.get_bits(68, 28),  0x09abcdef);
    TEST_ASSERT_EQ(block.get_bits(92, 28),  0x03456789);
    TEST_ASSERT_EQ(block.get_bits(96, 28),  0x02345678);
    TEST_ASSERT_EQ(block.get_bits(100, 28), 0x01234567);
}

static void test_get_bits_rev()
{
    InputBitVector block;
    block.data[0] = 0b0111011010;
    TEST_ASSERT_EQ(block.get_bits_rev(10, 10),   0b0101101110);
    TEST_ASSERT_EQ(block.get_bits_rev(11, 10),   0b1011011100);
}

static void test_get_bits64()
{
    InputBitVector block;
    block.data[0] = 0x11223344;
    block.data[1] = 0x2468ace1;
    block.data[2] = 0x9abcdef0;
    block.data[3] = 0x12345678;
    TEST_ASSERT_EQ(block.get_bits64(0, 60),   0x0468ace111223344ull);
    TEST_ASSERT_EQ(block.get_bits64(4, 60),   0x02468ace11122334ull);
    TEST_ASSERT_EQ(block.get_bits64(8, 60),   0x002468ace1112233ull);
    TEST_ASSERT_EQ(block.get_bits64(28, 60),  0x0bcdef02468ace11ull);
    TEST_ASSERT_EQ(block.get_bits64(32, 60),  0x0abcdef02468ace1ull);
    TEST_ASSERT_EQ(block.get_bits64(36, 60),  0x09abcdef02468aceull);
    TEST_ASSERT_EQ(block.get_bits64(60, 60),  0x03456789abcdef02ull);
    TEST_ASSERT_EQ(block.get_bits64(64, 60),  0x023456789abcdef0ull);
    TEST_ASSERT_EQ(block.get_bits64(68, 60),  0x0123456789abcdefull);
    TEST_ASSERT_EQ(block.get_bits64(92, 60),  0x0000000123456789ull);
    TEST_ASSERT_EQ(block.get_bits64(96, 60),  0x0000000012345678ull);
    TEST_ASSERT_EQ(block.get_bits64(100, 60), 0x0000000001234567ull);
}

static void test_trits()
{
    uint8_t decoded[32];

    union {
        struct {
            uint64_t m0 : 4;
            uint64_t T0 : 1;
            uint64_t T1 : 1;
            uint64_t m1 : 4;
            uint64_t T2 : 1;
            uint64_t T3 : 1;
            uint64_t m2 : 4;
            uint64_t T4 : 1;
            uint64_t m3 : 4;
            uint64_t T5 : 1;
            uint64_t T6 : 1;
            uint64_t m4 : 4;
            uint64_t T7 : 1;
        } c;
        uint64_t u;
    } in;

    in.u = 0;
    in.c.m0 = 0x0;
    in.c.m1 = 0x1;
    in.c.m2 = 0x2;
    in.c.m3 = 0x3;
    in.c.m4 = 0x4;
    unpack_trit_block(4, in.u, decoded);
    TEST_ASSERT_EQ(decoded[0], 0x00);
    TEST_ASSERT_EQ(decoded[1], 0x01);
    TEST_ASSERT_EQ(decoded[2], 0x02);
    TEST_ASSERT_EQ(decoded[3], 0x03);
    TEST_ASSERT_EQ(decoded[4], 0x04);

    in.u = 0;
    in.c.m0 = 0x0;
    in.c.m1 = 0x1;
    in.c.m2 = 0x2;
    in.c.m3 = 0x3;
    in.c.m4 = 0x4;
    in.c.T4 = in.c.T3 = in.c.T2 = 1;
    unpack_trit_block(4, in.u, decoded);
    TEST_ASSERT_EQ(decoded[0], 0x00);
    TEST_ASSERT_EQ(decoded[1], 0x01);
    TEST_ASSERT_EQ(decoded[2], 0x02);
    TEST_ASSERT_EQ(decoded[3], 0x23);
    TEST_ASSERT_EQ(decoded[4], 0x24);
}

static void test_fp16()
{
    TEST_ASSERT_EQ(fp16::zero().u, 0x0000);
    TEST_ASSERT_EQ(fp16::one().u, 0x3c00);
}

static void test_fp16_unorm()
{
    for (int a = 0; a < 65536; ++a) {
        fp16 f0 = fp16::from_uint16_div_64k(a);
        uint8_t u0 = f0.to_unorm8();
        uint8_t u1 = fp16::unorm8_from_uint16_div_64k(a);
        TEST_ASSERT_EQ((int)u0, (int)u1);
    }
}

static void test()
{
    test_get_bits();
    test_get_bits64();
    test_get_bits_rev();
    test_trits();
    test_fp16();
    test_fp16_unorm();

    if (test_failures > 0)
        exit(-1);
    printf("All tests passed\n");
}


int main(int argc, char **argv)
{
    test();
    return 0;
}
