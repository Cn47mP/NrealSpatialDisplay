cmake_minimum_required(VERSION 3.20)
project(NrealSpatialDisplay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGui 源码目录
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/src/gui/imgui)
if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR "ImGui not found! Run: cd src/gui && bash download_imgui.sh")
endif()

# 项目源文件
set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/app/AppConfig.cpp
    src/imu/AirIMU.cpp
    src/imu/NrealLightIMU.cpp
    src/capture/DisplayCapture.cpp
    src/capture/CaptureManager.cpp
    src/render/D3DRenderer.cpp
    src/render/ScreenQuad.cpp
    src/layout/LayoutEngine.cpp
    src/layout/LayoutPreset.cpp
    src/display/DisplaySwitcher.cpp
    src/network/UnityStream.cpp
    src/utils/Log.cpp
    src/gui/SettingsGUI.cpp
    # ImGui (core + win32/dx11 backend)
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

# Include 目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/airapi/hidapi-win/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# 外部库链接
if(EXISTS "${CMAKE_SOURCE_DIR}/lib/AirAPI_Windows.lib")
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/AirAPI_Windows.lib
    )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/airapi/hidapi-win/x64/hidapi.lib")
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/airapi/hidapi-win/x64/hidapi.lib
    )
endif()

# 系统库
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11.lib
    dxgi.lib
    dxguid.lib
    d3dcompiler.lib
    winmm.lib
    user32.lib
    gdi32.lib
)

# 编译选项
target_compile_options(${PROJECT_NAME} PRIVATE
    /w /wd4100 /wd4201 /wd4996
    /utf-8
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release"
)

# 复制 config/ 到 exe 所在目录
if(EXISTS "${CMAKE_SOURCE_DIR}/config/default.json")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/config/default.json"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config/default.json"
        COMMENT "Copying config/default.json"
    )
endif()

# 复制运行时 DLL 到 exe 所在目录
if(EXISTS "${CMAKE_SOURCE_DIR}/airapi/hidapi-win/x64/hidapi.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/airapi/hidapi-win/x64/hidapi.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/hidapi.dll"
        COMMENT "Copying hidapi.dll"
    )
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/airapi/AirAPI_Windows.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/airapi/AirAPI_Windows.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/AirAPI_Windows.dll"
        COMMENT "Copying AirAPI_Windows.dll"
    )
endif()

# HLSL Shader 编译
find_program(DXC_EXECUTABLE dxc HINTS ENV PATH)
if(DXC_EXECUTABLE)
    set(SHADER_DIR ${CMAKE_SOURCE_DIR}/src/render/shaders)
    set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/Release)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${DXC_EXECUTABLE} /T vs_5_0 /E main /Fo ${SHADER_OUT_DIR}/screen_vs.cso ${SHADER_DIR}/screen_vs.hlsl
        COMMAND ${DXC_EXECUTABLE} /T ps_5_0 /E main /Fo ${SHADER_OUT_DIR}/screen_ps.cso ${SHADER_DIR}/screen_ps.hlsl
        COMMENT "Compiling HLSL shaders"
    )
else()
    message(WARNING "dxc.exe not found - HLSL shaders will not be compiled automatically")
endif()
