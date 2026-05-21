cmake_minimum_required(VERSION 3.10)
project(Rasterizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED)
find_path(EIGEN3_PARENT_INCLUDE_DIR
        NAMES eigen3/Eigen/Eigen
        PATHS
            /opt/homebrew/include
            /usr/local/include
            /usr/include
            /opt/local/include
)

if (NOT EIGEN3_PARENT_INCLUDE_DIR)
    message(FATAL_ERROR "Eigen not found. Please install it so <eigen3/Eigen/Eigen> is available.")
endif()

add_executable(Rasterizer main.cpp rasterizer.hpp rasterizer.cpp global.hpp Triangle.hpp Triangle.cpp Texture.hpp Texture.cpp Shader.hpp OBJ_Loader.h)
target_include_directories(Rasterizer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${EIGEN3_PARENT_INCLUDE_DIR}
        ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(Rasterizer PRIVATE ${OpenCV_LIBRARIES})
#target_compile_options(Rasterizer PUBLIC -Wall -Wextra -pedantic)
