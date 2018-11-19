cmake_minimum_required(VERSION 3.12)
project(database)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -ggdb")

set(SOURCE_FILES
    src/common.cpp
    src/common.h
    src/database_engine.cpp
    src/database_engine.h
    src/execution/executor.cpp
    src/execution/executor.h
    src/execution/expression_execution.cpp
    src/execution/expression_execution.h
    src/execution/helpers.cpp
    src/execution/helpers.h
    src/execution/index_scanner.cpp
    src/execution/index_scanner.h
    src/execution/operation_visitor.cpp
    src/execution/operation_visitor.cpp
    src/execution/operation_visitor.h
    src/execution/select_operation.cpp
    src/execution/select_operation.h
    src/execution/update_operation.cpp
    src/execution/update_operation.h
    src/execution/virtual_table.cpp
    src/execution/virtual_table.h
    src/helpers.cpp
    src/helpers.h
    src/indices.cpp
    src/indices.h
    src/query.cpp
    src/query.h
    src/query_expressions/compiler.cpp
    src/query_expressions/compiler.h
    src/query_expressions/expressions.cpp
    src/query_expressions/expressions.h
    src/query_expressions/helpers.cpp
    src/query_expressions/helpers.h
    src/query_expressions/ir_optimizer.cpp
    src/query_expressions/ir_optimizer.h
    src/query_expressions/visitor.cpp
    src/query_expressions/visitor.h
    src/storage.cpp
    src/storage.h
    src/table.cpp
    src/table.h)

add_executable(database ${SOURCE_FILES} src/main.cpp)

# Tests
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

find_package(CxxTest)
if(CXXTEST_FOUND)
    add_custom_target(
        run_all_tests ${CMAKE_COMMAND} -E env CTEST_OUTPUT_ON_FAILURE=1
        ${CMAKE_CTEST_COMMAND} -C $<CONFIG> --verbose
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    add_library(TestLib STATIC ${SOURCE_FILES} ${TESTS_DIR}/test_helpers.h)

    include_directories(${CXXTEST_INCLUDE_DIR})
    enable_testing()

    add_executable(tests-dummy ${TESTS_DIR}/dummy.cpp)
    target_link_libraries(tests-dummy TestLib)

    macro(add_test_case TEST_TARGET_NAME TEST_FILE)
        CXXTEST_ADD_TEST(${TEST_TARGET_NAME} "${TEST_TARGET_NAME}-runner.cpp" ${TESTS_DIR}/${TEST_FILE})
        target_link_libraries(${TEST_TARGET_NAME} TestLib)
        add_dependencies(run_all_tests ${TEST_TARGET_NAME})
    endmacro()

    macro(add_test_case_with_defines TEST_TARGET_NAME TEST_FILE DEFINES)
        add_test_case(${TEST_TARGET_NAME} ${TEST_FILE})
        target_compile_definitions(${TEST_TARGET_NAME} PUBLIC ${DEFINES})
    endmacro()

    macro(add_test_case_default_name TEST_FILE)
        string(REPLACE "_" "-" TEST_TARGET_NAME ${TEST_FILE})
        string(REPLACE ".h" "" TEST_TARGET_NAME ${TEST_TARGET_NAME})
        SET(TEST_TARGET_NAME "tests-${TEST_TARGET_NAME}")

        add_test_case(${TEST_TARGET_NAME} ${TEST_FILE})
    endmacro()

    add_test_case_default_name(select_simple.h)
    add_test_case_with_defines(tests-select-simple-optimize-expression select_simple.h OPTIMIZE_EXPRESSIONS)
    add_test_case_with_defines(tests-select-simple-optimize-full select_simple.h OPTIMIZE_FULL)
    add_test_case_default_name(select_index.h)
    add_test_case_default_name(update_simple.h)
    add_test_case_default_name(join.h)
endif()
