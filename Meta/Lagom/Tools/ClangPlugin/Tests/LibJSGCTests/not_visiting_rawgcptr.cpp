#include <LibJS/Runtime/Object.h>

class TestClass : public JS::Object {
    JS_OBJECT(TestClass, JS::Object);

    JS::RawGCPtr<JS::Object> m_object;
};
