#include <AK/Function.h>

void take_fn(Function<void()>) { }

void test()
{
    IGNORE_USE_IN_ESCAPING_LAMBDA int a = 0;
    take_fn([&a] {
        (void)a;
    });
}
