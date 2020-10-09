/*
 * Copyright (c) 2020, Matthew Olsson <matthewcolsson@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <AK/StringBuilder.h>
#include <LibGfx/Painter.h>
#include <LibWeb/CSS/StyleResolver.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/DOM/Event.h>
#include <LibWeb/Layout/LayoutSVGSVG.h>
#include <LibWeb/SVG/SVGPathElement.h>
#include <LibWeb/SVG/SVGSVGElement.h>
#include <ctype.h>

namespace Web::SVG {

SVGSVGElement::SVGSVGElement(DOM::Document& document, const FlyString& tag_name)
    : SVGGraphicsElement(document, tag_name)
{
}

RefPtr<LayoutNode> SVGSVGElement::create_layout_node(const CSS::StyleProperties* parent_style)
{
    auto style = document().style_resolver().resolve_style(*this, parent_style);
    if (style->display() == CSS::Display::None)
        return nullptr;
    return adopt(*new LayoutSVGSVG(document(), *this, move(style)));
}

void SVGSVGElement::parse_attribute(const FlyString& name, const String& value)
{
    SVGGraphicsElement::parse_attribute(name, value);

    if (name == "viewBox") {
        size_t i = 0;
        size_t property_index = 0;
        bool in_number = false;
        bool seen_dot = false;
        bool seen_comma = false;

        Optional<float> min_x;
        Optional<float> min_y;
        Optional<float> width;
        Optional<float> height;

        StringBuilder builder;

        auto try_append = [&]() {
            auto str = builder.to_string();
            float number = strtof(str.characters(), nullptr);

            switch (property_index) {
            case 0: min_x = number; break;
            case 1: min_y = number; break;
            case 2: width = number; break;
            case 3: height = number; break;
            }

            ++property_index;
        };

        while (i < value.length()) {
            char ch = value[i++];

            if (isspace(ch) || ch == ',') {
                if (ch == ',') {
                    if (seen_comma)
                        return;
                    seen_comma = true;
                }
                if (in_number) {
                    try_append();
                    builder.clear();
                    in_number = false;
                    seen_dot = false;
                    seen_comma = false;
                }
            } else if (ch == '-' || ch == '+') {
                if (in_number)
                    return;
                in_number = true;
                if (ch == '-')
                    builder.append('-');
            } else if (isdigit(ch)) {
                in_number = true;
                builder.append(ch);
            } else if (ch == '.') {
                if (seen_dot)
                    return;
                in_number = true;
                seen_dot = true;
                builder.append('.');
            }
        }

        if (!builder.is_empty())
            try_append();

        m_viewbox.min_x = min_x;
        m_viewbox.min_y = min_y;

        if (width.has_value() && width.value() > 0.0)
            m_viewbox.width = width;

        if (height.has_value() && height.value() > 0.0)
            m_viewbox.height = height;
    } else if (name == "preserveAspectRatio") {
        auto parts = value.view().split_view(' ');
        if (parts.is_empty())
            return;

        auto align = parts[0];
        if (align == "xMinYMin") {
            m_align_mode = AlignMode::XMinYMin;
        } else if (align == "xMidYMin") {
            m_align_mode = AlignMode::XMidYMin;
        } else if (align == "xMaxYMin") {
            m_align_mode = AlignMode::XMaxYMin;
        } else if (align == "xMinYMid") {
            m_align_mode = AlignMode::XMinYMid;
        } else if (align == "xMidYMid") {
            m_align_mode = AlignMode::XMidYMid;
        } else if (align == "xMaxYMid") {
            m_align_mode = AlignMode::XMaxYMid;
        } else if (align == "xMinYMax") {
            m_align_mode = AlignMode::XMinYMax;
        } else if (align == "xMidYMax") {
            m_align_mode = AlignMode::XMidYMax;
        } else if (align == "xMaxYMax") {
            m_align_mode = AlignMode::XMaxYMax;
        } else {
            m_align_mode = AlignMode::None;
        }

        if (align != "none" && parts.size() > 1) {
            if (parts[1] == "slice") {
                m_aspect_ratio_mode = AspectRatioMode::Slice;
            }
        }
    }
}

unsigned SVGSVGElement::width() const
{
    return attribute(HTML::AttributeNames::width).to_uint().value_or(300);
}

unsigned SVGSVGElement::height() const
{
    return attribute(HTML::AttributeNames::height).to_uint().value_or(150);
}

const Gfx::AffineTransform& SVGSVGElement::calculate_transformations(Gfx::FloatRect bounding_box)
{
    if (m_transformations.has_value())
        return m_transformations.value();

    float scale_x = 1.0;
    float scale_y = 1.0;

    if (m_viewbox.width.has_value())
        scale_x = bounding_box.width() / m_viewbox.width.value();

    if (m_viewbox.height.has_value())
        scale_y = bounding_box.height() / m_viewbox.height.value();

    if (m_align_mode != AlignMode::None) {
        if (m_aspect_ratio_mode == AspectRatioMode::Meet) {
            auto min_scale = min(scale_x, scale_y);
            scale_x = min_scale;
            scale_y = min_scale;
        } else {
            auto max_scale = max(scale_x, scale_y);
            scale_x = max_scale;
            scale_y = max_scale;
        }
    }

    float translate_x = bounding_box.x();
    float translate_y = bounding_box.y();

    if (m_viewbox.min_x.has_value())
        translate_x -= m_viewbox.min_x.value() * scale_x;

    if (m_viewbox.min_y.has_value())
        translate_y -= m_viewbox.min_y.value() * scale_y;

    if (m_viewbox.width.has_value() && (m_align_mode == AlignMode::XMidYMin || m_align_mode == AlignMode::XMidYMid || m_align_mode == AlignMode::XMidYMax))
        translate_x += (bounding_box.width() - m_viewbox.width.value() * scale_x) / 2.0;

    if (m_viewbox.width.has_value() && (m_align_mode == AlignMode::XMaxYMin || m_align_mode == AlignMode::XMaxYMid || m_align_mode == AlignMode::XMaxYMax))
        translate_x += (bounding_box.width() - m_viewbox.width.value() * scale_x);

    if (m_viewbox.height.has_value() && (m_align_mode == AlignMode::XMinYMid || m_align_mode == AlignMode::XMidYMid || m_align_mode == AlignMode::XMaxYMid))
        translate_y += (bounding_box.height() - m_viewbox.height.value() * scale_y) / 2.0;

    if (m_viewbox.height.has_value() && (m_align_mode == AlignMode::XMinYMax || m_align_mode == AlignMode::XMidYMax || m_align_mode == AlignMode::XMaxYMax))
        translate_y += (bounding_box.height() - m_viewbox.height.value() * scale_y);

    m_transformations = Gfx::AffineTransform().translate(translate_x, translate_y).scale(scale_x, scale_y);

    return m_transformations.value();
}

}
