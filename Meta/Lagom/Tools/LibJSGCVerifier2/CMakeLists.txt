#cmake_minimum_required(VERSION 3.24)
#
#project(LibJSGCVerifier2 C CXX)
#
#find_package(Clang CONFIG REQUIRED HINTS "../../../../Toolchain/Local/clang")
#find_package(LLVM CONFIG REQUIRED HINTS "../../../../Toolchain/Local/clang")
#
#add_library(LibJSGCVerifier2 MODULE main.cpp)
#
#target_include_directories(LibJSGCVerifier2 SYSTEM PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
#target_compile_features(LibJSGCVerifier2 PRIVATE cxx_std_20)
#target_link_libraries(LibJSGCVerifier2 PRIVATE clangAST clangBasic clangFrontend clangLex clangTooling LLVMSupport)
#
#target_compile_options(LibJSGCVerifier2 PRIVATE
#        -Wall
#        -Wextra
#        -Werror
#        -Wno-unused
#        -fno-rtti
#)

if (EXISTS "${CMAKE_BINARY_DIR}/../../Toolchain/Local/clang")
    find_package(Clang CONFIG REQUIRED HINTS "${CMAKE_BINARY_DIR}/../../Toolchain/Local/clang")

    add_library(LibJSGCVerifier2 main.cpp)
    add_library(Lagom::LibJSGCVerifier2 ALIAS LibJSGCVerifier2)

    target_include_directories(LibJSGCVerifier2 SYSTEM PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
    target_compile_features(LibJSGCVerifier2 PRIVATE cxx_std_20)
    target_link_libraries(LibJSGCVerifier2 PRIVATE clangAST clangBasic clangFrontend clangLex clangTooling LLVMSupport)

    target_compile_options(LibJSGCVerifier2 PRIVATE
            -Wall
            -Wextra
            -Werror
            -Wno-unused
            -fno-rtti
    )
endif()
