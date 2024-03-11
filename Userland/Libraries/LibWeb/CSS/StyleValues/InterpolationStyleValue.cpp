/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/CSS/StyleValues/InterpolationStyleValue.h>

namespace Web::CSS {

ValueComparingNonnullRefPtr<InterpolationStyleValue> InterpolationStyleValue::create(ValueComparingNonnullRefPtr<StyleValue const> from, ValueComparingNonnullRefPtr<StyleValue const> to, float delta)
{
    return adopt_ref(*new (nothrow) InterpolationStyleValue(move(from), move(to), delta));
}

String InterpolationStyleValue::to_string() const
{
    return MUST(String::formatted("calc({} + ({} - {}) * {})", m_from->to_string(), m_to->to_string(), m_from->to_string(), m_delta));
}

bool InterpolationStyleValue::properties_equal(InterpolationStyleValue const& other) const
{
    return m_from == other.m_from && m_to == other.m_to && m_delta == other.m_delta;
}

InterpolationStyleValue::InterpolationStyleValue(ValueComparingNonnullRefPtr<StyleValue const> from, ValueComparingNonnullRefPtr<StyleValue const> to, float delta)
    : StyleValueWithDefaultOperators(Type::Interpolation)
    , m_from(move(from))
    , m_to(move(to))
    , m_delta(delta)
{
}

}
