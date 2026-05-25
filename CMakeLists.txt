cmake_minimum_required(VERSION 3.20)
project(chromatic_echoes CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Homebrew prefix
execute_process(COMMAND brew --prefix
    OUTPUT_VARIABLE BREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)

set(GLAD_DIR   ${CMAKE_SOURCE_DIR}/third_party/glad)
set(STB_DIR    ${CMAKE_SOURCE_DIR}/third_party/stb)
set(GLM_DIR    ${CMAKE_SOURCE_DIR}/third_party/glm)
set(ASSIMP_DIR ${CMAKE_SOURCE_DIR}/third_party/assimp)

find_library(GLFW_LIB  glfw   PATHS ${BREW_PREFIX}/lib REQUIRED)
find_library(ASSIMP_LIB assimp PATHS ${ASSIMP_DIR}/lib  REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(chromatic_echoes
    main.cpp
    ${GLAD_DIR}/src/glad.c
)

target_include_directories(chromatic_echoes PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${GLAD_DIR}/include
    ${STB_DIR}
    ${GLM_DIR}
    ${ASSIMP_DIR}/include
    ${BREW_PREFIX}/include
)

target_link_libraries(chromatic_echoes PRIVATE
    OpenGL::GL
    ${GLFW_LIB}
    ${ASSIMP_LIB}
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)

target_compile_definitions(chromatic_echoes PRIVATE
    GL_SILENCE_DEPRECATION
    GLFW_INCLUDE_NONE
)

add_custom_command(TARGET chromatic_echoes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:chromatic_echoes>/assets
    COMMENT "Copying assets..."
)

message(STATUS "Build:  cmake --build build -j4")
message(STATUS "Run:    ./build/bin/chromatic_echoes")
