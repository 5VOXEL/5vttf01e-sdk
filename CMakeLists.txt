cmake_minimum_required(VERSION 3.16)

project(5vttf01e_sdk
    VERSION 1.1.1
    DESCRIPTION "5Voxel 5VTTF01E (VoxelCount) ToF camera network streaming SDK"
    LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "The 5VTTF01E samples currently use Winsock and build on Windows only.")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Point OpenCV_DIR at the folder containing OpenCVConfig.cmake, e.g.
#   -DOpenCV_DIR=C:/opencv/build/x64/vc16/lib
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui)

add_executable(network_streaming examples/network_streaming/network_streaming.cpp)

target_include_directories(network_streaming PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS})
target_link_libraries(network_streaming PRIVATE ${OpenCV_LIBS} ws2_32)
target_compile_definitions(network_streaming PRIVATE _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS)

if(MSVC)
    target_compile_options(network_streaming PRIVATE /W3 /utf-8)
endif()

# Copy OpenCV DLLs next to the executable so it runs straight from the build tree
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
    add_custom_command(TARGET network_streaming POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:network_streaming> $<TARGET_FILE_DIR:network_streaming>
        COMMAND_EXPAND_LISTS)
endif()

install(TARGETS network_streaming RUNTIME DESTINATION bin)
install(FILES include/network_streaming.h DESTINATION include)
