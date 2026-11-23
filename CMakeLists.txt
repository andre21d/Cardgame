cmake_minimum_required(VERSION 3.20)
project(CardGameScene CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_executable(GLProject
    GLProject/main.cpp
)

target_include_directories(GLProject PRIVATE
    GLProject
)

if(EXISTS "C:/msys64/ucrt64/include")
    target_include_directories(GLProject PRIVATE "C:/msys64/ucrt64/include")
endif()

target_link_libraries(GLProject PRIVATE
    OpenGL::GL
    glfw
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/GLProject/assets")
    add_custom_command(
        TARGET GLProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/GLProject/assets
                $<TARGET_FILE_DIR:GLProject>/assets
        COMMENT "Copying assets to build directory..."
    )
endif()