project(C1Recognizer)

cmake_minimum_required(VERSION 3.5)

# compiler must be 11 or 14
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wall -g -pedantic")

# add macros to generate ANTLR Cpp code from grammar
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(C1Parser ${CMAKE_CURRENT_SOURCE_DIR}/grammar/C1Parser.yy ${CMAKE_CURRENT_BINARY_DIR}/C1Parser.cpp
             COMPILE_FLAGS "-Wall --defines=${CMAKE_CURRENT_BINARY_DIR}/C1Parser.h")
FLEX_TARGET(C1Scanner ${CMAKE_CURRENT_SOURCE_DIR}/grammar/C1Scanner.ll  ${CMAKE_CURRENT_BINARY_DIR}/C1Scanner.cpp
            COMPILE_FLAGS "--debug --c++")
ADD_FLEX_BISON_DEPENDENCY(C1Scanner C1Parser)

# include generated files in project environment
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(c1checker
  ${CMAKE_CURRENT_SOURCE_DIR}/src/C1Driver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/SyntaxTree.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/SyntaxTreePrinter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/SyntaxTreeChecker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ErrorReporter.cpp
  ${BISON_C1Parser_OUTPUTS}
  ${FLEX_C1Scanner_OUTPUTS})

