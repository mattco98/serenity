/*
 * Copyright (c) 2021, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCrypto/BigInt/SignedBigInteger.h>
#include <LibJS/Bytecode/ConstantPool.h>
#include <LibJS/Runtime/PrimitiveString.h>

namespace JS::Bytecode {

String const& ConstantPool::string_at(ConstantPoolEntry const& entry) const
{
    return value_at(entry).as_string().string();
}

Crypto::SignedBigInteger const& ConstantPool::bigint_at(const ConstantPoolEntry& entry) const
{
    return value_at(entry).as_bigint().big_integer();
}

}