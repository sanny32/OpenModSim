# Cross-platform code-coverage support for the test targets.
#
# Enable with -DOMODSIM_ENABLE_COVERAGE=ON. Instrumentation is only applied to
# the targets passed to omodsim_apply_coverage() (the testable library and the
# test executables), so the report reflects the production code that the tests
# actually exercise.
#
#   * GCC / Clang : gcov-style instrumentation (--coverage), read by gcovr.
#   * MSVC        : no compile flags needed; OpenCppCoverage instruments the
#                   binaries at run time via their PDBs, so we just make sure
#                   debug info is emitted and optimizations stay off.
#
# The driver that builds, runs the tests and produces the report lives in
# tools/coverage.py and works the same way on every OS.

option(OMODSIM_ENABLE_COVERAGE "Instrument the test targets for code coverage" OFF)

function(omodsim_apply_coverage target)
    if(NOT OMODSIM_ENABLE_COVERAGE)
        return()
    endif()

    # LTO folds away the per-translation-unit mapping coverage tools rely on.
    set_target_properties(${target} PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE --coverage -O0 -g)
        target_link_options(${target} PRIVATE --coverage)
    elseif(MSVC)
        # /Zi emits PDBs for OpenCppCoverage; /OPT:NOREF,NOICF keep line mapping intact.
        target_compile_options(${target} PRIVATE /Zi /Od)
        target_link_options(${target} PRIVATE /DEBUG /OPT:NOREF /OPT:NOICF)
    else()
        message(WARNING "OMODSIM_ENABLE_COVERAGE: unsupported compiler "
                        "'${CMAKE_CXX_COMPILER_ID}', no instrumentation applied.")
    endif()
endfunction()
