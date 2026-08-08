cmake_minimum_required(VERSION 3.10)

project(engine LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

#glad
add_library(glad STATIC ${CMAKE_CURRENT_SOURCE_DIR}/include/glad/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(ENGINE_SRC
    src/Application.cpp
    src/Application.h
    src/Engine.cpp
    src/Engine.h
    src/eng.h
    src/Input/InputManager.cpp
    src/Input/InputManager.h
    src/Platform/Window.cpp
    src/Platform/Window.h
    src/Platform/GLFW/GLFWWindow.cpp
    src/Platform/GLFW/GLFWWindow.h
    src/Renderer/RenderDevice.cpp
    src/Renderer/RenderDevice.h
    src/Renderer/ShaderProgram.h
    src/Renderer/OpenGL/OpenGLRenderDevice.cpp
    src/Renderer/OpenGL/OpenGLRenderDevice.h
    src/Renderer/OpenGL/OpenGLShaderProgram.cpp
    src/Renderer/OpenGL/OpenGLShaderProgram.h
)

add_library(engine STATIC ${ENGINE_SRC})

target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


target_link_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(engine PUBLIC libglfw3dll.a glad)
