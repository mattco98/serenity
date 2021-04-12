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

#include <AK/BinarySearch.h>
#include <AK/QuickSort.h>
#include <LibJS/Runtime/GlobalObject.h>
#include <LibJS/Runtime/SetIterator.h>
#include <LibJS/Runtime/SetObject.h>

namespace JS {

SetIterator* SetIterator::create(GlobalObject& global_object, SetObject* set, Object::PropertyKind iteration_kind)
{
    return global_object.heap().allocate<SetIterator>(global_object, *global_object.builtin_set_iterator_prototype(), set, iteration_kind);
}

SetIterator::SetIterator(Object& prototype, SetObject* set, Object::PropertyKind iteration_kind)
    : Object(prototype)
    , m_set(set)
    , m_iteration_kind(iteration_kind)
{
    // Sort values
    for (auto& entry : set->data())
        m_ordered_values.append(entry);

    quick_sort(m_ordered_values, [&](OrderedEntry& a, OrderedEntry& b) {
        return a.index < b.index;
    });
}

SetIterator::~SetIterator()
{
}

Value SetIterator::next()
{
    if (m_index >= m_ordered_values.size()) {
        m_set = nullptr;
        m_set->remove_iterator(this);
        return {};
    }
    return m_ordered_values[m_index++].value;
}

void SetIterator::visit_edges(Cell::Visitor& visitor)
{
    Base::visit_edges(visitor);
    visitor.visit(m_set);
}

void SetIterator::on_value_set(OrderedEntry entry)
{
    if (!m_ordered_values.is_empty())
        VERIFY(entry.index > m_ordered_values.last().index);
    m_ordered_values.append(entry);
}

void SetIterator::on_value_delete(OrderedEntry entry)
{
    size_t index = 0;
    auto* value = binary_search(m_ordered_values, entry, &index, [&](auto& lhs, auto& rhs) {
        return lhs.index - rhs.index;
    });

    if (!value)
        return;

    m_ordered_values.remove(index);
}

void SetIterator::on_cleared()
{
    m_ordered_values.clear();
}

}
