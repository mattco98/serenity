/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// RUN: %clang++ -cc1 -verify %plugin_opts% %s 2>&1

#include <AK/Function.h>

void take_fn(Function<void()>) { }
void take_fn_escaping(ESCAPING Function<void()>) { }

void test()
{
    int a = 0;

    take_fn([&a] {
        (void)a;
    });

    // expected-warning@+2 {{Variable with local storage is captured by reference in a lambda marked ESCAPING}}
    // expected-note@+1 {{Annotate the lambda with DOES_NOT_OUTLIVE_CAPTURES if the captures will not go out of scope before the lambda is executed}}
    take_fn_escaping([&a] {
        (void)a;
    });
}
