cmake_minimum_required(VERSION 3.10)

project(engine LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(ASSETS_DIR ${CMAKE_SOURCE_DIR}/assets)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assimp)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/assimp/config.h.in
    ${PROJECT_BINARY_DIR}/assimp/config.h
)

configure_file(
    ${PROJECT_SOURCE_DIR}/config.h.in
    ${PROJECT_BINARY_DIR}/config.h
)

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
    src/Renderer/Material.cpp
    src/Renderer/Material.h
    src/Renderer/Mesh.cpp
    src/Renderer/Mesh.h
    src/Renderer/MeshFactory.cpp
    src/Renderer/MeshFactory.h
    src/Renderer/OpenGL/OpenGLMesh.cpp
    src/Renderer/OpenGL/OpenGLMesh.h
    src/Graphics/VertexLayout.cpp
    src/Graphics/VertexLayout.h
    src/Graphics/Clear.h
    src/Renderer/RenderQueue.cpp
    src/Renderer/RenderQueue.h
    src/Scene/Scene.cpp
    src/Scene/Scene.h
    src/Scene/GameObject.cpp
    src/Scene/GameObject.h
    src/Scene/Component.cpp
    src/Scene/Component.h
    src/Scene/Components/MeshComponent.cpp
    src/Scene/Components/MeshComponent.h
    src/Scene/Components/CameraComponent.cpp
    src/Scene/Components/CameraComponent.h
    src/Scene/Components/PlayerControllerComponent.cpp
    src/Scene/Components/PlayerControllerComponent.h
    src/IO/FileSystem.cpp
    src/IO/FileSystem.h
    src/Renderer/Texture.cpp
    src/Renderer/Texture.h
    src/Renderer/OpenGL/OpenGLTexture.cpp
    src/Renderer/OpenGL/OpenGLTexture.h
    src/Assets/MaterialAssetLoader.cpp
    src/Assets/MaterialAssetLoader.h
    src/Assets/MaterialDesc.h
    src/Assets/ImageLoader.cpp
    src/Assets/ImageLoader.h
    src/Renderer/TextureDesc.h
    src/Assets/ModelAssetLoader.cpp
    src/Assets/ModelAssetLoader.h
    src/Renderer/Model.cpp
    src/Renderer/Model.h
    src/Assets/TextureAssetLoader.cpp
    src/Assets/TextureAssetLoader.h
    src/Assets/AssetManager.cpp
    src/Assets/AssetManager.h
    src/Common.h
    src/Renderer/RenderState.cpp
    src/Renderer/RenderState.h
    src/Scene/Components/LightComponent.h
    src/Scene/Components/LightComponent.cpp
    src/Scene/Components/SpotLightComponent.cpp
    src/Scene/Components/SpotLightComponent.h
    src/Scene/Components/DirectionalLightComponent.cpp
    src/Scene/Components/DirectionalLightComponent.h
    src/Scene/Components/PointLightComponent.cpp
    src/Scene/Components/PointLightComponent.h
    src/Scene/Components/AnimationComponent.cpp
    src/Scene/Components/AnimationComponent.h
    src/Renderer/Animation.h
    src/Scene/Components/SkinnedMeshComponent.cpp
    src/Scene/Components/SkinnedMeshComponent.h
)

add_library(engine STATIC ${ENGINE_SRC})

target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(engine PUBLIC ${PROJECT_BINARY_DIR})


target_link_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(engine PUBLIC libglfw3dll.a glad libassimp)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${ASSETS_DIR}
    ${CMAKE_BINARY_DIR}/assets
)
