cmake_minimum_required(VERSION 3.24)

project(CameraLinkCapture VERSION 1.0.0 LANGUAGES CXX)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "CameraLink Capture requires 64-bit Windows.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(_default_qt_root "C:/Qt/6.8.3/msvc2022_64")
set(CAMERALINK_QT_ROOT "$ENV{QT_ROOT}" CACHE PATH "Qt 6 MSVC installation root")
if(NOT CAMERALINK_QT_ROOT)
    set(CAMERALINK_QT_ROOT "${_default_qt_root}" CACHE PATH "Qt 6 MSVC installation root" FORCE)
endif()

list(PREPEND CMAKE_PREFIX_PATH "${CAMERALINK_QT_ROOT}")
set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/opencv/build/x64/vc16/lib" CACHE PATH "OpenCV package directory")

find_package(Qt6 6.8.3 EXACT REQUIRED COMPONENTS Core Gui Widgets Test)
find_package(OpenCV 4.8.1 EXACT REQUIRED COMPONENTS core highgui imgcodecs imgproc videoio)

set(SAPERA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/SaperaSDK" CACHE PATH "Sapera SDK root")
set(SAPERA_LIBRARY "${SAPERA_ROOT}/Lib/Win64/SapClassBasic.lib")
if(NOT EXISTS "${SAPERA_LIBRARY}")
    message(FATAL_ERROR "Sapera library not found: ${SAPERA_LIBRARY}")
endif()

add_library(Sapera::Basic UNKNOWN IMPORTED GLOBAL)
set_target_properties(Sapera::Basic PROPERTIES
    IMPORTED_LOCATION "${SAPERA_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SAPERA_ROOT}/Common;${SAPERA_ROOT}/Basic;${SAPERA_ROOT}/Include"
)

include(cmake/RuntimeDeployment.cmake)

function(cameralink_configure_target target)
    target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(${target} PRIVATE /utf-8 /W3)
endfunction()

# This layer deliberately excludes Qt and Sapera so command and preview handoff
# behavior stays deterministic under unit tests.
add_library(CameraLinkCaptureRuntime STATIC
    CaptureRuntime.cpp
    PreviewMailbox.cpp
)
cameralink_configure_target(CameraLinkCaptureRuntime)
target_link_libraries(CameraLinkCaptureRuntime PUBLIC ${OpenCV_LIBS})

# The hardware-facing core is shared by the console and GUI front ends. Only the
# front ends own their respective interaction/event loops.
add_library(CameraLinkCaptureCore STATIC
    CaptureWindow.cpp
    ConfigManager.cpp
    FrameBuffer.cpp
    RealtimeView.cpp
    RecordFromBuffer.cpp
    SaperaUse.cpp
)
cameralink_configure_target(CameraLinkCaptureCore)
target_link_libraries(CameraLinkCaptureCore
    PUBLIC
        CameraLinkCaptureRuntime
        Sapera::Basic
        ${OpenCV_LIBS}
)

add_executable(SaperaTest main.cpp)
cameralink_configure_target(SaperaTest)
target_link_libraries(SaperaTest PRIVATE CameraLinkCaptureCore)
cameralink_copy_opencv_runtime(SaperaTest)

add_executable(CameraLinkCapture WIN32
    gui/CameraPreviewWidget.cpp
    gui/CommandConsoleWidget.cpp
    gui/CameraLinkMainWindow.cpp
    gui/QtCaptureController.cpp
    gui/QtCaptureWorker.cpp
    gui/main.cpp
)
cameralink_configure_target(CameraLinkCapture)
target_link_libraries(CameraLinkCapture PRIVATE CameraLinkCaptureCore Qt6::Widgets)
cameralink_copy_opencv_runtime(CameraLinkCapture)
cameralink_deploy_qt_runtime(CameraLinkCapture)

include(CTest)
if(BUILD_TESTING)
    add_executable(SaperaTest.CoreTests
        CaptureWindow.cpp
        FrameBuffer.cpp
        tests/CoreTests.cpp
    )
    cameralink_configure_target(SaperaTest.CoreTests)
    target_link_libraries(SaperaTest.CoreTests PRIVATE Sapera::Basic ${OpenCV_LIBS})
    cameralink_copy_opencv_runtime(SaperaTest.CoreTests)
    add_test(NAME CoreTests COMMAND SaperaTest.CoreTests)

    add_executable(CaptureRuntimeTests tests/CaptureRuntimeTests.cpp)
    cameralink_configure_target(CaptureRuntimeTests)
    target_link_libraries(CaptureRuntimeTests PRIVATE CameraLinkCaptureRuntime)
    cameralink_copy_opencv_runtime(CaptureRuntimeTests)
    add_test(NAME CaptureRuntimeTests COMMAND CaptureRuntimeTests)

    add_executable(CameraLinkCapture.GuiTests
        tests/GuiTests.cpp
        gui/CameraPreviewWidget.cpp
        gui/CommandConsoleWidget.cpp
        gui/CameraLinkMainWindow.cpp
    )
    cameralink_configure_target(CameraLinkCapture.GuiTests)
    target_link_libraries(CameraLinkCapture.GuiTests
        PRIVATE
            CameraLinkCaptureRuntime
            Qt6::Widgets
            Qt6::Test
    )
    cameralink_deploy_qt_runtime(CameraLinkCapture.GuiTests)
    add_test(NAME GuiTests COMMAND CameraLinkCapture.GuiTests)
    set_tests_properties(GuiTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows")

    add_executable(SaperaTest.QtEnvironmentTest tests/QtEnvironmentTest.cpp)
    cameralink_configure_target(SaperaTest.QtEnvironmentTest)
    target_link_libraries(SaperaTest.QtEnvironmentTest PRIVATE Qt6::Widgets)
    cameralink_deploy_qt_runtime(SaperaTest.QtEnvironmentTest)
    add_test(NAME QtEnvironmentTest COMMAND SaperaTest.QtEnvironmentTest)
    set_tests_properties(QtEnvironmentTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows")
endif()
