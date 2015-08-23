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

#ifndef INCLUDED_OASTC_COMMON
#define INCLUDED_OASTC_COMMON

#include <iostream>

#if 0 // the asserts don't hugely affect performance so might as well leave them enabled

#define ASSERT(expr) (void)(expr)
#define UNREACHABLE() __builtin_unreachable()

#else

#define ASSERT(expr) do { \
    if (!(expr)) { \
        std::cerr << __FILE__ << ":" << __LINE__ << " Assertion failed: " << #expr << "\n"; abort(); \
    } \
    } while (0)

#define UNREACHABLE() do { \
    std::cerr << __FILE__ << ":" << __LINE__ << " Hit unreachable code\n"; abort(); \
    } while (0)

#endif

#define ARRAY_SIZE(x) (int)(sizeof(x) / sizeof((x)[0]))

#endif // INCLUDED_OASTC_COMMON
