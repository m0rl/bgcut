cmake_minimum_required(VERSION 3.10)
project(bgcut)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(bgcut main.cpp)

find_package(OpenCV REQUIRED)
target_include_directories(bgcut PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(bgcut ${OpenCV_LIBS})

find_package(Boost 1.65 COMPONENTS program_options)
target_include_directories(bgcut PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(bgcut ${Boost_LIBRARIES})

target_compile_options(bgcut PRIVATE -Wall -pedantic -Werror)
