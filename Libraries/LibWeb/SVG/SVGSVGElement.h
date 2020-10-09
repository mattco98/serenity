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

#pragma once

#include <LibGfx/AffineTransform.h>
#include <LibGfx/Bitmap.h>
#include <LibWeb/SVG/SVGGraphicsElement.h>

namespace Web::SVG {

class SVGSVGElement final : public SVGGraphicsElement {
public:
    using WrapperType = Bindings::SVGSVGElementWrapper;

    enum class AlignMode {
        None,
        XMinYMin,
        XMidYMin,
        XMaxYMin,
        XMinYMid,
        XMidYMid,
        XMaxYMid,
        XMinYMax,
        XMidYMax,
        XMaxYMax,
    };

    enum class AspectRatioMode {
        Meet,
        Slice,
    };

    struct ViewBox {
        Optional<float> min_x;
        Optional<float> min_y;
        Optional<float> width;
        Optional<float> height;
    };

    SVGSVGElement(DOM::Document&, const FlyString& tag_name);

    RefPtr<LayoutNode> create_layout_node(const CSS::StyleProperties* parent_style) override;
    void parse_attribute(const FlyString& name, const String& value) override;

    unsigned width() const;
    unsigned height() const;
    const Gfx::AffineTransform& calculate_transformations(Gfx::FloatRect bounding_box);

    AlignMode align_mode() const { return m_align_mode; }
    AspectRatioMode aspect_ratio_mode() const { return m_aspect_ratio_mode; }
    ViewBox viewbox() const { return m_viewbox; }

private:
    RefPtr<Gfx::Bitmap> m_bitmap;

    AlignMode m_align_mode { AlignMode::XMidYMid };
    AspectRatioMode m_aspect_ratio_mode { AspectRatioMode::Meet };
    ViewBox m_viewbox;
    Optional<Gfx::AffineTransform> m_transformations;
};

}

AK_BEGIN_TYPE_TRAITS(Web::SVG::SVGSVGElement)
static bool is_type(const Web::DOM::Node& node) { return node.is_svg_element() && downcast<Web::SVG::SVGElement>(node).local_name() == Web::SVG::TagNames::svg; }
AK_END_TYPE_TRAITS()
