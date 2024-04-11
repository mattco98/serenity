/// [classes_have_incorrect_macro_types.cpp:17:5] warning: Invalid JS-CELL-like macro invocation; expected JS_CELL
/// [classes_have_incorrect_macro_types.cpp:21:5] warning: Invalid JS-CELL-like macro invocation; expected JS_CELL
/// [classes_have_incorrect_macro_types.cpp:25:5] warning: Invalid JS-CELL-like macro invocation; expected JS_OBJECT
/// [classes_have_incorrect_macro_types.cpp:29:5] warning: Invalid JS-CELL-like macro invocation; expected JS_OBJECT
/// [classes_have_incorrect_macro_types.cpp:36:5] warning: Invalid JS-CELL-like macro invocation; expected JS_CELL
/// [classes_have_incorrect_macro_types.cpp:40:5] warning: Invalid JS-CELL-like macro invocation; expected JS_OBJECT

#include <LibJS/Runtime/Object.h>
#include <LibJS/Runtime/PrototypeObject.h>

// Note: Using WEB_PLATFORM_OBJECT() on a class that doesn't inherit from Web::Bindings::PlatformObject
//       is a compilation error, so that is not tested here.
// Note: It's pretty hard to have the incorrect type in a JS::PrototypeObject, since the base name would
//       have a comma in it, and wouldn't be passable as the basename without a typedef.

class CellWithObjectMacro : JS::Cell {
    JS_OBJECT(CellWithObjectMacro, JS::Cell);
};

class CellWithEnvironmentMacro : JS::Cell {
    JS_ENVIRONMENT(CellWithEnvironmentMacro, JS::Cell);
};

class ObjectWithCellMacro : JS::Object {
    JS_CELL(ObjectWithCellMacro, JS::Object);
};

class ObjectWithEnvironmentMacro : JS::Object {
    JS_ENVIRONMENT(ObjectWithEnvironmentMacro, JS::Object);
};

// JS_PROTOTYPE_OBJECT can only be used in the JS namespace
namespace JS {

class CellWithPrototypeMacro : Cell {
    JS_PROTOTYPE_OBJECT(CellWithPrototypeMacro, Cell, Cell);
};

class ObjectWithPrototypeMacro : Object {
    JS_PROTOTYPE_OBJECT(ObjectWithPrototypeMacro, Object, Object);
};

}
