cmake_minimum_required(VERSION 3.24)

project(libjs-gc-verifier CXX)

find_package(Clang CONFIG REQUIRED)

add_executable(libjs-gc-verifier src/main.cpp src/CollectCellsHandler.cpp)

target_include_directories(libjs-gc-verifier SYSTEM PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_compile_features(libjs-gc-verifier PRIVATE cxx_std_20)
target_link_libraries(libjs-gc-verifier PRIVATE clangTooling)

target_compile_options(libjs-gc-verifier PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-unused
    -fno-rtti
    -ggdb3 -O0
)

