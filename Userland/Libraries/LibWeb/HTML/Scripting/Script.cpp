/*
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/HTML/Scripting/Environments.h>
#include <LibWeb/HTML/Scripting/Script.h>

namespace Web::HTML {

Script::Script(AK::URL base_url, DeprecatedString filename, EnvironmentSettingsObject& environment_settings_object)
    : m_base_url(move(base_url))
    , m_filename(move(filename))
    , m_settings_object(environment_settings_object)
{
}

Script::~Script() = default;

void Script::visit_edges(Visitor& visitor)
{
    Base::visit_edges(visitor);
    visitor.visit(m_settings_object);
}

}
