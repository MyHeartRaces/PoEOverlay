cmake_minimum_required(VERSION 3.20)
project(PoEOverlay VERSION 0.1.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configure output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include vcpkg toolchain if not already included
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

add_subdirectory(cef)
target_link_libraries(MyApp PRIVATE libcef)

# Find dependencies
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(CEF CONFIG REQUIRED)

# Define source files
set(SOURCES
    src/main.cpp
    src/core/Application.cpp
    src/core/Settings.cpp
    src/core/Logger.cpp
    src/core/EventSystem.cpp
    src/core/ErrorHandler.cpp
    src/window/overlay_window.cpp
    src/window/monitor_info.cpp
    src/window/window_manager.cpp
    src/window/input_handler.cpp
    src/rendering/overlay_renderer.cpp
    src/rendering/animation_manager.cpp
    src/rendering/border_renderer.cpp
    src/rendering/composite_renderer.cpp
    src/rendering/z_order_manager.cpp
)

# Define header files
set(HEADERS
    src/core/Application.cpp
    src/core/Settings.cpp
    src/core/Logger.cpp
    src/core/EventSystem.cpp
    src/core/ErrorHandler.cpp
    src/window/overlay_window.cpp
    src/window/monitor_info.cpp
    src/window/window_manager.cpp
    src/window/input_handler.cpp
    src/rendering/overlay_renderer.cpp
    src/rendering/animation_manager.cpp
    src/rendering/border_renderer.cpp
    src/rendering/composite_renderer.cpp
    src/rendering/z_order_manager.cpp
)

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Add compile definitions for Windows
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    fmt::fmt
)

# Link CEF libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ${CEF_LIBRARIES})

# Set Windows subsystem
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Copy CEF resources to output directory
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CEF_ROOT}/Resources" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/Resources"
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CEF_ROOT}/Release/libcef.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/"
    )
endif()

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
