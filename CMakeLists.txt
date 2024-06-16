cmake_minimum_required(VERSION 3.29)

project("test_vulkan")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME})

target_sources(
    ${PROJECT_NAME}
        PRIVATE
            src/main.cpp
            src/logger.cpp
            src/ui/main_window.cpp
            src/render/renderer.cpp
            src/event_loop.cpp
            src/services/file_service.cpp
)

add_compile_definitions("TV_DEBUG_MODE=$<CONFIG:Debug>")

target_include_directories(
    ${PROJECT_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glm
            ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/GLFW/include
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(
        ${PROJECT_NAME}
            PRIVATE
                Vulkan::Vulkan
                ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/GLFW/lib/mingw/libglfw3.a
    )
else()
    target_link_libraries(
        ${PROJECT_NAME}
            PRIVATE
                Vulkan::Vulkan
                ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/GLFW/lib/msvc/glfw3.lib
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(
        ${PROJECT_NAME}
            PRIVATE
                -Wall
                -Wextra
                -Werror
                -pedantic
    )
else()
    target_compile_options(
        ${PROJECT_NAME}
            PRIVATE
                /W4
                /WX
    )
endif()
