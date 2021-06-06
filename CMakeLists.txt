project(MHSJRecognizer)

cmake_minimum_required(VERSION 3.5)

# compiler must be 11 or 14
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wall -g -pedantic")

# add macros to generate ANTLR Cpp code from grammar
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(MHSJParser ${CMAKE_CURRENT_SOURCE_DIR}/grammar/MHSJParser.yy ${CMAKE_CURRENT_BINARY_DIR}/MHSJParser.cpp
             COMPILE_FLAGS "-Wall -Wcounterexamples --defines=${CMAKE_CURRENT_BINARY_DIR}/MHSJParser.h")
FLEX_TARGET(MHSJScanner ${CMAKE_CURRENT_SOURCE_DIR}/grammar/MHSJScanner.ll  ${CMAKE_CURRENT_BINARY_DIR}/MHSJScanner.cpp
            COMPILE_FLAGS "--debug --c++")
ADD_FLEX_BISON_DEPENDENCY(MHSJScanner MHSJParser)

# include generated files in project environment
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(MHSJchecker
  ${CMAKE_CURRENT_SOURCE_DIR}/src/MHSJDriver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/SyntaxTree.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/SyntaxTreePrinter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/SyntaxTreeChecker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ErrorReporter.cpp
  ${BISON_MHSJParser_OUTPUTS}
  ${FLEX_MHSJScanner_OUTPUTS})

