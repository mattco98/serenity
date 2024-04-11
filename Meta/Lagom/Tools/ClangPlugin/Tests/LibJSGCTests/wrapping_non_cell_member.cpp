/// [wrapping_non_cell_member.cpp:11:25] warning: Specialization type must inherit from JS::Cell
/// [wrapping_non_cell_member.cpp:12:32] warning: Specialization type must inherit from JS::Cell
/// [wrapping_non_cell_member.cpp:13:28] warning: Specialization type must inherit from JS::Cell

#include <LibJS/Heap/GCPtr.h>
#include <LibJS/Heap/MarkedVector.h>

struct NotACell { };

class TestClass {
    JS::GCPtr<NotACell> m_member_1;
    JS::NonnullGCPtr<NotACell> m_member_2;
    JS::RawGCPtr<NotACell> m_member_3;
};
