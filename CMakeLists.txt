cmake_minimum_required(VERSION 3.20)
project(Coordinates18_23 LANGUAGES CXX)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The bundled libraries target Windows x64. Use Visual Studio x64.")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
foreach(number RANGE 18 23)
    set(target Practice${number})
    add_executable(${target} practice${number}/main.cpp)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${target}")
    target_include_directories(${target} PRIVATE third_party/include)
    target_compile_definitions(${target} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_compile_options(${target} PRIVATE /utf-8 /W3)
    target_link_libraries(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lib/x64/glfw3.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lib/x64/glew32.lib" opengl32 user32 gdi32 shell32)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/practice${number}/vertex.glsl"
            "${CMAKE_CURRENT_SOURCE_DIR}/practice${number}/fragmet.glsl"
            "$<TARGET_FILE_DIR:${target}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/third_party/bin/x64/glew32.dll" "$<TARGET_FILE_DIR:${target}>")
endforeach()
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Practice18)
