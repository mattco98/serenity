/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// RUN: %clang++ -cc1 -verify %plugin_opts% %s 2>&1
// expected-no-diagnostics

#include <AK/Function.h>

void take_fn(ESCAPING Function<void()>) { }

void test()
{
    int a = 0;

    take_fn([&a] DOES_NOT_OUTLIVE_CAPTURES {
        (void)a;
    });
}
