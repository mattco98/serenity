/*
 * Copyright (c) 2024, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "GrammarContext.h"
#include <LibCore/File.h>

ErrorOr<void> generate_grammar_header_file(Core::File&, GrammarContext&);
ErrorOr<void> generate_grammar_implementation_file(Core::File&, GrammarContext&);
