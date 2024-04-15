// RUN: %clang++ -cc1 -verify %plugin_opts% %s 2>&1
// expected-no-diagnostics

#include <LibJS/Runtime/Object.h>

class NonCell {
    JS::GCPtr<JS::Object> m_object;
};
