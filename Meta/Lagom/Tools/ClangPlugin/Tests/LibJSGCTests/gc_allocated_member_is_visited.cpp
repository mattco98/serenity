#include <LibJS/Runtime/Object.h>

class TestClass : public JS::Object {
    JS_OBJECT(TestClass, JS::Object);

    virtual void visit_edges(Visitor& visitor) override
    {
        Base::visit_edges(visitor);
        visitor.visit(m_object);
    }

    JS::GCPtr<JS::Object> m_object;
};
