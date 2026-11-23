cmake_minimum_required(VERSION 3.24)

project(photo26-bootstrap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

add_library(photo26-bootstrap-ui STATIC
    src/bootstrap_window.cpp
    src/bootstrap_window.hpp
)
target_link_libraries(photo26-bootstrap-ui PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets)
target_include_directories(photo26-bootstrap-ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(WIN32)
    add_executable(photo26-bootstrap WIN32 src/main.cpp)
    target_link_libraries(photo26-bootstrap PRIVATE psapi)
else()
    add_executable(photo26-bootstrap src/main.cpp)
endif()
target_link_libraries(photo26-bootstrap PRIVATE photo26-bootstrap-ui)

enable_testing()
add_executable(photo26-bootstrap-tests tests/test_bootstrap_window.cpp)
target_link_libraries(photo26-bootstrap-tests PRIVATE photo26-bootstrap-ui Qt6::Test)
add_test(NAME photo26_bootstrap_ui_test COMMAND photo26-bootstrap-tests)

