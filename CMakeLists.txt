cmake_minimum_required(VERSION 3.20)
project(NrealSpatialDisplay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/src/gui/imgui)
if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR "ImGui not found!")
endif()

set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/app/AppConfig.cpp
    src/imu/AirIMU.cpp
    src/imu/ImuFilter.cpp
    src/imu/NrealLightIMU.cpp
    src/capture/DisplayCapture.cpp
    src/capture/CaptureManager.cpp
    src/render/D3DRenderer.cpp
    src/render/ScreenQuad.cpp
    src/render/Camera.cpp
    src/layout/LayoutEngine.cpp
    src/layout/LayoutPreset.cpp
    src/display/DisplaySwitcher.cpp
    src/network/UnityStream.cpp
    src/utils/ComHelper.cpp
    src/utils/Log.cpp
    src/gui/SettingsGUI.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/airapi/hidapi-win/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

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

target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11.lib
    dxgi.lib
    dxguid.lib
    winmm.lib
    user32.lib
    gdi32.lib
)

target_compile_options(${PROJECT_NAME} PRIVATE
    /w-/wd4100 /wd4201 /wd4996
    /utf-8
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release"
)