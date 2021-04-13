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

#include <AK/GenericLexer.h>
#include <LibGfx/Painter.h>
#include <LibWeb/CSS/StyleResolver.h>
#include <LibWeb/DOM/Document.h>
#include <LibWeb/DOM/Event.h>
#include <LibWeb/Layout/SVGSVGBox.h>
#include <LibWeb/SVG/SVGPathElement.h>
#include <LibWeb/SVG/SVGSVGElement.h>
#include <ctype.h>

namespace Web::SVG {

class SVGElementLexer final : public GenericLexer {
public:
    explicit SVGElementLexer(const StringView& input)
        : GenericLexer(input)
    {
    }

    Optional<SVGSVGElement::ViewBox> parse_view_box()
    {
        Vector<int> numbers;

        while (numbers.size() < 4) {
            auto int_optional = parse_integer();
            if (!int_optional.has_value())
                return {};
            numbers.append(int_optional.value());

            if (is_eof())
                break;

            auto space = consume_while(isspace);
            if (space.is_null() || space.is_empty())
                break;
        }

        // FIXME: Are viewbox elements supposed to have default values?
        if (numbers.size() != 4)
            return {};

        return SVGSVGElement::ViewBox { numbers[0], numbers[1], numbers[2], numbers[3] };
    }

    SVGSVGElement::PreserveAspectRatio parse_preserve_aspect_ratio()
    {
        SVGSVGElement::PreserveAspectRatio preserve_aspect_ratio {
            SVGSVGElement::Align::None,
            SVGSVGElement::MeetOrSlice::Meet
        };

        auto align = parse_align();
        if (!align.has_value())
            return preserve_aspect_ratio;

        preserve_aspect_ratio.align = align.value();
        auto space = consume_while(isspace);
        if (space.is_null() || space.is_empty())
            return preserve_aspect_ratio;

        auto meet_or_slice = parse_meet_or_slice();
        if (!meet_or_slice.has_value())
            return preserve_aspect_ratio;

        preserve_aspect_ratio.meet_or_slice = meet_or_slice.value();
        return preserve_aspect_ratio;
    }

private:
    Optional<int> parse_integer()
    {
        auto string = consume_while(isdigit);
        if (string.is_null() || string.is_empty())
            return {};

        return string.to_int();
    }

    Optional<SVGSVGElement::Align> parse_align()
    {
        auto string = consume_while(isalpha);
        if (string == "xMinYMin")
            return SVGSVGElement::Align::XMinYMin;
        if (string == "xMidYMin")
            return SVGSVGElement::Align::XMidYMin;
        if (string == "xMaxYMin")
            return SVGSVGElement::Align::XMaxYMin;
        if (string == "xMinYMid")
            return SVGSVGElement::Align::XMinYMid;
        if (string == "xMidYMid")
            return SVGSVGElement::Align::XMidYMid;
        if (string == "xMaxYMid")
            return SVGSVGElement::Align::XMaxYMid;
        if (string == "xMinYMax")
            return SVGSVGElement::Align::XMinYMax;
        if (string == "xMidYMax")
            return SVGSVGElement::Align::XMidYMax;
        if (string == "xMaxYMax")
            return SVGSVGElement::Align::XMaxYMax;

        return {};
    }

    Optional<SVGSVGElement::MeetOrSlice> parse_meet_or_slice()
    {
        auto string = consume_while(isalpha);
        if (string == "meet")
            return SVGSVGElement::MeetOrSlice::Meet;
        if (string == "slice")
            return SVGSVGElement::MeetOrSlice::Slice;

        return {};
    }
};

SVGSVGElement::SVGSVGElement(DOM::Document& document, QualifiedName qualified_name)
    : SVGGraphicsElement(document, qualified_name)
{
}

RefPtr<Layout::Node> SVGSVGElement::create_layout_node()
{
    auto style = document().style_resolver().resolve_style(*this);
    if (style->display() == CSS::Display::None)
        return nullptr;
    return adopt(*new Layout::SVGSVGBox(document(), *this, move(style)));
}

unsigned SVGSVGElement::width() const
{
    return attribute(HTML::AttributeNames::width).to_uint().value_or(300);
}

unsigned SVGSVGElement::height() const
{
    return attribute(HTML::AttributeNames::height).to_uint().value_or(150);
}

Optional<SVGSVGElement::ViewBox> SVGSVGElement::view_box() const
{
    auto string = attribute(HTML::AttributeNames::viewBox);
    return SVGElementLexer(string).parse_view_box();
}

SVGSVGElement::PreserveAspectRatio SVGSVGElement::preserve_aspect_ratio() const
{
    auto string = attribute(HTML::AttributeNames::preserveAspectRatio);
    return SVGElementLexer(string).parse_preserve_aspect_ratio();
}

}
