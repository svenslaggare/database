cmake_minimum_required(VERSION 3.12)
project(database)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

set(SOURCE_FILES
        src/table.cpp
        src/table.h
        src/query.cpp
        src/query.h
        src/database_engine.cpp
        src/database_engine.h
        src/column_storage.cpp
        src/column_storage.h
        src/common.cpp
        src/common.h
        src/helpers.cpp
        src/helpers.h src/query_expressions/expressions.cpp src/query_expressions/expressions.h src/query_expressions/visitor.cpp src/query_expressions/visitor.h src/query_expressions/internal_expressions.cpp src/query_expressions/internal_expressions.cpp src/query_expressions/internal_expressions.h src/query_expressions/optimizer.cpp src/query_expressions/optimizer.h src/query_expressions/evaluator.cpp src/query_expressions/evaluator.h src/query_expressions/evaluator.cpp src/query_expressions/evaluator.h src/query_expressions/helpers.cpp src/query_expressions/helpers.h src/execution/operation_visitor.cpp src/execution/operation_visitor.cpp src/execution/operation_visitor.h src/execution/executor.cpp src/execution/executor.h src/query_expressions/compiler.cpp src/query_expressions/compiler.h)

add_executable(database ${SOURCE_FILES} src/main.cpp)