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
    src/browser/CefManager.cpp
    src/browser/BrowserHandler.cpp
    src/browser/BrowserClient.cpp
    src/browser/RenderHandler.cpp
    src/browser/ResourceHandler.cpp
    src/browser/CefApp.cpp
    src/browser/BrowserView.cpp
    src/browser/BrowserInterface.cpp
)

# Define header files
set(HEADERS
    include/core/Application.h
    include/core/Settings.h
    include/core/Logger.h
    include/core/EventSystem.h
    include/core/ErrorHandler.h
    include/window/overlay_window.h
    include/window/monitor_info.h
    include/window/window_manager.h
    include/window/input_handler.h
    include/rendering/overlay_renderer.h
    include/rendering/animation_manager.h
    include/rendering/border_renderer.h
    include/rendering/composite_renderer.h
    include/rendering/z_order_manager.h
    include/browser/CefManager.h
    include/browser/BrowserHandler.h
    include/browser/BrowserClient.h
    include/browser/RenderHandler.h
    include/browser/ResourceHandler.h
    include/browser/CefApp.h
    include/browser/BrowserView.h
    include/browser/BrowserInterface.h
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
    
    # Copy CEF process executables
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CEF_ROOT}/Release/chrome_elf.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/"
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CEF_ROOT}/Release/d3dcompiler_47.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/"
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CEF_ROOT}/Release/libEGL.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/"
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CEF_ROOT}/Release/libGLESv2.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/"
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CEF_ROOT}/Release/snapshot_blob.bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/"
    )
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CEF_ROOT}/Release/v8_context_snapshot.bin" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/"
    )
endif()

# CEF subprocess executable
if(WIN32)
    add_executable(CefSubProcess WIN32
        src/browser/cef_subprocess.cpp
    )
    
    target_include_directories(CefSubProcess PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    target_link_libraries(CefSubProcess PRIVATE
        ${CEF_LIBRARIES}
    )

    # Add compile definitions for Windows
    target_compile_definitions(CefSubProcess PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )
    
    add_custom_command(TARGET CefSubProcess POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "$<TARGET_FILE:CefSubProcess>" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/"
    )
endif()

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)