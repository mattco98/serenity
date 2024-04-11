set(LAGOM_BUILD OFF CACHE BOOL "Build without the serenity toolchain Clang")

if (LAGOM_BUILD)
    find_package(Clang 16 CONFIG REQUIRED)
else()
    find_package(Clang CONFIG REQUIRED HINTS "../../../../Toolchain/Local/clang")
endif()

set(SOURCES
    main.cpp
    DiagConsumer.cpp
    FileProcessor.cpp
    LambdaCapturePluginAction.cpp
    LibJSGCPluginAction.cpp
)

lagom_tool(ClangPlugin LIBS clangTooling)
target_include_directories(ClangPlugin SYSTEM PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_compile_features(ClangPlugin PRIVATE cxx_std_20)
target_compile_options(ClangPlugin PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-unused
    -fno-rtti
    -ggdb3
)
