/// [missing_member_in_visit_edges.cpp:13:5] warning: GC-allocated member is not visited in TestClass::visit_edges

#include <LibJS/Runtime/Object.h>

class TestClass : public JS::Object {
    JS_OBJECT(TestClass, JS::Object);

    virtual void visit_edges(Visitor& visitor) override
    {
        Base::visit_edges(visitor);
    }

    JS::GCPtr<JS::Object> m_object;
};
