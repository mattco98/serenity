/// [wrong_basename_arg.cpp:15:26] warning: Expected second argument of JS_OBJECT macro invocation to be ParentObject
/// [wrong_basename_arg.cpp:22:26] warning: Expected second argument of JS_OBJECT macro invocation to be ::ParentObject
/// [wrong_basename_arg.cpp:38:30] warning: Expected second argument of JS_CELL macro invocation to be Parent4

#include <LibJS/Runtime/Object.h>

// The only way to have an incorrect basename is if the class is deeply nested, and the base name
// refers to a parent class

class ParentObject : JS::Object {
    JS_OBJECT(ParentObject, JS::Object);
};

class TestClass : ParentObject {
    JS_OBJECT(TestClass, JS::Object);
};

// Basename must exactly match the argument
namespace JS {

class TestClass : ::ParentObject {
    JS_OBJECT(TestClass, ParentObject);
};

}

// Nested classes
class Parent1 {};
class Parent2 : JS::Cell {
    JS_CELL(Parent2, JS::Cell);
};
class Parent3 {};
class Parent4 : public Parent2 {
    JS_CELL(Parent4, Parent2);
};

class NestedCellClass : Parent1, Parent3, Parent4 {
    JS_CELL(NestedCellClass, Parent2);
};
