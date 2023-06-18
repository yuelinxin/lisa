cmake_minimum_required(VERSION 3.21)
project(lisa)

set(CMAKE_CXX_STANDARD 14)

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(
        llvm_libs 
        support 
        core 
        irreader 
        scalaropts
        orcjit
        native)

add_executable(lisa
        lisa-llvm/src/lexer.h
        lisa-llvm/src/token.h
        lisa-llvm/src/lexer.cpp
        lisa-llvm/src/parser.cpp
        lisa-llvm/src/parser.h
        lisa-llvm/src/ast.h
        lisa-llvm/src/codegen.cpp
        lisa-llvm/src/driver.cpp)

target_link_libraries(lisa ${llvm_libs})
