find_package(Clang 16 CONFIG REQUIRED)

find_package(Clang 16 CONFIG REQUIRED)
add_custom_target(ClangPlugins ALL)

function(clang_plugin target_name)
    cmake_parse_arguments(CLANG_PLUGIN "" "" "SOURCES" ${ARGN})
    add_library(${target_name} ${CLANG_PLUGIN_SOURCES})
    install(TARGETS ${target_name} DESTINATION ${CMAKE_INSTALL_LIBDIR})
    target_include_directories(${target_name} SYSTEM PRIVATE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
    target_compile_features(${target_name} PRIVATE cxx_std_20)
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Werror -Wno-unused -fno-rtti -ggdb3)

    set_property(GLOBAL APPEND PROPERTY CLANG_PLUGIN_COMPILE_OPTIONS -fplugin=${CMAKE_INSTALL_LIBDIR}/lib${target_name}.so)
    add_dependencies(ClangPlugins ${target_name})
endfunction()

clang_plugin(LambdaCaptureClangPlugin SOURCES LambdaCapturePluginAction.cpp)
clang_plugin(LibJSGCClangPlugin SOURCES LibJSGCPluginAction.cpp)
