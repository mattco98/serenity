set(LAGOM_BUILD OFF CACHE BOOL "Build without the serenity toolchain Clang")

if (LAGOM_BUILD)
    find_package(Clang 16 CONFIG REQUIRED)
else()
    find_package(Clang CONFIG REQUIRED HINTS "../../../../Toolchain/Local/clang")
endif()

lagom_lib(LambdaCaptureClangPlugin lambda_capture_clang_plugin SOURCES LambdaCapturePluginAction.cpp)
target_include_directories(LambdaCaptureClangPlugin SYSTEM PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_compile_features(LambdaCaptureClangPlugin PRIVATE cxx_std_20)
target_compile_options(LambdaCaptureClangPlugin PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-unused
    -fno-rtti
    -ggdb3
)

lagom_lib(LibJSGCClangPlugin libjs_gc_clang_plugin SOURCES LibJSGCPluginAction.cpp)
target_include_directories(LibJSGCClangPlugin SYSTEM PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_compile_features(LibJSGCClangPlugin PRIVATE cxx_std_20)
target_compile_options(LibJSGCClangPlugin PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wno-unused
        -fno-rtti
        -ggdb3
)

#install(
#    TARGETS LambdaCaptureClangPlugin
#    EXPORT LagomTargets
#    RUNTIME COMPONENT Lagom_Runtime
#)

#set(SOURCES
#    main.cpp
#    DiagConsumer.cpp
#    FileProcessor.cpp
#    LambdaCapturePluginAction.cpp
#    LibJSGCPluginAction.cpp
#)
#
#lagom_tool(ClangPlugin LIBS clangTooling)
#target_include_directories(ClangPlugin SYSTEM PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
#target_compile_features(ClangPlugin PRIVATE cxx_std_20)
