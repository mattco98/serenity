/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
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

#include <AK/LogStream.h>
#include <AK/Optional.h>
#include <LibGfx/AffineTransform.h>
#include <LibGfx/Rect.h>

namespace Gfx {

bool AffineTransform::is_identity() const
{
    return a() == 1.0 && b() == 0.0 && c() == 0.0 && d() == 1.0 && tx() == 0.0 && ty() == 0.0;
}

static float hypotenuse(float x, float y)
{
    // FIXME: This won't handle overflow :(
    return sqrt(x * x + y * y);
}

float AffineTransform::x_scale() const
{
    return hypotenuse(m_values[0], m_values[1]);
}

float AffineTransform::y_scale() const
{
    return hypotenuse(m_values[2], m_values[3]);
}

template<>
AffineTransform& AffineTransform::translate(int tx, int ty)
{
    m_values[4] += tx * m_values[0] + ty * m_values[2];
    m_values[5] += tx * m_values[1] + ty * m_values[3];
    return *this;
}

template<>
AffineTransform& AffineTransform::translate(float tx, float ty)
{
    m_values[4] += tx * m_values[0] + ty * m_values[2];
    m_values[5] += tx * m_values[1] + ty * m_values[3];
    return *this;
}

template<>
AffineTransform& AffineTransform::translate(const IntPoint& translation)
{
    return translate(translation.x(), translation.y());
}

template<>
AffineTransform& AffineTransform::translate(const FloatPoint& translation)
{
    return translate(translation.x(), translation.y());
}

template<>
AffineTransform& AffineTransform::rotate_radians(int radians)
{
    float sin_angle = sinf(static_cast<float>(radians));
    float cos_angle = cosf(static_cast<float>(radians));
    AffineTransform rotation(cos_angle, sin_angle, -sin_angle, cos_angle, 0, 0);
    multiply(rotation);
    return *this;
}

template<>
AffineTransform& AffineTransform::rotate_radians(float radians)
{
    float cos_angle = cosf(radians);
    float sin_angle = sinf(radians);

    AffineTransform result;
    result.m_values[0] = a() * cos_angle + b() * sin_angle;
    result.m_values[1] = b() * cos_angle - a() * sin_angle;
    result.m_values[2] = c() * cos_angle + d() * sin_angle;
    result.m_values[3] = d() * cos_angle - c() * sin_angle;
    result.m_values[4] = tx();
    result.m_values[5] = ty();
    *this = result;
    return *this;
}

template<>
AffineTransform& AffineTransform::rotate_radians(double radians)
{
    double cos_angle = cos(radians);
    double sin_angle = sin(radians);

    AffineTransform result;
    result.m_values[0] = a() * cos_angle + b() * sin_angle;
    result.m_values[1] = b() * cos_angle - a() * sin_angle;
    result.m_values[2] = c() * cos_angle + d() * sin_angle;
    result.m_values[3] = d() * cos_angle - c() * sin_angle;
    result.m_values[4] = tx();
    result.m_values[5] = ty();
    *this = result;
    return *this;
}

template<>
AffineTransform& AffineTransform::rotate_degrees(int degrees)
{
    return rotate_radians(degrees * 0.017453292519943);
}

template<>
AffineTransform& AffineTransform::rotate_degrees(float degrees)
{
    return rotate_radians(degrees * 0.017453292519943);
}

template<>
AffineTransform& AffineTransform::scale(int sx, int sy)
{
    m_values[0] *= sx;
    m_values[1] *= sy;
    m_values[2] *= sx;
    m_values[3] *= sy;
    return *this;
}

template<>
AffineTransform& AffineTransform::scale(float sx, float sy)
{
    m_values[0] *= sx;
    m_values[1] *= sy;
    m_values[2] *= sx;
    m_values[3] *= sy;
    return *this;
}

template<>
AffineTransform& AffineTransform::scale(const IntPoint& scaling)
{
    return scale(scaling.x(), scaling.y());
}

template<>
AffineTransform& AffineTransform::scale(const FloatPoint& scaling)
{
    return scale(scaling.x(), scaling.y());
}

AffineTransform& AffineTransform::multiply(const AffineTransform& other)
{
    AffineTransform result;
    result.m_values[0] = a() * other.a() + b() * other.c();
    result.m_values[1] = a() * other.b() + b() * other.d();
    result.m_values[2] = c() * other.a() + d() * other.c();
    result.m_values[3] = c() * other.b() + d() * other.d();
    result.m_values[4] = a() * other.tx() + b() * other.ty() + tx();
    result.m_values[5] = c() * other.tx() + d() * other.ty() + ty();
    *this = result;
    return *this;
}

AffineTransform& AffineTransform::invert()
{
    AffineTransform result;
    float det = a() * d() - b() * c();
    float minus_det = -det;

    result.m_values[0] = d() / det;
    result.m_values[1] = b() / minus_det;
    result.m_values[2] = c() / minus_det;
    result.m_values[3] = a() / det;
    result.m_values[4] = (d() * tx() - b() * ty()) / minus_det;
    result.m_values[5] = (c() * tx() - a() * ty()) / det;
    *this = result;
    return *this;
}

template<>
IntPoint AffineTransform::translation() const
{
    return { tx(), ty() };
}

template<>
FloatPoint AffineTransform::translation() const
{
    return { tx(), ty() };
}

float AffineTransform::rotation_radians() const
{
    return static_cast<float>(atan2(c(), d()));
}

template<>
int AffineTransform::rotation_degrees() const
{
    return static_cast<int>(rotation_radians() * 57.29577951308232);
}

template<>
float AffineTransform::rotation_degrees() const
{
    return rotation_radians() * 57.29577951308232;
}

FloatPoint AffineTransform::scaling() const
{
    return {
        sqrtf(pow(a(), 2.0) + pow(c(), 2.0)),
        sqrtf(pow(b(), 2.0) + pow(d(), 2.0))
    };
}

void AffineTransform::map(float unmapped_x, float unmapped_y, float& mapped_x, float& mapped_y) const
{
    mapped_x = a() * unmapped_x + b() * unmapped_y + tx();
    mapped_y = c() * unmapped_x + d() * unmapped_y + ty();
}

template<>
IntPoint AffineTransform::map(const IntPoint& point) const
{
    float mapped_x;
    float mapped_y;
    map(point.x(), point.y(), mapped_x, mapped_y);
    return { roundf(mapped_x), roundf(mapped_y) };
}

template<>
FloatPoint AffineTransform::map(const FloatPoint& point) const
{
    float mapped_x;
    float mapped_y;
    map(point.x(), point.y(), mapped_x, mapped_y);
    return { mapped_x, mapped_y };
}

template<>
IntSize AffineTransform::map(const IntSize& size) const
{
    return { roundf(size.width() * x_scale()), roundf(size.height() * y_scale()) };
}

template<>
FloatSize AffineTransform::map(const FloatSize& size) const
{
    return { size.width() * x_scale(), size.height() * y_scale() };
}

template<typename T>
static T smallest_of(T p1, T p2, T p3, T p4)
{
    return min(min(p1, p2), min(p3, p4));
}

template<typename T>
static T largest_of(T p1, T p2, T p3, T p4)
{
    return max(max(p1, p2), max(p3, p4));
}

template<>
FloatRect AffineTransform::map(const FloatRect& rect) const
{
    FloatPoint p1 = map(rect.top_left());
    FloatPoint p2 = map(rect.top_right().translated(1, 0));
    FloatPoint p3 = map(rect.bottom_right().translated(1, 1));
    FloatPoint p4 = map(rect.bottom_left().translated(0, 1));
    float left = smallest_of(p1.x(), p2.x(), p3.x(), p4.x());
    float top = smallest_of(p1.y(), p2.y(), p3.y(), p4.y());
    float right = largest_of(p1.x(), p2.x(), p3.x(), p4.x());
    float bottom = largest_of(p1.y(), p2.y(), p3.y(), p4.y());
    return { left, top, right - left, bottom - top };
}

template<>
IntRect AffineTransform::map(const IntRect& rect) const
{
    return enclosing_int_rect(map(FloatRect(rect)));
}

const LogStream& operator<<(const LogStream& stream, const AffineTransform& value)
{
    if (value.is_identity())
        return stream << "{ Identity }";

    return stream << "{ "
                  << value.a() << ", "
                  << value.b() << ", "
                  << value.c() << ", "
                  << value.d() << ", "
                  << value.tx() << ", "
                  << value.ty() << " }";
}

}
