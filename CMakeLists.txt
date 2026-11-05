cmake_minimum_required(VERSION 3.50)

project(my_engine)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(MY_ENGINE_SRC
    src/main.cpp
    src/Game/Game.cpp
    src/Game/Factory/ModelFactory.cpp
    src/Game/Scripts/ThirdPerspectiveController.cpp
    src/Game/Entities/Player.cpp
    src/Game/GameObjects/TestObject.cpp
    src/Game/GameObjects/Camera.cpp
)

set_property(GLOBAL PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/engine)

add_executable(my_engine ${MY_ENGINE_SRC})

target_include_directories(my_engine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(my_engine PUBLIC engine)

option(ENGINE_BUILD_TESTS "Build engine logic regression tests" OFF)
if(ENGINE_BUILD_TESTS)
    enable_testing()
    add_executable(ui_context_tests engine/test/UIContextTests.cpp)
    target_link_libraries(ui_context_tests PRIVATE engine)
    add_test(NAME ui_context_tests COMMAND ui_context_tests)
    add_executable(font_asset_tests engine/test/FontAssetTests.cpp)
    target_link_libraries(font_asset_tests PRIVATE engine)
    add_test(NAME font_asset_tests COMMAND font_asset_tests)
    add_executable(opengl_texture_tests engine/test/OpenGLTextureTests.cpp)
    target_link_libraries(opengl_texture_tests PRIVATE engine)
    add_test(NAME opengl_texture_tests COMMAND opengl_texture_tests)
    add_executable(text_layout_tests engine/test/TextLayoutTests.cpp)
    target_link_libraries(text_layout_tests PRIVATE engine)
    add_test(NAME text_layout_tests COMMAND text_layout_tests)
endif()

#copy dll
set(DLL_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/glfw3.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/libassimp-6.dll
)

foreach(DLL ${DLL_FILES})
    if(NOT EXISTS ${DLL})
        message(WARNING "DLL not found: ${DLL}")
    endif()
endforeach()

add_custom_command(
    TARGET my_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DLL_FILES}
        $<TARGET_FILE_DIR:my_engine> 
    COMMENT "Copying DLLs to output directory for development"
)


