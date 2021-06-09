/*
 * Copyright (c) 2021, Gunnar Beutner <gbeutner@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/DistinctNumeric.h>
#include <AK/String.h>
#include <AK/Vector.h>
#include <LibJS/Runtime/Value.h>

namespace JS::Bytecode {

TYPEDEF_DISTINCT_NUMERIC_GENERAL(size_t, false, true, false, false, false, false, ConstantTableIndex);

class ObjectLiteralDescriptor {
public:
    explicit ObjectLiteralDescriptor(size_t size)
    {
        m_names.resize(size);
        m_values.resize(size);
    }

    size_t size() const { return m_names.size(); }
    ConstantTableIndex name(size_t index) const { return m_names[index]; }
    Value const& value(size_t index) const { return m_values[index]; }

    void set_name_value(size_t index, ConstantTableIndex name, Value const& value)
    {
        m_names[index] = name;
        m_values[index] = value;
    }

    bool operator==(ObjectLiteralDescriptor const& other);

private:
    Vector<ConstantTableIndex> m_names;
    Vector<Value> m_values;
};

class ConstantTable {
    AK_MAKE_NONMOVABLE(ConstantTable);
    AK_MAKE_NONCOPYABLE(ConstantTable);

public:
    ConstantTable() = default;

    ConstantTableIndex insert(StringView string);
    ConstantTableIndex insert(ObjectLiteralDescriptor const& string);

    String const& get_string(ConstantTableIndex) const;
    ObjectLiteralDescriptor const& get_object_literal_descriptor(ConstantTableIndex) const;

    void dump() const;
    bool is_empty() const { return m_strings.is_empty(); }

private:
    Vector<String> m_strings;
    Vector<ObjectLiteralDescriptor> m_object_literal_descriptors;
};

}
