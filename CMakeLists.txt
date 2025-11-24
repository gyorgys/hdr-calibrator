cmake_minimum_required(VERSION 3.15)
project(hdr-calibrator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_definitions(-DUNICODE -D_UNICODE)
elseif(UNIX)
    # Linux/Proton settings
    # For Proton/Wine compatibility, we'll compile for Windows but on Linux
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
    set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
    set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
    
    # Tell CMake to look for Windows libraries
    set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# Source files
set(SOURCES
    main.cpp
)

# Headers
set(HEADERS
    hdr_utils.h
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Link libraries
if(WIN32 AND NOT UNIX)
    # Native Windows build
    target_link_libraries(${PROJECT_NAME} 
        d3d11
        dxgi
        dinput8
        dxguid
    )
else()
    # Cross-compile for Windows on Linux (for Proton)
    target_link_libraries(${PROJECT_NAME}
        -ld3d11
        -ldxgi
        -ldinput8
        -ldxguid
    )
endif()

# Shader compilation (optional - can also compile at runtime)
# Find fxc.exe or use dxc for shader compilation
find_program(FXC_EXECUTABLE fxc.exe 
    HINTS 
    "C:/Program Files (x86)/Windows Kits/10/bin/*/x64"
    "C:/Program Files (x86)/Windows Kits/10/bin/10.0.*/x64"
)

if(FXC_EXECUTABLE)
    message(STATUS "Found FXC: ${FXC_EXECUTABLE}")
    
    set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)
    file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
    
    # Compile vertex shader
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_DIR}/vertex_shader.cso
        COMMAND ${FXC_EXECUTABLE} /T vs_5_0 /E main /Fo ${SHADER_OUTPUT_DIR}/vertex_shader.cso ${CMAKE_SOURCE_DIR}/shaders/vertex_shader.hlsl
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/vertex_shader.hlsl
        COMMENT "Compiling vertex shader..."
    )
    
    # Compile pixel shader
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_DIR}/pixel_shader.cso
        COMMAND ${FXC_EXECUTABLE} /T ps_5_0 /E main /Fo ${SHADER_OUTPUT_DIR}/pixel_shader.cso ${CMAKE_SOURCE_DIR}/shaders/pixel_shader.hlsl
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/pixel_shader.hlsl
        COMMENT "Compiling pixel shader..."
    )
    
    # Add custom target for shaders
    add_custom_target(shaders ALL
        DEPENDS 
            ${SHADER_OUTPUT_DIR}/vertex_shader.cso
            ${SHADER_OUTPUT_DIR}/pixel_shader.cso
    )
    
    add_dependencies(${PROJECT_NAME} shaders)
else()
    message(WARNING "FXC not found - shaders will need to be compiled manually or at runtime")
endif()

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
if(EXISTS ${SHADER_OUTPUT_DIR})
    install(DIRECTORY ${SHADER_OUTPUT_DIR}/ DESTINATION bin/shaders)
endif()
