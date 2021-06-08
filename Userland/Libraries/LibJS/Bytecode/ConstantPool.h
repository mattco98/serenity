/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Format.h>
#include <AK/HashMap.h>
#include <LibJS/Runtime/Value.h>

namespace JS::Bytecode {

class ConstantPoolEntry {
public:
    explicit ConstantPoolEntry(u32 index)
        : m_index(index)
    {
    }

    u32 index() const { return m_index; }

private:
    u32 m_index;
};

class ConstantPool {
public:
    ConstantPool() = default;

    explicit ConstantPool(Vector<Value> values)
        : m_values(move(values))
    {
    }

    Value const& value_at(ConstantPoolEntry const& entry) const { return m_values[entry.index()]; }
    String const& string_at(ConstantPoolEntry const& entry) const;
    Crypto::SignedBigInteger const& bigint_at(ConstantPoolEntry const& entry) const;

private:
    Vector<Value> m_values;
};

class ConstantPoolBuilder {
public:
    ConstantPoolEntry add(Value const& value)
    {
        auto existing_entry = m_values.get(value);
        if (existing_entry.has_value())
            return existing_entry.value();

        ConstantPoolEntry entry { static_cast<u32>(m_values.size()) };
        m_values.set(value, entry);
        return entry;
    }

    ConstantPool build()
    {
        Vector<Value> values;
        values.resize(m_values.size());
        for (auto& [value, entry] : m_values)
            values[entry.index()] = value;

        return ConstantPool(values);
    }

private:
    HashMap<Value, ConstantPoolEntry> m_values;
};

}

namespace AK {

template<>
struct Formatter<JS::Bytecode::ConstantPoolEntry> : Formatter<FormatString> {
    void format(FormatBuilder& builder, JS::Bytecode::ConstantPoolEntry const& entry)
    {
        return Formatter<FormatString>::format(builder, "#{}", entry.index());
    }
};

}
