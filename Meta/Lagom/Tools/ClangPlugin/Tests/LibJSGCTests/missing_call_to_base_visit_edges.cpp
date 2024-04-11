/// [missing_call_to_base_visit_edges.cpp:8:5] warning: Missing call to Base::visit_edges

#include <LibJS/Runtime/Object.h>

class TestClass : public JS::Object {
    JS_OBJECT(TestClass, JS::Object);

    virtual void visit_edges(Visitor&) override
    {
    }
};
