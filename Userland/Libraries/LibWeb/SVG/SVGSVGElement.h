/*
 * Copyright (c) 2020, Matthew Olsson <matthewcolsson@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibGfx/Bitmap.h>
#include <LibWeb/SVG/SVGGraphicsElement.h>

namespace Web::SVG {

class SVGSVGElement final : public SVGGraphicsElement {
public:
    enum class Align {
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

    enum MeetOrSlice {
        Meet,
        Slice,
    };

    struct PreserveAspectRatio {
        Align align;
        MeetOrSlice meet_or_slice;
    };

    struct ViewBox {
        int min_x;
        int min_y;
        int width;
        int height;
    };

    using WrapperType = Bindings::SVGSVGElementWrapper;

    SVGSVGElement(DOM::Document&, QualifiedName);

    virtual RefPtr<Layout::Node> create_layout_node() override;

    unsigned width() const;
    unsigned height() const;
    Optional<ViewBox> view_box() const;
    PreserveAspectRatio preserve_aspect_ratio() const;

private:
    RefPtr<Gfx::Bitmap> m_bitmap;
};

}
