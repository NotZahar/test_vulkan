cmake_minimum_required(VERSION 3.29)

project("test_vulkan")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
        src/main.cpp
)

target_include_directories(${PROJECT_NAME} 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/glm
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/GLFW/include)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        Vulkan::Vulkan 
	    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/GLFW/lib/glfw3.lib)

if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -pedantic)
endif()