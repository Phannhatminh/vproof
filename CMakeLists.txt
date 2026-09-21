cmake_minimum_required(VERSION 3.16)
project(vproof CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_path(GMP_INCLUDE_DIR gmp.h HINTS /opt/homebrew/include /usr/local/include)
find_library(GMP_LIBRARY NAMES gmp HINTS /opt/homebrew/lib /usr/local/lib)
if(NOT GMP_INCLUDE_DIR OR NOT GMP_LIBRARY)
  message(FATAL_ERROR "cần GMP (brew install gmp)")
endif()

add_library(vruntime
  src/rational.cpp
  src/prop.cpp
  src/world.cpp
  src/prover.cpp
  src/parser.cpp
  src/interp.cpp
  src/symbolic.cpp
)
target_include_directories(vruntime PUBLIC include ${GMP_INCLUDE_DIR})
target_link_libraries(vruntime PUBLIC ${GMP_LIBRARY})

enable_testing()
add_executable(test_world tests/test_world.cpp)
target_link_libraries(test_world PRIVATE vruntime)
add_test(NAME world COMMAND test_world)

add_executable(test_prop tests/test_prop.cpp)
target_link_libraries(test_prop PRIVATE vruntime)
add_test(NAME prop COMMAND test_prop)

add_executable(test_apply tests/test_apply.cpp)
target_link_libraries(test_apply PRIVATE vruntime)
add_test(NAME apply COMMAND test_apply)

add_executable(test_prover tests/test_prover.cpp)
target_link_libraries(test_prover PRIVATE vruntime)
add_test(NAME prover COMMAND test_prover)

add_executable(test_reflect tests/test_reflect.cpp)
target_link_libraries(test_reflect PRIVATE vruntime)
add_test(NAME reflect COMMAND test_reflect)

add_executable(vproof src/main.cpp)
target_link_libraries(vproof PRIVATE vruntime)

add_executable(test_examples tests/test_examples.cpp)
target_link_libraries(test_examples PRIVATE vruntime)
add_test(NAME examples COMMAND test_examples ${CMAKE_CURRENT_SOURCE_DIR}/examples ${CMAKE_CURRENT_SOURCE_DIR}/docs)
