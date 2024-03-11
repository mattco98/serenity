/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibWeb/CSS/StyleValue.h>

namespace Web::CSS {

class InterpolationStyleValue : public StyleValueWithDefaultOperators<InterpolationStyleValue> {
public:
    static ValueComparingNonnullRefPtr<InterpolationStyleValue> create(ValueComparingNonnullRefPtr<StyleValue const> from, ValueComparingNonnullRefPtr<StyleValue const> to, float delta);
    virtual ~InterpolationStyleValue() override = default;

    ValueComparingNonnullRefPtr<StyleValue const> from() const { return m_from; }
    ValueComparingNonnullRefPtr<StyleValue const> to() const { return m_to; }
    float delta() const { return m_delta; }

    virtual String to_string() const override;
    bool properties_equal(InterpolationStyleValue const&) const;

private:
    InterpolationStyleValue(ValueComparingNonnullRefPtr<StyleValue const> from, ValueComparingNonnullRefPtr<StyleValue const> to, float delta);

    ValueComparingNonnullRefPtr<StyleValue const> m_from;
    ValueComparingNonnullRefPtr<StyleValue const> m_to;
    float m_delta;
};

}
