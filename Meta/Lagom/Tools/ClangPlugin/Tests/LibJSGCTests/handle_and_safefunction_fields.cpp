/// [handle_and_safefunction_fields.cpp:9:30] warning: Types inheriting from JS::Cell should not have JS::SafeFunction fields
/// [handle_and_safefunction_fields.cpp:10:26] warning: Types inheriting from JS::Cell should not have JS::Handle fields

#include <LibJS/Heap/Cell.h>
#include <LibJS/Heap/Handle.h>
#include <LibJS/SafeFunction.h>

class CellClass : JS::Cell {
    JS::SafeFunction<void()> m_func;
    JS::Handle<JS::Cell> m_handle;
};

class NonCellClass {
    JS::SafeFunction<void()> m_func;
    JS::Handle<JS::Cell> m_handle;
};
