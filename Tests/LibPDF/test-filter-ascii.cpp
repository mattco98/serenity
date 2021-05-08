/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Span.h>
#include <LibPDF/Filter.h>
#include <LibTest/TestCase.h>

template<typename... Args>
bool buffer_equal(const ByteBuffer& actual, Args... expected)
{
    if (actual.size() != sizeof...(Args))
        return false;

    size_t i = 0;
    return ((actual[i++] == expected) && ...);
}

TEST_CASE(ascii_hex_decode)
{
    static constexpr auto encoding = "ASCIIHexDecode";

    {
        Array<u8, 8> data {
            '5',
            '1',
            'a',
            '8',
            '4',
            'b',
            'c',
            '7',
        };
        auto result = PDF::Filter::decode(data, encoding);
        EXPECT(buffer_equal(result.value(), 0x51, 0xa8, 0x4b, 0xc7));
    }

    {
        Array<u8, 7> data {
            '5',
            '1',
            'a',
            '8',
            '4',
            'b',
            'c',
        };
        auto result = PDF::Filter::decode(data, encoding);
        EXPECT(buffer_equal(result.value(), 0x51, 0xa8, 0x4b, 0xc0));
    }
}

TEST_CASE(ascii85_decode)
{
    static constexpr auto encoding = "ASCII85Decode";

    {
        Array<u8, 9> data = {
            'B',
            'O',
            'u',
            ' ',
            ' ',
            '!',
            'r',
            'D',
            'Z',
        };
        auto result = PDF::Filter::decode(data, encoding);
        EXPECT(buffer_equal(result.value(), 'h', 'e', 'l', 'l', 'o'));
    }

    {
        Array<u8, 10> data = {
            'B',
            'O',
            'u',
            ' ',
            ' ',
            '!',
            'r',
            'z',
            'D',
            'Z',
        };
        auto result = PDF::Filter::decode(data, encoding);
        EXPECT(buffer_equal(result.value(), 'h', 'e', 'l', 'l', 0, 0, 0, 0, 'o'));
    }
}
