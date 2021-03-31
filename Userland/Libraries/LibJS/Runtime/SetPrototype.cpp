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

#include <AK/Function.h>
#include <LibJS/Runtime/Function.h>
#include <LibJS/Runtime/GlobalObject.h>
#include <LibJS/Runtime/SetObject.h>
#include <LibJS/Runtime/SetPrototype.h>

namespace JS {

SetPrototype::SetPrototype(GlobalObject& global_object)
    : Object(*global_object.builtin_object_prototype())
{
}

void SetPrototype::initialize(GlobalObject& global_object)
{
    auto& vm = this->vm();
    Object::initialize(global_object);

    u8 attr = Attribute::Writable | Attribute::Configurable;
    define_native_property(vm.names.size, size, nullptr);
    define_native_function(vm.names.add, add, 1, attr);
    define_native_function(vm.names.clear, clear, 0, attr);
    define_native_function(vm.names.delete_, delete_, 1, attr);
    define_native_function(vm.names.forEach, for_each, 1, attr);
    define_native_function(vm.names.has, has, 1, attr);
}

SetPrototype::~SetPrototype()
{
}

static SetObject* set_object_from(VM& vm, GlobalObject& global_object)
{
    auto* this_object = vm.this_value(global_object).to_object(global_object);
    if (!this_object)
        return nullptr;
    if (!is<SetObject>(this_object)) {
        vm.throw_exception<TypeError>(global_object, ErrorType::NotA, "Set");
        return nullptr;
    }
    return static_cast<SetObject*>(this_object);
}

JS_DEFINE_NATIVE_FUNCTION(SetPrototype::add)
{
    auto* set_object = set_object_from(vm, global_object);
    if (!set_object)
        return {};
    auto value = vm.argument(0);
    if (value.is_negative_zero())
        value = Value(0.0);
    set_object->set(value);
    return set_object;
}

JS_DEFINE_NATIVE_FUNCTION(SetPrototype::clear)
{
    auto* set_object = set_object_from(vm, global_object);
    if (!set_object)
        return {};
    set_object->clear();
    return js_undefined();
}

JS_DEFINE_NATIVE_FUNCTION(SetPrototype::delete_)
{
    auto* set_object = set_object_from(vm, global_object);
    if (!set_object)
        return {};
    return Value(set_object->remove(vm.argument(0)));
}

JS_DEFINE_NATIVE_FUNCTION(SetPrototype::for_each)
{
    auto* set_object = set_object_from(vm, global_object);
    if (!set_object)
        return {};

    auto callback = vm.argument(0);
    auto this_arg = vm.argument(1).value_or(js_undefined());

    if (!callback.is_function())
        vm.throw_exception<TypeError>(global_object, ErrorType::NotAFunction, callback.to_string_without_side_effects());

    auto& callback_fn = callback.as_function();

    set_object->for_each_value([&](Value value) {
        [[maybe_unused]] auto ignored = vm.call(callback_fn, this_arg, value, value, set_object);
        return IterationDecision::Continue;
    });

    return js_undefined();
}

JS_DEFINE_NATIVE_FUNCTION(SetPrototype::has)
{
    auto* set_object = set_object_from(vm, global_object);
    if (!set_object)
        return {};

    bool found = false;
    auto target = vm.argument(0).value_or(js_undefined());

    set_object->for_each_value([&](Value value) {
        if (same_value_zero(target, value)) {
            found = true;
            return IterationDecision::Break;
        }
        return IterationDecision::Continue;
    });

    return Value(found);
}

JS_DEFINE_NATIVE_GETTER(SetPrototype::size)
{
    auto* set_object = set_object_from(vm, global_object);
    if (!set_object)
        return {};

    return Value(set_object->data().size());
}

}
