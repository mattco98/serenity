lagom_test(
    test-runner.cpp
    NAME TestClangPlugin
    LIBS LibDiff
    WORKING_DIRECTORY "${SERENITY_PROJECT_ROOT}/Meta/Lagom/Tools/ClangPlugin/"
)
