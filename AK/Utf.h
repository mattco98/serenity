/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

namespace AK {

constexpr size_t code_point_length(u32 code_point)
{
    if (code_point <= 0x7f)
        return 1;
    if (code_point <= 0x07ff)
        return 2;
    if (code_point <= 0xffff)
        return 3;
    if (code_point <= 0x10ffff)
        return 4;
    return 3;
}

template<typename Func>
constexpr void for_each_char_in_code_point(u32 code_point, Func&& func)
{
    if (code_point <= 0x7f) {
        func((char)code_point);
    } else if (code_point <= 0x07ff) {
        func((char)(((code_point >> 6) & 0x1f) | 0xc0));
        func((char)(((code_point >> 0) & 0x3f) | 0x80));
    } else if (code_point <= 0xffff) {
        func((char)(((code_point >> 12) & 0x0f) | 0xe0));
        func((char)(((code_point >> 6) & 0x3f) | 0x80));
        func((char)(((code_point >> 0) & 0x3f) | 0x80));
    } else if (code_point <= 0x10ffff) {
        func((char)(((code_point >> 18) & 0x07) | 0xf0));
        func((char)(((code_point >> 12) & 0x3f) | 0x80));
        func((char)(((code_point >> 6) & 0x3f) | 0x80));
        func((char)(((code_point >> 0) & 0x3f) | 0x80));
    } else {
        func((char)0xef);
        func((char)0xbf);
        func((char)0xbd);
    }
}

}
