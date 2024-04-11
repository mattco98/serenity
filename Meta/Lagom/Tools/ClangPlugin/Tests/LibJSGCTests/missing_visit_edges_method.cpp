/// [missing_visit_edges_method.cpp:5:7] warning: JS::Cell-inheriting class TestClass contains a GC-allocated member 'm_cell' but has no visit_edges method

#include <LibJS/Runtime/Object.h>

class TestClass : public JS::Object {
    JS_OBJECT(TestClass, JS::Object);

    JS::GCPtr<JS::Object> m_cell;
};
