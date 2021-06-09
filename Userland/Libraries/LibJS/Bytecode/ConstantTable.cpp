/*
 * Copyright (c) 2021, Gunnar Beutner <gbeutner@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibJS/Bytecode/ConstantTable.h>

namespace JS::Bytecode {

bool ObjectLiteralDescriptor::operator==(ObjectLiteralDescriptor const& other)
{
    if (m_names != other.m_names || m_values.size() != other.m_values.size())
        return false;

    for (size_t i = 0; i < m_values.size(); i++) {
        if (!same_value(m_values[i], other.m_values[i]))
            return false;
    }

    return true;
}

ConstantTableIndex ConstantTable::insert(StringView string)
{
    for (size_t i = 0; i < m_strings.size(); i++) {
        if (m_strings[i] == string)
            return i;
    }
    m_strings.append(string);
    return m_strings.size() - 1;
}

ConstantTableIndex ConstantTable::insert(ObjectLiteralDescriptor const& descriptor)
{
    for (size_t i = 0; i < m_object_literal_descriptors.size(); i++) {
        if (m_object_literal_descriptors[i] == descriptor)
            return i;
    }
    m_object_literal_descriptors.append(descriptor);
    return m_object_literal_descriptors.size() - 1;
}

String const& ConstantTable::get_string(ConstantTableIndex index) const
{
    return m_strings[index.value()];
}

ObjectLiteralDescriptor const& ConstantTable::get_object_literal_descriptor(ConstantTableIndex index) const
{
    return m_object_literal_descriptors[index.value()];
}

void ConstantTable::dump() const
{
    outln("String Table:");
    for (size_t i = 0; i < m_strings.size(); i++)
        outln("  {}: {}", i, m_strings[i]);
}

}
