#include <AK/Function.h>

void take_fn(NOESCAPE Function<void()>) { }

void test()
{
    int a = 0;
    take_fn([&a] {
        (void)a;
    });
}
