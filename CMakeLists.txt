cmake_minimum_required(VERSION 3.31)

project(AMDScissorBugDemo CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED FALSE)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(AMDScissorBugDemo
        src/main.cpp
        glad/src/gl.c
        src/shader.cpp
        src/fbo.cpp)

target_link_directories(AMDScissorBugDemo PRIVATE
        "${CMAKE_SOURCE_DIR}/glfw/lib-mingw-w64/")

target_link_libraries(AMDScissorBugDemo glfw3.dll)

target_include_directories(AMDScissorBugDemo PRIVATE
        "${CMAKE_SOURCE_DIR}/glad/include/"
        "${CMAKE_SOURCE_DIR}/glfw/include")

add_custom_command(TARGET AMDScissorBugDemo POST_BUILD COMMAND ${CMAKE_COMMAND}
        -E copy_if_different
        ${CMAKE_SOURCE_DIR}/glfw/lib-mingw-w64/glfw3.dll
        $<TARGET_FILE_DIR:AMDScissorBugDemo>/glfw3.dll)

target_link_options(AMDScissorBugDemo PRIVATE -static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive)