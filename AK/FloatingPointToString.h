/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/StringView.h>

namespace AK {

constexpr StringView float_to_string(float);
// constexpr StringView double_to_string(double);
// constexpr StringView long_double_to_string(long double);

template<typename T>
constexpr StringView floating_point_to_string(T number) requires(IsFloatingPoint<T>)
{
    if constexpr (IsSame<T, float>)
        return float_to_string(number);
    // if constexpr (IsSame<T, double>)
    //     return double_to_string(number);
    // if constexpr (IsSame<T, long double>)
    //     return long_double_to_string(number);

    VERIFY_NOT_REACHED();
}

}

using AK::floating_point_to_string;
