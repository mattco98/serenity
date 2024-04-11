#include <LibJS/Runtime/Object.h>

class TestClass : public JS::Object {
    JS_OBJECT(TestClass, JS::Object);

    virtual void visit_edges(Visitor& visitor) override
    {
        Base::visit_edges(visitor);

        // FIXME: It might be nice to check that the object is specifically passed to .visit() or .ignore()
        (void)m_object;
    }

    JS::GCPtr<JS::Object> m_object;
};
