// RUN: %clang++ -cc1 -verify %plugin_opts% %s 2>&1
// expected-no-diagnostics

#include <AK/Function.h>
#include <AK/Vector.h>

class Foo {
public:
    void add_function(int a, Function<void()> f, int b)
    {
        (void)a;
        (void)b;
        m_functions.append(move(f));
    }

private:
    Vector<Function<void()>> m_functions;
};

void test()
{
    Foo foo;
    IGNORE_USE_IN_ESCAPING_LAMBDA int a = 10;
    foo.add_function(1, [&a] {
        (void)a;
    }, 3);
}
