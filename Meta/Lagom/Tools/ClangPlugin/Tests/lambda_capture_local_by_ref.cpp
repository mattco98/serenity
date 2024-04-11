/// [lambda_capture_local_by_ref.cpp:12:15] warning: Variable with local storage is captured by reference in a lambda that may be asynchronously executed
/// [lambda_capture_local_by_ref.cpp:11:5] note: Annotate the variable declaration with IGNORE_USE_IN_ESCAPING_LAMBDA if it outlives the lambda
/// [lambda_capture_local_by_ref.cpp:7:14] note: Annotate the parameter with NOESCAPE if the lambda will not outlive the function call

#include <AK/Function.h>

void take_fn(Function<void()>) { }

void test()
{
    int a = 0;
    take_fn([&a] {
        (void)a;
    });
}
