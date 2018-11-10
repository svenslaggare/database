cmake_minimum_required(VERSION 3.12)
project(database)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

set(SOURCE_FILES
        src/common.cpp
        src/common.h
        src/database_engine.cpp
        src/database_engine.h
        src/execution/executor.cpp
        src/execution/executor.h
        src/execution/helpers.cpp
        src/execution/helpers.h
        src/execution/operation_visitor.cpp
        src/execution/operation_visitor.cpp
        src/execution/operation_visitor.h
        src/execution/select_operation.cpp
        src/execution/select_operation.h
        src/helpers.cpp
        src/helpers.h
        src/query.cpp
        src/query.h
        src/query_expressions/compiler.cpp
        src/query_expressions/compiler.h
        src/query_expressions/execution.cpp
        src/query_expressions/execution.h
        src/query_expressions/expressions.cpp
        src/query_expressions/expressions.h
        src/query_expressions/helpers.cpp
        src/query_expressions/helpers.h
        src/query_expressions/visitor.cpp
        src/query_expressions/visitor.h
        src/storage.cpp
        src/storage.h
        src/table.cpp
        src/table.h src/indices.cpp src/indices.h)

add_executable(database ${SOURCE_FILES} src/main.cpp)