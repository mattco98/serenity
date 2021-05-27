/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Format.h>
#include <LibGfx/AffineTransform.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/Font.h>
#include <LibGfx/FontDatabase.h>
#include <LibGfx/Painter.h>
#include <LibGfx/Path.h>
#include <LibGfx/Point.h>
#include <LibGfx/Rect.h>
#include <LibGfx/Size.h>
#include <LibPDF/Test.h>
#include <LibPDF/Document.h>
#include <LibPDF/Object.h>
#include <LibPDF/Value.h>

#define ENUMERATE_COLOR_SPACES(V) \
    V(DeviceGray)                 \
    V(DeviceRGB)                  \
    V(DeviceCMYK)                 \
    V(CalGray)                    \
    V(CalRGB)                     \
    V(Lab)                        \
    V(ICCBased)                   \
    V(Indexed)                    \
    V(Pattern)                    \
    V(Separation)                 \
    V(DeviceN)

namespace PDF {

class ColorSpace {
public:
    enum class Type {
#define ENUM(name) name,
        ENUMERATE_COLOR_SPACES(ENUM)
#undef ENUM
    };

    static Optional<ColorSpace::Type> color_space_from_string(const FlyString& str)
    {
#define ENUM(name)    \
    if (str == #name) \
        return ColorSpace::Type::name;
        ENUMERATE_COLOR_SPACES(ENUM)
#undef ENUM

        return {};
    }

    ColorSpace(ColorSpace::Type type)
        : ColorSpace(type, {})
    {
    }

    ColorSpace(ColorSpace::Type type, Vector<Value> parameters)
        : m_type(type)
        , m_parameters(move(parameters))
    {
    }

    ALWAYS_INLINE ColorSpace::Type type() const { return m_type; }
    ALWAYS_INLINE const Vector<Value>& parameters() const { return m_parameters; }

    Color default_color() const
    {
        switch (m_type) {
        case Type::DeviceGray:
        case Type::DeviceRGB:
            return Color::NamedColor::Black;
        case Type::DeviceCMYK:
            return Color::from_cmyk(1.0f, 1.0f, 1.0f, 0.0f);
        case Type::CalRGB:

        default:
                TODO();
        }
    }

    Color color(const Vector<Value>& arguments) const
    {
        switch (m_type) {
        case Type::DeviceGray: {
                VERIFY(arguments.size() == 1);
            auto gray = static_cast<u8>(arguments[0].to_float() * 255.0f);
            return Color(gray, gray, gray);
        }
        case Type::DeviceRGB: {
                VERIFY(arguments.size() == 3);
            auto r = static_cast<u8>(arguments[0].to_float() * 255.0f);
            auto g = static_cast<u8>(arguments[1].to_float() * 255.0f);
            auto b = static_cast<u8>(arguments[2].to_float() * 255.0f);
            return Color(r, g, b);
        }
        case Type::DeviceCMYK: {
                VERIFY(arguments.size() == 4);
            auto c = arguments[0].to_float();
            auto m = arguments[1].to_float();
            auto y = arguments[2].to_float();
            auto k = arguments[3].to_float();
            return Color::from_cmyk(c, m, y, k);
        }
        default:
                TODO();
        }
    }

private:
    ColorSpace::Type m_type;
    Vector<Value> m_parameters;
};

enum class LineCapStyle : u8 {
    ButtCap = 0,
    RoundCap = 1,
    SquareCap = 2,
};

enum class LineJoinStyle : u8 {
    Miter = 0,
    Round = 1,
    Bevel = 2,
};

struct LineDashPattern {
    Vector<int> pattern;
    int phase;
};

enum class TextRenderingMode : u8 {
    Fill = 0,
    Stroke = 1,
    FillThenStroke = 2,
    Invisible = 3,
    FillAndClip = 4,
    StrokeAndClip = 5,
    FillStrokeAndClip = 6,
    Clip = 7,
};

struct TextState {
    float character_spacing { 3.0f };
    float word_spacing { 5.0f };
    float horizontal_scaling { 1.0f };
    float leading { 0.0f };
    RefPtr<Gfx::Font> font;
    TextRenderingMode rendering_mode { TextRenderingMode::Fill };
    float rise { 0.0f };
    bool knockout { true };
};

struct GraphicsState {
    Gfx::AffineTransform ctm;
    ColorSpace stroke_color_space { ColorSpace::Type::DeviceGray };
    ColorSpace paint_color_space { ColorSpace::Type::DeviceGray };
    Gfx::Color stroke_color { Gfx::Color::NamedColor::Black };
    Gfx::Color paint_color { Gfx::Color::NamedColor::Black };
    float line_width { 1.0f };
    LineCapStyle line_cap_style { LineCapStyle::ButtCap };
    LineJoinStyle line_join_style { LineJoinStyle::Miter };
    float miter_limit { 10.0f };
    LineDashPattern line_dash_pattern { {}, 0 };
    TextState text_state {};
};

class Renderer {
public:
    static void render(Document&, const Page&, RefPtr<Gfx::Bitmap>);

private:
    Renderer(RefPtr<Document>, const Page&, RefPtr<Gfx::Bitmap>);

    void render();

    void handle_command(const Command&);
#define V(name, snake_name, symbol) \
    void handle_##snake_name(const Vector<Value>& args);
    ENUMERATE_COMMANDS(V)
#undef V
    void handle_text_next_line_show_string(const Vector<Value>& args);
    void handle_text_next_line_show_string_set_spacing(const Vector<Value>& args);

    // shift is the manual advance given in the TJ command array
    void show_text(const String&, int shift = 0);
    ColorSpace get_color_space(const Value&);

    ALWAYS_INLINE const GraphicsState& state() const { return m_graphics_state_stack.last(); }
    ALWAYS_INLINE GraphicsState& state() { return m_graphics_state_stack.last(); }
    ALWAYS_INLINE const TextState& text_state() const { return state().text_state; }
    ALWAYS_INLINE TextState& text_state() { return state().text_state; }

    template<typename T>
    ALWAYS_INLINE Gfx::Point<T> map(T x, T y) const;

    template<typename T>
    ALWAYS_INLINE Gfx::Size<T> map(Gfx::Size<T>) const;

    template<typename T>
    ALWAYS_INLINE Gfx::Rect<T> map(Gfx::Rect<T>) const;

    const Gfx::AffineTransform& calculate_text_rendering_matrix();

    RefPtr<Document> m_document;
    RefPtr<Gfx::Bitmap> m_bitmap;
    const Page& m_page;
    Gfx::Painter m_painter;

    Gfx::Path m_current_path;
    Vector<GraphicsState> m_graphics_state_stack;
    Gfx::AffineTransform m_text_matrix;
    Gfx::AffineTransform m_text_line_matrix;
    Gfx::AffineTransform m_userspace_matrix;

    bool m_text_rendering_matrix_is_dirty { true };
    Gfx::AffineTransform m_text_rendering_matrix;
};

}

namespace AK {

template<>
struct Formatter<PDF::LineCapStyle> : Formatter<StringView> {
    void format(FormatBuilder& builder, const PDF::LineCapStyle& style)
    {
        switch (style) {
        case PDF::LineCapStyle::ButtCap:
            Formatter<StringView>::format(builder, "LineCapStyle::ButtCap");
            break;
        case PDF::LineCapStyle::RoundCap:
            Formatter<StringView>::format(builder, "LineCapStyle::RoundCap");
            break;
        case PDF::LineCapStyle::SquareCap:
            Formatter<StringView>::format(builder, "LineCapStyle::SquareCap");
            break;
        }
    }
};

template<>
struct Formatter<PDF::LineJoinStyle> : Formatter<StringView> {
    void format(FormatBuilder& builder, const PDF::LineJoinStyle& style)
    {
        switch (style) {
        case PDF::LineJoinStyle::Miter:
            Formatter<StringView>::format(builder, "LineJoinStyle::Miter");
            break;
        case PDF::LineJoinStyle::Round:
            Formatter<StringView>::format(builder, "LineJoinStyle::Round");
            break;
        case PDF::LineJoinStyle::Bevel:
            Formatter<StringView>::format(builder, "LineJoinStyle::Bevel");
            break;
        }
    }
};

template<>
struct Formatter<PDF::LineDashPattern> : Formatter<StringView> {
    void format(FormatBuilder& format_builder, const PDF::LineDashPattern& pattern)
    {
        StringBuilder builder;
        builder.append("[");
        bool first = true;

        for (auto& i : pattern.pattern) {
            if (!first)
                builder.append(", ");
            first = false;
            builder.appendff("{}", i);
        }

        builder.appendff("] {}", pattern.phase);
        return Formatter<StringView>::format(format_builder, builder.to_string());
    }
};

template<>
struct Formatter<PDF::TextRenderingMode> : Formatter<StringView> {
    void format(FormatBuilder& builder, const PDF::TextRenderingMode& style)
    {
        switch (style) {
        case PDF::TextRenderingMode::Fill:
            Formatter<StringView>::format(builder, "TextRenderingMode::Fill");
            break;
        case PDF::TextRenderingMode::Stroke:
            Formatter<StringView>::format(builder, "TextRenderingMode::Stroke");
            break;
        case PDF::TextRenderingMode::FillThenStroke:
            Formatter<StringView>::format(builder, "TextRenderingMode::FillThenStroke");
            break;
        case PDF::TextRenderingMode::Invisible:
            Formatter<StringView>::format(builder, "TextRenderingMode::Invisible");
            break;
        case PDF::TextRenderingMode::FillAndClip:
            Formatter<StringView>::format(builder, "TextRenderingMode::FillAndClip");
            break;
        case PDF::TextRenderingMode::StrokeAndClip:
            Formatter<StringView>::format(builder, "TextRenderingMode::StrokeAndClip");
            break;
        case PDF::TextRenderingMode::FillStrokeAndClip:
            Formatter<StringView>::format(builder, "TextRenderingMode::FillStrokeAndClip");
            break;
        case PDF::TextRenderingMode::Clip:
            Formatter<StringView>::format(builder, "TextRenderingMode::Clip");
            break;
        }
    }
};

template<>
struct Formatter<PDF::TextState> : Formatter<StringView> {
    void format(FormatBuilder& format_builder, const PDF::TextState& state)
    {
        StringBuilder builder;
        builder.append("TextState {\n");
        builder.appendff("    character_spacing={}\n", state.character_spacing);
        builder.appendff("    word_spacing={}\n", state.word_spacing);
        builder.appendff("    horizontal_scaling={}\n", state.horizontal_scaling);
        builder.appendff("    leading={}\n", state.leading);
        builder.appendff("    font={}\n", state.font ? state.font->name() : "<null>");
        builder.appendff("    rendering_mode={}\n", state.rendering_mode);
        builder.appendff("    rise={}\n", state.rise);
        builder.appendff("    knockout={}\n", state.knockout);
        builder.append(" }");
        Formatter<StringView>::format(format_builder, builder.to_string());
    }
};

template<>
struct Formatter<PDF::GraphicsState> : Formatter<StringView> {
    void format(FormatBuilder& format_builder, const PDF::GraphicsState& state)
    {
        StringBuilder builder;
        builder.append("GraphicsState {\n");
        builder.appendff("  ctm={}\n", state.ctm);
        builder.appendff("  stroke_color={}\n", state.stroke_color);
        builder.appendff("  paint_color={}\n", state.paint_color);
        builder.appendff("  line_width={}\n", state.line_width);
        builder.appendff("  line_cap_style={}\n", state.line_cap_style);
        builder.appendff("  line_join_style={}\n", state.line_join_style);
        builder.appendff("  miter_limit={}\n", state.miter_limit);
        builder.appendff("  line_dash_pattern={}\n", state.line_dash_pattern);
        builder.appendff("  text_state={}\n", state.text_state);
        builder.append("}");
        Formatter<StringView>::format(format_builder, builder.to_string());
    }
};

}
