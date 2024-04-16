// RUN: %clang++ -cc1 -verify %plugin_opts% %s 2>&1

#include <AK/Function.h>
#include <AK/Vector.h>

class Foo {
public:
    // expected-note@+1 {{Annotate the parameter with NOESCAPE if this is a false positive}}
    void add_function(int a, Function<void()> f, int b)
    {
        add_function2(a, move(f), b);
    }

    void add_function2(int a, Function<void()> f, int b)
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

    // expected-note@+1 {{Annotate the variable declaration with IGNORE_USE_IN_ESCAPING_LAMBDA if it outlives the lambda}}
    int a = 10;

    // expected-warning@+1 {{Variable with local storage is captured by reference in a lambda that escapes its function}}
    foo.add_function(1, [&a] {
        (void)a;
    }, 3);
}
