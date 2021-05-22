/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "math.h"
#include <AK/FloatExtractor.h>
#include <AK/FloatingPointToString.h>

namespace AK {

struct DecIntervalRep {
    u32 d0;
    u32 e0;
};

constexpr DecIntervalRep compute_shortest(u32 a, u32 b, u32 c, bool accept_smaller, bool accept_larger, bool break_tie_down)
{
    u32 i = 0;

    if (!accept_larger)
        c--;

    bool all_a_zero = true;
    u32 digit = 0;
    bool all_b_zero = true;

    while (floor(a / 10) < floor(c / 10)) {
        all_a_zero = all_a_zero && (a % 10) == 0;
        a = floor(a / 10);
        c = floor(c / 10);
        digit = b % 10;
        all_b_zero = all_b_zero && digit == 0;
        b = floor(b / 10);
        i++;
    }

    if (accept_smaller && all_a_zero) {
        while (a % 10 == 0) {
            a = a / 10;
            c = floor(c / 10);
            digit = b % 10;
            all_b_zero = all_b_zero && digit == 0;
            b = floor(b / 10);
            i++;
        }
    }

    bool is_tie = digit == 5 && all_b_zero;
    bool want_round_down = digit < 5 || (is_tie && break_tie_down);
    bool round_down = (want_round_down && (a != b || all_a_zero)) || b + 1 > c;

    return { b + (round_down ? 0 : 1), i };
}

constexpr void float_to_string(float number)
{
    // Step 1. Decode the floating point number, and unify normalized and subnormal cases

    using Extractor = FloatExtractor<float>;

    constexpr auto len_m = Extractor::mantissa_bits;
    constexpr auto exp_bias = Extractor::exponent_bias;

    Extractor extractor;
    extractor.number = number;

    u32 s = extractor.sign;
    u32 m = extractor.mantissa;
    u32 e = extractor.exponent;

    u32 mf;
    u32 ef;

    if (e == 0) {
        mf = m;
        ef = 1 - exp_bias - len_m;
    } else {
        mf = (2 << len_m) + m;
        ef = e - exp_bias - len_m;
    }

    // Step 2. Determine the interval of information-preserving outputs

    u32 e2 = ef - 2;

    // Can these overflow?
    u32 v = 4 * mf;
    u32 w = v + 2;
    u32 u = v - 1;

    if (m != 0 || e <= 1)
        u--;

    // Step 3. Convert (u, v, w) * 2 ^ e2 to a decimal power base such that
    //         (a, b, c) * 10 ^ e10 == (u, v, w) * 2 ^ e2

    u32 e10;
    u32 a;
    u32 b;
    u32 c;

    if (e2 >= 0) {
        e10 = 0;

    } else {
        e10 = e2;
    }
}

}
