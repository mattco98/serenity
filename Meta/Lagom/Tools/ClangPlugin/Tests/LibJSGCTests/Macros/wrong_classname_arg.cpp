/// [wrong_classname_arg.cpp:12:13] warning: Expected first argument of JS_CELL macro invocation to be TestCellClass
/// [wrong_classname_arg.cpp:16:15] warning: Expected first argument of JS_OBJECT macro invocation to be TestObjectClass
/// [wrong_classname_arg.cpp:20:20] warning: Expected first argument of JS_ENVIRONMENT macro invocation to be TestEnvironmentClass
/// [wrong_classname_arg.cpp:24:25] warning: Expected first argument of WEB_PLATFORM_OBJECT macro invocation to be TestPlatformClass

#include <LibJS/Runtime/PrototypeObject.h>
#include <LibWeb/Bindings/PlatformObject.h>

// An incorrect first argument for JS_PROTOTYPE_OBJECT is a compile error, so that is not tested

class TestCellClass : JS::Cell {
    JS_CELL(bad, JS::Cell);
};

class TestObjectClass : JS::Object {
    JS_OBJECT(bad, JS::Object);
};

class TestEnvironmentClass : JS::Environment {
    JS_ENVIRONMENT(bad, JS::Environment);
};

class TestPlatformClass : Web::Bindings::PlatformObject {
    WEB_PLATFORM_OBJECT(bad, Web::Bindings::PlatformObject);
};
