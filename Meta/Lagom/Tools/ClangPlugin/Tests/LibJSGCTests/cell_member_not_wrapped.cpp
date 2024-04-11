/// [cell_member_not_wrapped.cpp:36:17] warning: reference to JS::Cell type should be wrapped in JS::NonnullGCPtr
/// [cell_member_not_wrapped.cpp:37:17] warning: pointer to JS::Cell type should be wrapped in JS::GCPtr
/// [cell_member_not_wrapped.cpp:38:25] warning: pointer to JS::Cell type should be wrapped in JS::GCPtr
/// [cell_member_not_wrapped.cpp:39:15] warning: pointer to JS::Cell type should be wrapped in JS::GCPtr
/// [cell_member_not_wrapped.cpp:40:15] warning: pointer to JS::Cell type should be wrapped in JS::GCPtr
/// [cell_member_not_wrapped.cpp:51:17] warning: reference to JS::Cell type should be wrapped in JS::NonnullGCPtr
/// [cell_member_not_wrapped.cpp:52:17] warning: pointer to JS::Cell type should be wrapped in JS::GCPtr
/// [cell_member_not_wrapped.cpp:53:25] warning: pointer to JS::Cell type should be wrapped in JS::GCPtr
/// [cell_member_not_wrapped.cpp:54:15] warning: pointer to JS::Cell type should be wrapped in JS::GCPtr
/// [cell_member_not_wrapped.cpp:55:15] warning: pointer to JS::Cell type should be wrapped in JS::GCPtr

#include <LibJS/Runtime/Object.h>

// Ensure it can see through typedefs
typedef JS::Object NewType1;
using NewType2 = JS::Object;

class TestClass : public JS::Object {
    JS_OBJECT(TestClass, JS::Object);

public:
    explicit TestClass(JS::Realm& realm, JS::Object& obj)
        : JS::Object(realm, nullptr)
        , m_object_ref(obj)
    {
    }

private:
    virtual void visit_edges(Visitor& visitor) override
    {
        Base::visit_edges(visitor);
        visitor.visit(m_object_ref);
        visitor.visit(m_object_ptr);
    }

    JS::Object& m_object_ref;
    JS::Object* m_object_ptr;
    Vector<JS::Object*> m_objects;
    NewType1* m_newtype_1;
    NewType2* m_newtype_2;
};

class TestClassNonCell {
public:
    explicit TestClassNonCell(JS::Object& obj)
        : m_object_ref(obj)
    {
    }

private:
    JS::Object& m_object_ref;
    JS::Object* m_object_ptr;
    Vector<JS::Object*> m_objects;
    NewType1* m_newtype_1;
    NewType2* m_newtype_2;
};
