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

#include <LibJS/Runtime/Object.h>
#include <LibJS/Runtime/SetObject.h>

namespace JS {

class SetIterator final : public Object {
    JS_OBJECT(SetIterator, Object);

public:
    virtual ~SetIterator() override;

    SetObject* set() const { return m_set; }
    Object::PropertyKind iteration_kind() const { return m_iteration_kind; }
    size_t index() const { return m_index; }

    bool done() const { return !m_set; }
    Value next();

    void for_each_value(AK::Function<IterationDecision(Value)> func);

private:
    friend class SetIteratorPrototype;
    friend class SetObject;
    friend class Heap;

    // SetIterator creation methods are private to force the use of SetObject::create_iterator
    static SetIterator* create(GlobalObject&, SetObject* set, Object::PropertyKind iteration_kind);

    explicit SetIterator(Object& prototype, SetObject* set, Object::PropertyKind iteration_kind);

    virtual void visit_edges(Cell::Visitor&) override;

    void on_value_set(OrderedEntry entry);
    void on_value_delete(OrderedEntry entry);
    void on_cleared();

    SetObject* m_set;
    Object::PropertyKind m_iteration_kind;
    size_t m_index { 0 };
    Vector<OrderedEntry> m_ordered_values;
};

}
