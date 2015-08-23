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

#ifndef INCLUDED_OASTC_FP16
#define INCLUDED_OASTC_FP16

#include "common.h"

namespace oastc
{

/**
 * IEEE-754 half-precision float
 */
struct fp16
{
    union
    {
        struct
        {
            uint16_t m : 10;
            uint16_t e : 5;
            uint16_t s : 1;
        };
        uint16_t u;
    };

    // If e == 0, this represents zero or a subnormal:
    //   (-1)^s * 2^-14 * 0.m
    // Otherwise this represents the number:
    //   (-1)^s * 2^(e-15) * 1.m
    // (We never encounter infinity/NaN)
    //
    // "1.m" means the concatenation of "1." and the 10 bits of m
    // into a fractional binary number.


    static fp16 one()
    {
        return { 0, 15, 0 };
    }

    static fp16 zero()
    {
        return { 0, 0, 0 };
    }

    /**
     * Convert 0.0 to 0x00, 1.0 to 0xff.
     * Values outside the range [0.0, 1.0] will give undefined results.
     */
    uint8_t to_unorm8()
    {
        // v = round_to_nearest(1.mmmmmmmmmm * 2^(e-15) * 255)
        //   = round_to_nearest((1.mmmmmmmmmm * 255) * 2^(e-15))
        //   = round_to_nearest((1mmmmmmmmmm * 255) * 2^(e-25))
        //   = round_to_zero((1mmmmmmmmmm * 255) * 2^(e-25) + 0.5)
        //   = round_to_zero(((1mmmmmmmmmm * 255) * 2^(e-24) + 1) / 2)
        //
        // This happens to give the correct answer for zero/subnormals too

        ASSERT(s == 0 && u <= one().u); // check 0 <= this <= 1
        // (implies e <= 15, which means the bit-shifts below are safe)

        uint32_t v = ((1 << 10) | m) * 255;
        v = ((v >> (24 - e)) + 1) >> 1;
        return v;
    }

    /**
     * Takes a uint16_t, divides by 65536, converts the infinite-precision
     * result to fp16 with round-to-zero.
     */
    static fp16 from_uint16_div_64k(uint16_t v)
    {
        if (v < 4) {
            // Zero or subnormal
            fp16 r = { (uint16_t)(v << 8), 0, 0 };
            return r;

        } else {
            // Count the leading 0s in the uint16_t
            int n = __builtin_clz(v) - (sizeof(unsigned int) - sizeof(uint16_t)) * 8;

            // Shift the mantissa up so bit 16 is the hidden 1 bit,
            // mask it off, then shift back down to 10 bits
            int m = ( ((uint32_t)v << (n + 1)) & 0xffff ) >> 6;

            //  (0{n} 1 X{15-n}) * 2^-16
            // = 1.X * 2^(15-n-16)
            // = 1.X * 2^(14-n - 15)
            // which is the FP16 form with e = 14 - n

            int e = 14 - n;

            ASSERT(e >= 1 && e <= 30);
            ASSERT(m >= 0 && m < 0x400);

            return { (uint16_t)m, (uint16_t)e, 0 };
        }
    }

    /**
     * Equivalent to fp16::from_uint16_div_64k(v).to_unorm8() but possibly faster
     */
    static uint8_t unorm8_from_uint16_div_64k(uint16_t v)
    {
        if (v == 0) {
            // __builtin_clz(0) is undefined, so handle v==0 specially
            return 0;

        } else {
            int n = __builtin_clz(v) - (sizeof(unsigned int) - sizeof(uint16_t)) * 8;

            uint32_t r = (((uint32_t)v << n) >> 5) * 255;
            r = ((r >> (10 + n)) + 1) >> 1;
            return r;
        }
    }
};

static_assert(sizeof(fp16) == 2, "fp16 must be 16 bits");

} // namespace oastc

#endif // INCLUDED_OASTC_FP16
