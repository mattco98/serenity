/// [classes_are_missing_expected_macros.cpp:10:7] warning: Expected record to have a JS_CELL macro invocation
/// [classes_are_missing_expected_macros.cpp:13:7] warning: Expected record to have a JS_OBJECT macro invocation
/// [classes_are_missing_expected_macros.cpp:16:7] warning: Expected record to have a JS_ENVIRONMENT macro invocation
/// [classes_are_missing_expected_macros.cpp:19:7] warning: Expected record to have a JS_PROTOTYPE_OBJECT macro invocation
/// [classes_are_missing_expected_macros.cpp:22:7] warning: Expected record to have a WEB_PLATFORM_OBJECT macro invocation

#include <LibJS/Runtime/PrototypeObject.h>
#include <LibWeb/Bindings/PlatformObject.h>

class TestCellClass : JS::Cell {
};

class TestObjectClass : JS::Object {
};

class TestEnvironmentClass : JS::Environment {
};

class TestPrototypeClass : JS::PrototypeObject<TestCellClass, TestCellClass> {
};

class TestPlatformClass : Web::Bindings::PlatformObject {
};
