/// [nested_macros.cpp:13:12] warning: Expected record to have a JS_OBJECT macro invocation
/// [nested_macros.cpp:26:12] warning: Expected record to have a JS_OBJECT macro invocation

#include <LibJS/Runtime/Object.h>

class TestClass : JS::Object {
    JS_OBJECT(TestClass, JS::Object);

    struct NestedClassOk : JS::Cell {
        JS_CELL(NestedClassOk, JS::Cell);
    };

    struct NestedClassBad : JS::Object {
    };

    struct NestedClassNonCell {
    };
};

// Same test, but the parent object is not a cell
class TestClass2 {
    struct NestedClassOk : JS::Cell {
        JS_CELL(NestedClassOk, JS::Cell);
    };

    struct NestedClassBad : JS::Object {
    };

    struct NestedClassNonCell {
    };
};
