cmake_minimum_required(VERSION 3.21)
project(LunarUnlockerNative VERSION 1.0 LANGUAGES C CXX RC)

if(NOT WIN32)
    message(FATAL_ERROR "LunarUnlockerNative is a Windows-only test bridge")
endif()
if(NOT MSVC)
    message(FATAL_ERROR "The audited test bridge requires the MSVC x64 toolchain")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only x64 Minecraft/JVM targets are supported")
endif()

set(LUNARUNLOCKER_JAVA_HOME "$ENV{JAVA_HOME}" CACHE PATH "JDK containing JNI/JVMTI headers")
if(NOT LUNARUNLOCKER_JAVA_HOME)
    foreach(candidate
            "C:/Program Files/Java/jdk1.8.0_301"
            "C:/Program Files/Java/jdk-17")
        if(EXISTS "${candidate}/include/jni.h")
            set(LUNARUNLOCKER_JAVA_HOME "${candidate}")
            break()
        endif()
    endforeach()
endif()

find_path(LUNARUNLOCKER_JNI_INCLUDE jni.h
        HINTS "${LUNARUNLOCKER_JAVA_HOME}/include" REQUIRED)
find_path(LUNARUNLOCKER_JNI_MD_INCLUDE jni_md.h
        HINTS "${LUNARUNLOCKER_JAVA_HOME}/include/win32" REQUIRED)
find_path(LUNARUNLOCKER_JVMTI_INCLUDE jvmti.h
        HINTS "${LUNARUNLOCKER_JAVA_HOME}/include" REQUIRED)

set(LUNARUNLOCKER_PRODUCT_JAR "" CACHE FILEPATH
        "Recovered product JAR embedded into LunarUnlockerNative.dll")
if(NOT LUNARUNLOCKER_PRODUCT_JAR OR NOT EXISTS "${LUNARUNLOCKER_PRODUCT_JAR}")
    message(FATAL_ERROR
            "LUNARUNLOCKER_PRODUCT_JAR must name an existing recovered product JAR")
endif()
get_filename_component(LUNARUNLOCKER_PRODUCT_JAR
        "${LUNARUNLOCKER_PRODUCT_JAR}" ABSOLUTE)
file(TO_CMAKE_PATH "${LUNARUNLOCKER_PRODUCT_JAR}" LUNARUNLOCKER_PRODUCT_JAR_RC_PATH)
configure_file(payload.rc.in generated/payload.rc @ONLY)
set_property(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/generated/payload.rc"
        APPEND PROPERTY OBJECT_DEPENDS "${LUNARUNLOCKER_PRODUCT_JAR}")

add_library(LunarUnlockerNative SHARED
        dllmain.c
        loader_bootstrap.c
        native_bridge.c
        "${CMAKE_CURRENT_BINARY_DIR}/generated/payload.rc")
target_include_directories(LunarUnlockerNative PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${LUNARUNLOCKER_JNI_INCLUDE}"
        "${LUNARUNLOCKER_JNI_MD_INCLUDE}"
        "${LUNARUNLOCKER_JVMTI_INCLUDE}")
target_compile_definitions(LunarUnlockerNative PRIVATE
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS)
target_compile_options(LunarUnlockerNative PRIVATE /W4 /utf-8)
target_link_libraries(LunarUnlockerNative PRIVATE user32 ws2_32)

set(LUNARUNLOCKER_LOADER_GUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/loader_gui")
file(GLOB LUNARUNLOCKER_LOADER_GUI_SOURCES
        CONFIGURE_DEPENDS
        "${LUNARUNLOCKER_LOADER_GUI_DIR}/*.cpp"
        "${LUNARUNLOCKER_LOADER_GUI_DIR}/*.h"
)
add_executable(LunarUnlockerLoaderGui WIN32
        ${LUNARUNLOCKER_LOADER_GUI_SOURCES}
        "${CMAKE_CURRENT_SOURCE_DIR}/injector.c"
        "${LUNARUNLOCKER_LOADER_GUI_DIR}/assets.rc"
        "${LUNARUNLOCKER_LOADER_GUI_DIR}/icon.rc"
        "${LUNARUNLOCKER_LOADER_GUI_DIR}/app.manifest"
)
target_compile_features(LunarUnlockerLoaderGui PRIVATE cxx_std_17)
target_compile_definitions(LunarUnlockerLoaderGui PRIVATE
        WIN32_LEAN_AND_MEAN
        UNICODE _UNICODE NOMINMAX
        _CRT_SECURE_NO_WARNINGS)
target_compile_options(LunarUnlockerLoaderGui PRIVATE /W4 /permissive- /utf-8)
target_link_libraries(LunarUnlockerLoaderGui PRIVATE
        gdiplus
        winhttp
        shell32
        shlwapi
        ws2_32
)
target_include_directories(LunarUnlockerLoaderGui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/loader_gui")

set(LUNARUNLOCKER_LOADER_GUI_DLL_RC
        "${CMAKE_CURRENT_BINARY_DIR}/generated/loader_gui_dll.rc")
add_custom_command(
        OUTPUT "${LUNARUNLOCKER_LOADER_GUI_DLL_RC}"
        COMMAND "${CMAKE_COMMAND}"
                "-DDLL_PATH=$<TARGET_FILE:LunarUnlockerNative>"
                "-DRC_OUT=${LUNARUNLOCKER_LOADER_GUI_DLL_RC}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/embed_dll.cmake"
        DEPENDS LunarUnlockerNative
        COMMENT "Embedding LunarUnlockerNative.dll into the GUI loader")
target_sources(LunarUnlockerLoaderGui PRIVATE "${LUNARUNLOCKER_LOADER_GUI_DLL_RC}")

include(CTest)
if(BUILD_TESTING)
    add_executable(LunarUnlockerBootstrapTest
            tests/loader_bootstrap_test.c
            loader_bootstrap.c)
    target_include_directories(LunarUnlockerBootstrapTest PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(LunarUnlockerBootstrapTest PRIVATE
            WIN32_LEAN_AND_MEAN
            _CRT_SECURE_NO_WARNINGS)
    target_compile_options(LunarUnlockerBootstrapTest PRIVATE /W4 /utf-8)
    target_link_libraries(LunarUnlockerBootstrapTest PRIVATE ws2_32)
    add_test(NAME LunarUnlockerBootstrapStandalone
            COMMAND LunarUnlockerBootstrapTest standalone)
    add_test(NAME LunarUnlockerBootstrapHandoff COMMAND LunarUnlockerBootstrapTest)
    add_test(NAME LunarUnlockerBootstrapRejectsInvalid
            COMMAND LunarUnlockerBootstrapTest invalid)

    add_executable(LoaderSettingsTest
            tests/loader_settings_test.cpp
            loader_gui/loader_settings.cpp)
    target_include_directories(LoaderSettingsTest PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/loader_gui")
    target_compile_features(LoaderSettingsTest PRIVATE cxx_std_17)
    target_compile_definitions(LoaderSettingsTest PRIVATE
            WIN32_LEAN_AND_MEAN
            UNICODE _UNICODE NOMINMAX)
    target_compile_options(LoaderSettingsTest PRIVATE /W4 /permissive- /utf-8)
    set_property(TARGET LoaderSettingsTest PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    add_test(NAME LoaderSettings COMMAND LoaderSettingsTest)

    add_executable(AutoInjectReadinessTest
            tests/auto_inject_readiness_test.cpp
            loader_gui/auto_inject_readiness.cpp)
    target_include_directories(AutoInjectReadinessTest PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/loader_gui")
    target_compile_features(AutoInjectReadinessTest PRIVATE cxx_std_17)
    target_compile_options(AutoInjectReadinessTest PRIVATE /W4 /permissive- /utf-8)
    set_property(TARGET AutoInjectReadinessTest PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    add_test(NAME AutoInjectReadiness COMMAND AutoInjectReadinessTest)

    add_executable(Win32ProcessProbeTest
            tests/win32_process_probe_test.cpp
            loader_gui/win32_process_probe.cpp
            loader_gui/auto_inject_readiness.cpp)
    target_include_directories(Win32ProcessProbeTest PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/loader_gui")
    target_compile_features(Win32ProcessProbeTest PRIVATE cxx_std_17)
    target_compile_definitions(Win32ProcessProbeTest PRIVATE
            WIN32_LEAN_AND_MEAN
            UNICODE _UNICODE NOMINMAX)
    target_compile_options(Win32ProcessProbeTest PRIVATE /W4 /permissive- /utf-8)
    set_property(TARGET Win32ProcessProbeTest PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    add_test(NAME Win32ProcessProbe COMMAND Win32ProcessProbeTest)

    add_executable(InjectionCoordinatorTest
            tests/injection_coordinator_test.cpp
            loader_gui/injection_coordinator.cpp
            loader_gui/win32_process_probe.cpp
            loader_gui/auto_inject_readiness.cpp)
    target_include_directories(InjectionCoordinatorTest PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/loader_gui")
    target_compile_features(InjectionCoordinatorTest PRIVATE cxx_std_17)
    target_compile_definitions(InjectionCoordinatorTest PRIVATE
            WIN32_LEAN_AND_MEAN
            UNICODE _UNICODE NOMINMAX
            _CRT_SECURE_NO_WARNINGS)
    target_compile_options(InjectionCoordinatorTest PRIVATE /W4 /permissive- /utf-8)
    target_link_libraries(InjectionCoordinatorTest PRIVATE advapi32)
    set_property(TARGET InjectionCoordinatorTest PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    add_test(NAME InjectionCoordinator COMMAND InjectionCoordinatorTest)

    add_executable(ControllerModelWorkerTest
            tests/controller_model_worker_test.cpp
            loader_gui/controller_model.cpp
            loader_gui/injection_coordinator.cpp
            loader_gui/local_controller_service.cpp
            loader_gui/win32_process_probe.cpp
            loader_gui/auto_inject_readiness.cpp
            loader_gui/loader_settings.cpp)
    target_include_directories(ControllerModelWorkerTest PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/loader_gui")
    target_compile_features(ControllerModelWorkerTest PRIVATE cxx_std_17)
    target_compile_definitions(ControllerModelWorkerTest PRIVATE
            WIN32_LEAN_AND_MEAN
            UNICODE _UNICODE NOMINMAX
            _CRT_SECURE_NO_WARNINGS)
    target_compile_options(ControllerModelWorkerTest PRIVATE /W4 /permissive- /utf-8)
    target_link_libraries(ControllerModelWorkerTest PRIVATE
            advapi32 shell32 user32 winhttp ws2_32)
    set_property(TARGET ControllerModelWorkerTest PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    add_test(NAME ControllerModelWorker COMMAND ControllerModelWorkerTest)

    add_executable(LocalControllerServiceTest
            tests/local_controller_service_test.cpp
            loader_gui/local_controller_service.cpp)
    target_include_directories(LocalControllerServiceTest PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/loader_gui")
    target_compile_features(LocalControllerServiceTest PRIVATE cxx_std_17)
    target_compile_definitions(LocalControllerServiceTest PRIVATE
            WIN32_LEAN_AND_MEAN
            UNICODE _UNICODE NOMINMAX)
    target_compile_options(LocalControllerServiceTest PRIVATE /W4 /permissive- /utf-8)
    target_link_libraries(LocalControllerServiceTest PRIVATE ws2_32)
    set_property(TARGET LocalControllerServiceTest PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    add_test(NAME LocalControllerService COMMAND LocalControllerServiceTest)
    set_tests_properties(LocalControllerService PROPERTIES TIMEOUT 10)
endif()

set_property(TARGET LunarUnlockerNative LunarUnlockerLoaderGui
        PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set_target_properties(LunarUnlockerNative PROPERTIES
        OUTPUT_NAME "LunarUnlockerNative")
set_target_properties(LunarUnlockerLoaderGui PROPERTIES
        OUTPUT_NAME "LunarUnlocker")

set_target_properties(LunarUnlockerNative LunarUnlockerLoaderGui PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dist")
foreach(configuration Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${configuration}" configuration_upper)
    set_target_properties(LunarUnlockerNative LunarUnlockerLoaderGui PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${configuration_upper}
            "${CMAKE_BINARY_DIR}/dist")
endforeach()
