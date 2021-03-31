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

#include <AK/QuickSort.h>
#include <LibJS/Runtime/GlobalObject.h>
#include <LibJS/Runtime/SetObject.h>

namespace JS {

SetObject* SetObject::create(GlobalObject& global_object)
{
    return global_object.heap().allocate<SetObject>(global_object, *global_object.builtin_set_prototype());
}

SetObject::SetObject(Object& prototype)
    : Object(prototype)
{
}

void SetObject::initialize(GlobalObject& global_object)
{
    Object::initialize(global_object);
}

SetObject::~SetObject()
{
}

void SetObject::set(Value value)
{
    m_data.set({ value, m_current_index++ });
}

bool SetObject::contains(Value value)
{
    return m_data.contains({ value });
}

bool SetObject::remove(Value value)
{
    return m_data.remove({ value });
}

void SetObject::clear()
{
    m_data.clear();
}

void SetObject::for_each_value(AK::Function<IterationDecision(Value)> func) const
{
    // Sort values
    Vector<OrderedEntry> ordered_entry_list;

    for (auto& entry : m_data)
        ordered_entry_list.append(entry);

    quick_sort(ordered_entry_list, [&](OrderedEntry& a, OrderedEntry& b) {
        return a.index < b.index;
    });

    for (auto& entry : ordered_entry_list) {
        if (func(entry.value) == IterationDecision::Break)
            break;
    }
}

}
