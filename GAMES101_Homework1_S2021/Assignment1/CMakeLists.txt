cmake_minimum_required(VERSION 3.16)
project(Rasterizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(APPLE)
    foreach(prefix IN ITEMS /opt/homebrew /usr/local)
        if(EXISTS "${prefix}")
            list(APPEND CMAKE_PREFIX_PATH
                "${prefix}"
                "${prefix}/opt/opencv"
                "${prefix}/opt/opencv@4"
                "${prefix}/opt/eigen"
            )
        endif()
    endforeach()
endif()

find_package(OpenCV REQUIRED)

find_path(EIGEN3_PARENT_INCLUDE_DIR
    NAMES eigen3/Eigen/Eigen
    PATHS
        /opt/homebrew/include
        /opt/homebrew/opt/eigen/include
        /usr/local/include
        /usr/local/opt/eigen/include
)

if(NOT EIGEN3_PARENT_INCLUDE_DIR)
    message(FATAL_ERROR "Eigen headers were not found. Please install Eigen first, e.g. brew install eigen")
endif()

add_executable(Rasterizer
    main.cpp
    rasterizer.cpp
    Triangle.cpp
)

target_include_directories(Rasterizer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${EIGEN3_PARENT_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(Rasterizer PRIVATE ${OpenCV_LIBS})

message(STATUS "Using Eigen parent include dir: ${EIGEN3_PARENT_INCLUDE_DIR}")
message(STATUS "Using OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
