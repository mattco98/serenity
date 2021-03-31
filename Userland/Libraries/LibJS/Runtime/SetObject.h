/*
 * Copyright (c) 2021, Matthew Olsson <matthewcolsson@gmail.com>
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

#include <AK/Function.h>
#include <AK/IterationDecision.h>
#include <AK/Traits.h>
#include <AK/HashTable.h>
#include <LibJS/Runtime/Object.h>

namespace JS {

struct OrderedEntry {
    Value value;
    size_t index { 0 };
};

class SetObject : public Object {
    JS_OBJECT(SetObject, Object);

public:
    static SetObject* create(GlobalObject&);

    SetObject(Object& prototype);
    virtual void initialize(GlobalObject&) override;
    virtual ~SetObject() override;

    const HashTable<OrderedEntry>& data() const { return m_data; }

    void set(Value value);
    bool contains(Value value);
    bool remove(Value value);
    void clear();

    void for_each_value(AK::Function<IterationDecision(Value)> func) const;

private:
    HashTable<OrderedEntry> m_data;
    size_t m_current_index;
};

}

template<>
struct AK::Traits<JS::OrderedEntry> : public AK::GenericTraits<StringImpl*> {
    static constexpr bool equals(const JS::OrderedEntry& a, const JS::OrderedEntry& b)
    {
        return AK::Traits<JS::Value>::equals(a.value, b.value);
    }

    static unsigned hash(const JS::OrderedEntry& entry)
    {
        return Traits<JS::Value>::hash(entry.value);
    }
};
