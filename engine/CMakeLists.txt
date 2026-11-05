cmake_minimum_required(VERSION 3.50)

project(engine LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
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

# Build only the Bullet 2 libraries used by the engine. Keep Bullet's source
# tree intact, but exclude its demos, tests, Python bindings, and optional extras.
set(BULLET3_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/bullet3)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static Bullet libraries" FORCE)
set(BUILD_BULLET3 OFF CACHE BOOL "Disable Bullet 3" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "Disable Bullet demos" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "Disable Bullet 2 demos" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "Disable Bullet OpenGL demos" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "Disable Bullet extras" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "Disable Bullet tests" FORCE)
set(BUILD_PYBULLET OFF CACHE BOOL "Disable PyBullet" FORCE)
set(USE_SOFT_BODY_MULTI_BODY_DYNAMICS_WORLD OFF CACHE BOOL
    "Disable Bullet soft-body multibody world" FORCE)
set(USE_GRAPHICAL_BENCHMARK OFF CACHE BOOL
    "Disable Bullet graphical benchmarks" FORCE)
set(USE_GLUT OFF CACHE BOOL "Disable Bullet GLUT integration" FORCE)
set(INSTALL_LIBS OFF CACHE BOOL "Disable Bullet installation targets" FORCE)
set(INSTALL_CMAKE_FILES OFF CACHE BOOL
    "Disable Bullet CMake package installation" FORCE)
add_subdirectory(${BULLET3_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/bullet3 EXCLUDE_FROM_ALL)

set(FT_DISABLE_ZLIB ON CACHE BOOL "Disable FreeType zlib support" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "Disable FreeType bzip2 support" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "Disable FreeType PNG support" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "Disable FreeType HarfBuzz support" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "Disable FreeType Brotli support" FORCE)
set(FT_DISABLE_HVF ON CACHE BOOL "Disable FreeType HVF support" FORCE)
set(SKIP_INSTALL_ALL ON)
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/freetype
    ${CMAKE_CURRENT_BINARY_DIR}/freetype
    EXCLUDE_FROM_ALL
)

set(ENGINE_SRC
    include/miniaudio/miniaudio.c
    src/Application.cpp
    src/Engine.cpp
    src/Input/InputManager.cpp
    src/Platform/Window.cpp
    src/Platform/GLFW/GLFWWindow.cpp
    src/Renderer/RenderDevice.cpp
    src/Renderer/OpenGL/OpenGLRenderDevice.cpp
    src/Renderer/OpenGL/OpenGLShaderProgram.cpp
    src/Renderer/Material.cpp
    src/Renderer/Mesh.cpp
    src/Renderer/MeshFactory.cpp
    src/Renderer/OpenGL/OpenGLMesh.cpp
    src/Graphics/VertexLayout.cpp
    src/Renderer/RenderQueue.cpp
    src/Scene/Scene.cpp
    src/Scene/GameObject.cpp
    src/Scene/Component.cpp
    src/Scene/Components/MeshComponent.cpp
    src/Scene/Components/SpriteComponent.cpp
    src/Scene/Components/SpriteComponent.h
    src/Scene/Components/CameraComponent.cpp
    src/Scene/Components/PlayerControllerComponent.cpp
    src/IO/FileSystem.cpp
    src/Renderer/Texture.cpp
    src/Renderer/OpenGL/OpenGLTexture.cpp
    src/Assets/MaterialAssetLoader.cpp
    src/Assets/ImageLoader.cpp
    src/Assets/ModelAssetLoader.cpp
    src/Renderer/Model.cpp
    src/Assets/TextureAssetLoader.cpp
    src/Assets/AssetManager.cpp
    src/Renderer/RenderState.cpp
    src/Scene/Components/LightComponent.cpp
    src/Scene/Components/SpotLightComponent.cpp
    src/Scene/Components/DirectionalLightComponent.cpp
    src/Scene/Components/PointLightComponent.cpp
    src/Scene/Components/AnimationComponent.cpp
    src/Scene/Components/SkinnedMeshComponent.cpp
    src/Renderer/AnimatorController.cpp
    src/Physics/CollisionShape.cpp
    src/Physics/PhysicsWorld.cpp
    src/Physics/Bullet/BulletPhysicsWorld.cpp
    src/Scene/Components/RigidBodyComponent.cpp
    src/Assets/Scene/SceneDesc.cpp
    src/Assets/Scene/SceneLoadContext.cpp
    src/Assets/Scene/SceneTypeRegistry.cpp
    src/Assets/Scene/ComponentCodec/SceneComponentCodecs.cpp
    src/Assets/Scene/SceneAssetParser.cpp
    src/Assets/Scene/SceneDependencyResolver.cpp
    src/Assets/Scene/ComponentCodec/ComponentCodecCommon.cpp
    src/Assets/Scene/ComponentCodec/CameraComponentCodec.cpp
    src/Assets/Scene/ComponentCodec/RigidBodyComponentCodec.cpp
    src/Assets/Scene/ComponentCodec/MeshComponentCodec.cpp
    src/Assets/Scene/ComponentCodec/LightComponentCodec.cpp
    src/Assets/Scene/SceneInstantiator.cpp
    src/Assets/Scene/SceneAssetLoader.cpp
    src/Audio/AudioManager.cpp
    src/Scene/Components/AudioComponent.cpp
    src/Scene/Components/AudioListenerComponent.cpp
    src/Assets/Scene/ComponentCodec/AudioComponentCodec.cpp
    src/UI/UIElement.cpp
    src/UI/UIContext.cpp
    src/UI/UIImage.cpp
    src/UI/UIButton.cpp
    src/Assets/Font/FontFace.cpp
    src/Assets/FontAssetLoader.cpp
    src/Assets/Font/Font.cpp
    src/Assets/Font/GlyphAtlas.cpp
    src/UI/UIText.cpp
    src/UI/Utf8.cpp
    src/UI/TextTypes.cpp
    src/UI/TextLayout.cpp
    src/UI/TextPainter.cpp
)

add_library(engine STATIC ${ENGINE_SRC})

target_include_directories(engine PRIVATE
    ${BULLET3_ROOT}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/freetype/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}
)

target_link_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(engine PUBLIC
    libglfw3dll.a
    glad
    libassimp
    freetype
)
target_link_libraries(engine PRIVATE
    BulletDynamics
    BulletCollision
    LinearMath
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${ASSETS_DIR}
    ${CMAKE_BINARY_DIR}/assets
)
