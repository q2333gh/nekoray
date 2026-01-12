cmake_minimum_required(VERSION 3.16)

project(nekobox VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional: enable clang-tidy with cognitive complexity warnings when available
option(NEKO_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis if available" ON)
if (NEKO_ENABLE_CLANG_TIDY)
    # Try PATH first, then common LLVM install location on Windows
    if (WIN32)
        find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy.exe
            HINTS "C:/Program Files/LLVM/bin" "C:/Program Files (x86)/LLVM/bin")
    else ()
        find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    endif ()

    if (CLANG_TIDY_EXE)
        # Enforce cognitive complexity threshold (~25) via readability-function-cognitive-complexity
        set(CMAKE_CXX_CLANG_TIDY
            "${CLANG_TIDY_EXE};-checks=readability-function-cognitive-complexity;--warnings-as-errors=readability-function-cognitive-complexity")
        message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
    else ()
        message(STATUS "clang-tidy not found, NEKO_ENABLE_CLANG_TIDY is ON but will be ignored")
    endif ()
endif ()

# WINDOWS PDB FILE
if (WIN32)
    if (MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
        # Additional MSVC build speed optimizations
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")  # Multi-processor compilation
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")  # Faster standard library checks
    endif ()
endif ()

# Build speed optimizations
# Unity builds can significantly speed up compilation by combining multiple source files
# Set to a reasonable batch size (e.g., 8 files per unity build) to balance speed and memory
option(NEKO_UNITY_BUILD "Enable Unity Builds for faster compilation" OFF)
if (NEKO_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 8)
    message(STATUS "Unity builds enabled (batch size: ${CMAKE_UNITY_BUILD_BATCH_SIZE})")
endif ()

# Find Qt 6
if (NOT QT_VERSION_MAJOR)
    set(QT_VERSION_MAJOR 6)
endif ()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2 Network Svg LinguistTools)

if (NKR_CROSS)
    set_property(TARGET Qt${QT_VERSION_MAJOR}::moc PROPERTY IMPORTED_LOCATION /usr/bin/moc)
    set_property(TARGET Qt${QT_VERSION_MAJOR}::rcc PROPERTY IMPORTED_LOCATION /usr/bin/rcc)
    set_property(TARGET Qt${QT_VERSION_MAJOR}::lrelease PROPERTY IMPORTED_LOCATION /usr/bin/lrelease)
    set_property(TARGET Qt${QT_VERSION_MAJOR}::lupdate PROPERTY IMPORTED_LOCATION /usr/bin/lupdate)
endif ()

#### Platform Variables ####
if (WIN32)
    include("cmake/windows/windows.cmake")
else ()
    include("cmake/linux/linux.cmake")
endif ()

#### default prefix path ####

if (NOT NKR_LIBS)
    if (NKR_PACKAGE)
        list(APPEND NKR_LIBS ${CMAKE_SOURCE_DIR}/libs/deps/package)
    else ()
        list(APPEND NKR_LIBS ${CMAKE_SOURCE_DIR}/libs/deps/built)
    endif ()
endif ()

if (NOT NKR_DISABLE_LIBS)
    list(APPEND CMAKE_PREFIX_PATH ${NKR_LIBS})
endif ()

message(STATUS "[CMAKE_PREFIX_PATH] ${CMAKE_PREFIX_PATH}")

# for some cross toolchain
list(APPEND CMAKE_FIND_ROOT_PATH ${CMAKE_PREFIX_PATH})
message(STATUS "[CMAKE_FIND_ROOT_PATH] ${CMAKE_FIND_ROOT_PATH}")

#### NKR ####

include("cmake/print.cmake")
include("cmake/nkr.cmake")

find_package(Threads)

#### NKR EXTERNAL ####

if (NKR_NO_EXTERNAL)
    set(NKR_NO_GRPC 1)
    set(NKR_NO_YAML 1)
    set(NKR_NO_ZXING 1)
    set(NKR_NO_QHOTKEY 1)
endif ()

# grpc
if (NKR_NO_GRPC)
    nkr_add_compile_definitions(NKR_NO_GRPC)
else ()
    # My proto
    include("cmake/myproto.cmake")
    list(APPEND NKR_EXTERNAL_TARGETS myproto)
endif ()

# yaml-cpp
if (NKR_NO_YAML)
    nkr_add_compile_definitions(NKR_NO_YAML)
else ()
    find_package(yaml-cpp CONFIG REQUIRED) # only Release is built
    list(APPEND NKR_EXTERNAL_TARGETS yaml-cpp::yaml-cpp)
endif ()

# zxing-cpp
if (NKR_NO_ZXING)
    nkr_add_compile_definitions(NKR_NO_ZXING)
else ()
    find_package(ZXing CONFIG REQUIRED)
    list(APPEND NKR_EXTERNAL_TARGETS ZXing::ZXing)
endif ()

# QHotkey (static submodule)
if (NKR_NO_QHOTKEY)
    nkr_add_compile_definitions(NKR_NO_QHOTKEY)
else ()
    set(QHOTKEY_INSTALL OFF)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(3rdparty/QHotkey)
    list(APPEND NKR_EXTERNAL_TARGETS qhotkey)
endif ()

#### debug print ####

if (DBG_CMAKE)
    print_all_variables()
    print_target_properties(myproto)
    print_target_properties(yaml-cpp::yaml-cpp)
    print_target_properties(ZXing::ZXing)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CMAKE_COMMAND} -E time")
endif ()

# Sources
set(PROJECT_SOURCES
        ${PLATFORM_SOURCES}

        main/main.cpp
        main/NekoGui.cpp
        main/NekoGui_Utils.cpp
        main/HTTPRequestHelper.cpp

        3rdparty/base64.cpp
        3rdparty/qrcodegen.cpp
        3rdparty/QtExtKeySequenceEdit.cpp

        3rdparty/qv2ray/v2/ui/QvAutoCompleteTextEdit.cpp
        3rdparty/qv2ray/v2/components/proxy/QvProxyConfigurator.cpp
        3rdparty/qv2ray/v2/ui/widgets/common/QJsonModel.cpp
        3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.cpp
        3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.hpp
        3rdparty/qv2ray/v2/ui/widgets/editors/w_JsonEditor.ui

        3rdparty/qv2ray/v3/components/GeositeReader/GeositeReader.cpp
        3rdparty/qv2ray/v3/components/GeositeReader/picoproto.cpp

        rpc/gRPC.cpp

        db/Database.cpp
        db/traffic/TrafficLooper.cpp
        db/ProfileFilter.cpp
        db/ConfigBuilder.cpp

        fmt/AbstractBean.cpp
        fmt/Bean2CoreObj_box.cpp
        fmt/Bean2External.cpp
        fmt/Bean2Link.cpp
        fmt/Link2Bean.cpp
        fmt/ChainBean.hpp # translate

        sub/GroupUpdater.cpp

        sys/ExternalProcess.cpp
        sys/AutoRun.cpp

        ui/ThemeManager.cpp
        ui/Icon.cpp

        # QML Models and Controllers
        ui/qml/ProfileModel.cpp
        ui/qml/ProfileModel.hpp
        ui/qml/GroupModel.cpp
        ui/qml/GroupModel.hpp
        ui/qml/MainWindowController.cpp
        ui/qml/MainWindowController.hpp
        ui/qml/MainWindowAdapter.cpp
        ui/qml/MainWindowAdapter.hpp
        ui/qml/BasicSettingsController.cpp
        ui/qml/BasicSettingsController.hpp
        ui/qml/EditProfileController.cpp
        ui/qml/EditProfileController.hpp
        ui/qml/QmlTypes.cpp
        ui/qml/QmlTypes.hpp

        # Legacy widget interface stub to satisfy existing backend calls
        ui/mainwindow_interface.h
        ui/mainwindow.h
        ui/mainwindow_stub.cpp

        res/neko.qrc
        res/theme/feiyangqingyun/qss.qrc
        qml/qml.qrc
        ${QV2RAY_RC}
)

# Qt exe
if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(nekobox
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
    )
    set_target_properties(nekobox PROPERTIES AUTOUIC ON)
    # Define target properties for Android with Qt 6 as:
    #    set_property(TARGET nekobox APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
    #                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
    # For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else ()
    if (ANDROID)
        add_library(nekobox SHARED
                ${PROJECT_SOURCES}
        )
        # Define properties for Android with Qt 5 after find_package() calls as:
        #    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else ()
        add_executable(nekobox
                ${PROJECT_SOURCES}
        )
    endif ()
endif ()

# Target

set_property(TARGET nekobox PROPERTY AUTOMOC ON)
set_property(TARGET nekobox PROPERTY AUTORCC ON)

set_target_properties(nekobox PROPERTIES
        WIN32_EXECUTABLE TRUE
)

target_compile_definitions(nekobox PRIVATE MW_INTERFACE)

# Target Source Translations

set(TS_FILES
        translations/zh_CN.ts
        translations/fa_IR.ts
        translations/ru_RU.ts
)
set(LUPDATE_OPTIONS
        -locations none -no-obsolete
)
if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_lupdate(nekobox TS_FILES ${TS_FILES} OPTIONS ${LUPDATE_OPTIONS})
    qt_add_lrelease(nekobox TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE QM_FILES)
else ()
    qt5_create_translation(QM_FILES ${PROJECT_SOURCES} ${TS_FILES} OPTIONS ${LUPDATE_OPTIONS})
endif ()
configure_file(translations/translations.qrc ${CMAKE_BINARY_DIR} COPYONLY)
target_sources(nekobox PRIVATE ${CMAKE_BINARY_DIR}/translations.qrc)

# Target Link

target_link_libraries(nekobox PRIVATE
        Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Qml Qt${QT_VERSION_MAJOR}::Quick Qt${QT_VERSION_MAJOR}::QuickControls2 Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Svg
        Threads::Threads
        ${NKR_EXTERNAL_TARGETS}
        ${PLATFORM_LIBRARIES}
)

if (QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(nekobox)
endif ()
