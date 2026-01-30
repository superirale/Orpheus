cmake_minimum_required(VERSION 3.16)
project(Orpheus 
    VERSION 0.0.6 
    DESCRIPTION "Audio management library built on SoLoud"
    LANGUAGES C CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(ORPHEUS_BUILD_EXAMPLES "Build example applications" ON)
option(ORPHEUS_BUILD_TESTS "Build unit tests" ON)
option(ORPHEUS_USE_PCH "Use precompiled headers" ON)

# -----------------------------------------------------------------------------
# Find or Fetch SoLoud dependency
# -----------------------------------------------------------------------------
option(ORPHEUS_USE_SYSTEM_SOLOUD "Prefer system-installed SoLoud" ON)

set(SOLOUD_FOUND_SYSTEM FALSE)

if(ORPHEUS_USE_SYSTEM_SOLOUD)
    # Try to find SoLoud via CMake config first
    find_package(soloud CONFIG QUIET)

    if(NOT soloud_FOUND)
        # Fallback: Try pkg-config
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SOLOUD QUIET soloud)
        endif()
        
        if(NOT SOLOUD_FOUND)
            # Manual fallback: check common locations
            find_path(SOLOUD_INCLUDE_DIRS 
                NAMES soloud.h
                PATHS 
                    ${SOLOUD_ROOT}/include
                    /usr/local/include
                    /usr/include
                    /opt/homebrew/include
            )
            
            find_library(SOLOUD_LIBRARIES 
                NAMES soloud
                PATHS 
                    ${SOLOUD_ROOT}/lib
                    /usr/local/lib
                    /usr/lib
                    /opt/homebrew/lib
            )
            
            if(SOLOUD_INCLUDE_DIRS AND SOLOUD_LIBRARIES)
                set(SOLOUD_FOUND TRUE)
                set(SOLOUD_FOUND_SYSTEM TRUE)
            endif()
        else()
            set(SOLOUD_FOUND_SYSTEM TRUE)
        endif()
    else()
        set(SOLOUD_FOUND_SYSTEM TRUE)
    endif()
endif()

# -----------------------------------------------------------------------------
# Fetch nlohmann/json for JSON parsing (header-only)
# -----------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
endif()
set(JSON_INCLUDE_DIR ${json_SOURCE_DIR}/include)

# If not found on system, fetch and build from source
if(NOT SOLOUD_FOUND_SYSTEM AND NOT soloud_FOUND)
    message(STATUS "SoLoud not found on system - fetching from GitHub...")
    
    FetchContent_Declare(
        soloud
        GIT_REPOSITORY https://github.com/jarikomppa/soloud.git
        GIT_TAG        RELEASE_20200207
        GIT_SHALLOW    TRUE
    )
    
    # SoLoud build options
    set(SOLOUD_BACKEND_NULL OFF CACHE BOOL "" FORCE)
    set(SOLOUD_BACKEND_SDL2 OFF CACHE BOOL "" FORCE)
    set(SOLOUD_BACKEND_COREAUDIO ON CACHE BOOL "" FORCE)  # macOS default
    set(SOLOUD_BACKEND_MINIAUDIO ON CACHE BOOL "" FORCE)  # Cross-platform fallback
    set(SOLOUD_BACKEND_ALSA OFF CACHE BOOL "" FORCE)
    set(SOLOUD_BACKEND_OPENSLES OFF CACHE BOOL "" FORCE)
    set(SOLOUD_BACKEND_XAUDIO2 OFF CACHE BOOL "" FORCE)
    set(SOLOUD_BACKEND_WINMM OFF CACHE BOOL "" FORCE)
    set(SOLOUD_BACKEND_WASAPI OFF CACHE BOOL "" FORCE)
    
    # Check platform and adjust backends
    if(ANDROID)
        set(SOLOUD_BACKEND_COREAUDIO OFF CACHE BOOL "" FORCE)
        set(SOLOUD_BACKEND_MINIAUDIO OFF CACHE BOOL "" FORCE)
        set(SOLOUD_BACKEND_OPENSLES ON CACHE BOOL "" FORCE)
    elseif(WIN32)
        set(SOLOUD_BACKEND_COREAUDIO OFF CACHE BOOL "" FORCE)
        set(SOLOUD_BACKEND_WASAPI ON CACHE BOOL "" FORCE)
    elseif(UNIX AND NOT APPLE)
        set(SOLOUD_BACKEND_COREAUDIO OFF CACHE BOOL "" FORCE)
        # When cross-compiling for aarch64, only use MINIAUDIO (no ALSA dependencies)
        if(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
            set(SOLOUD_BACKEND_ALSA OFF CACHE BOOL "" FORCE)
            set(SOLOUD_BACKEND_MINIAUDIO ON CACHE BOOL "" FORCE)
        else()
            set(SOLOUD_BACKEND_ALSA ON CACHE BOOL "" FORCE)
        endif()
    endif()
    
    FetchContent_GetProperties(soloud)
    if(NOT soloud_POPULATED)
        FetchContent_Populate(soloud)
        
        # SoLoud doesn't have a proper CMakeLists.txt at root, so we build it manually
        set(SOLOUD_SRC_DIR ${soloud_SOURCE_DIR}/src)
        set(SOLOUD_INCLUDE_DIR ${soloud_SOURCE_DIR}/include)
        
        # Collect SoLoud source files
        file(GLOB SOLOUD_CORE_SOURCES "${SOLOUD_SRC_DIR}/core/*.cpp" "${SOLOUD_SRC_DIR}/core/*.c")
        file(GLOB_RECURSE SOLOUD_AUDIOSOURCE_SOURCES "${SOLOUD_SRC_DIR}/audiosource/*.cpp" "${SOLOUD_SRC_DIR}/audiosource/*.c")
        file(GLOB SOLOUD_FILTER_SOURCES "${SOLOUD_SRC_DIR}/filter/*.cpp")
        
        # Backend sources based on platform
        set(SOLOUD_BACKEND_SOURCES "")
        if(APPLE)
            file(GLOB SOLOUD_BACKEND_SOURCES "${SOLOUD_SRC_DIR}/backend/coreaudio/*.cpp")
        elseif(WIN32)
            file(GLOB SOLOUD_BACKEND_SOURCES "${SOLOUD_SRC_DIR}/backend/wasapi/*.cpp")
        elseif(ANDROID)
            file(GLOB SOLOUD_BACKEND_SOURCES "${SOLOUD_SRC_DIR}/backend/opensles/*.cpp" "${SOLOUD_SRC_DIR}/backend/opensles/*.c")
        elseif(UNIX AND NOT ANDROID)
            # When cross-compiling for aarch64, skip ALSA (use MINIAUDIO only)
            if(NOT (CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64"))
                file(GLOB SOLOUD_BACKEND_SOURCES "${SOLOUD_SRC_DIR}/backend/alsa/*.cpp")
            endif()
        endif()
        
        # Add miniaudio backend as fallback (not on Android)
        if(NOT ANDROID)
            file(GLOB SOLOUD_MINIAUDIO_SOURCES "${SOLOUD_SRC_DIR}/backend/miniaudio/*.cpp")
            list(APPEND SOLOUD_BACKEND_SOURCES ${SOLOUD_MINIAUDIO_SOURCES})
        endif()
        
        # Create SoLoud library
        add_library(soloud_fetched STATIC
            ${SOLOUD_CORE_SOURCES}
            ${SOLOUD_AUDIOSOURCE_SOURCES}
            ${SOLOUD_FILTER_SOURCES}
            ${SOLOUD_BACKEND_SOURCES}
        )
        
        target_include_directories(soloud_fetched 
            PUBLIC 
                $<BUILD_INTERFACE:${SOLOUD_INCLUDE_DIR}>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/soloud>
        )
        
        # Platform-specific definitions and links
        if(APPLE)
            target_compile_definitions(soloud_fetched PRIVATE WITH_COREAUDIO WITH_MINIAUDIO)
            find_library(COREAUDIO_FRAMEWORK CoreAudio)
            find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
            target_link_libraries(soloud_fetched PUBLIC ${COREAUDIO_FRAMEWORK} ${AUDIOTOOLBOX_FRAMEWORK})
        elseif(WIN32)
            target_compile_definitions(soloud_fetched PRIVATE WITH_WASAPI WITH_MINIAUDIO)
        elseif(ANDROID)
            target_compile_definitions(soloud_fetched PRIVATE WITH_OPENSLES)
            target_link_libraries(soloud_fetched PUBLIC OpenSLES)
        elseif(UNIX AND NOT ANDROID)
            # When cross-compiling for aarch64, use MINIAUDIO only (no ALSA)
            if(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
                target_compile_definitions(soloud_fetched PRIVATE WITH_MINIAUDIO)
            else()
                target_compile_definitions(soloud_fetched PRIVATE WITH_ALSA WITH_MINIAUDIO)
                find_package(ALSA QUIET)
                if(ALSA_FOUND)
                    target_link_libraries(soloud_fetched PUBLIC ${ALSA_LIBRARIES})
                    target_include_directories(soloud_fetched PRIVATE ${ALSA_INCLUDE_DIRS})
                endif()
            endif()
        endif()
        
        # Suppress warnings in third-party code
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
            target_compile_options(soloud_fetched PRIVATE -w)
        elseif(MSVC)
            target_compile_options(soloud_fetched PRIVATE /w)
        endif()
        
        set(SOLOUD_FETCHED TRUE)
    endif()
endif()

# -----------------------------------------------------------------------------
# Orpheus Library
# -----------------------------------------------------------------------------
# Collect source files
set(ORPHEUS_SOURCES
    src/AudioManager.cpp
    src/OcclusionProcessor.cpp
    src/ReverbBus.cpp
    src/VoicePool.cpp
    src/Bus.cpp
    src/MixZone.cpp
    src/ReverbZone.cpp
    src/SoundBank.cpp
    src/Event.cpp
    src/AudioZone.cpp
    src/Ducker.cpp
    src/MusicManager.cpp
)

set(ORPHEUS_HEADERS
    include/AudioManager.h
    include/Bus.h
    include/DSPFilters.h
    include/Event.h
    include/Parameter.h
    include/Profiler.h
    include/VirtualVoiceManager.h
)


# Create the library
add_library(orpheus ${ORPHEUS_SOURCES} ${ORPHEUS_HEADERS})
add_library(Orpheus::orpheus ALIAS orpheus)

# Prevent min/max macro conflicts on Windows
if(WIN32)
    target_compile_definitions(orpheus PRIVATE NOMINMAX)
endif()

# Include directories
target_include_directories(orpheus 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link SoLoud and add JSON include directory
target_include_directories(orpheus PRIVATE ${JSON_INCLUDE_DIR})

if(soloud_FOUND)
    target_link_libraries(orpheus PRIVATE soloud::soloud)
elseif(SOLOUD_FETCHED)
    target_link_libraries(orpheus PRIVATE soloud_fetched)
else()
    target_include_directories(orpheus PRIVATE ${SOLOUD_INCLUDE_DIRS})
    target_link_libraries(orpheus PRIVATE ${SOLOUD_LIBRARIES})
endif()

# Precompiled headers
if(ORPHEUS_USE_PCH AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(orpheus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch.h)
endif()

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(orpheus PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(orpheus PRIVATE /W4)
endif()

# -----------------------------------------------------------------------------
# Example Application
# -----------------------------------------------------------------------------
if(ORPHEUS_BUILD_EXAMPLES)
    add_executable(orpheus_example example/main.cpp)
    target_include_directories(orpheus_example PRIVATE ${JSON_INCLUDE_DIR})
    target_link_libraries(orpheus_example PRIVATE orpheus)
    if(WIN32)
        target_compile_definitions(orpheus_example PRIVATE NOMINMAX)
    endif()
    
    # Copy assets for examples
    add_custom_command(TARGET orpheus_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:orpheus_example>/assets
        COMMENT "Copying assets to build directory"
    )

    add_executable(orpheus_example_2 example/example_2.cpp)
    target_include_directories(orpheus_example_2 PRIVATE ${JSON_INCLUDE_DIR})
    target_link_libraries(orpheus_example_2 PRIVATE orpheus)
    if(WIN32)
        target_compile_definitions(orpheus_example_2 PRIVATE NOMINMAX)
    endif()

    add_custom_command(TARGET orpheus_example_2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:orpheus_example_2>/assets
        COMMENT "Copying assets for example 2"
    )
endif()

# -----------------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

# Install targets - include soloud_fetched if we built it
if(SOLOUD_FETCHED)
    install(TARGETS orpheus soloud_fetched
        EXPORT OrpheusTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    
    # Install SoLoud headers too
    install(DIRECTORY ${SOLOUD_INCLUDE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/soloud
    )
else()
    install(TARGETS orpheus
        EXPORT OrpheusTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(FILES ${ORPHEUS_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/orpheus
)

install(EXPORT OrpheusTargets
    FILE OrpheusTargets.cmake
    NAMESPACE Orpheus::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Orpheus
)

# Package configuration
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/OrpheusConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/OrpheusConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Orpheus
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/OrpheusConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/OrpheusConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/OrpheusConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Orpheus
)

# -----------------------------------------------------------------------------
# Unit Tests (Catch2)
# -----------------------------------------------------------------------------
if(ORPHEUS_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(Catch2)

    # Test sources
    file(GLOB ORPHEUS_TEST_SOURCES "tests/*.cpp")

    add_executable(orpheus_tests ${ORPHEUS_TEST_SOURCES})
    target_link_libraries(orpheus_tests PRIVATE orpheus Catch2::Catch2WithMain)
    target_include_directories(orpheus_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${JSON_INCLUDE_DIR}
    )
    if(WIN32)
        target_compile_definitions(orpheus_tests PRIVATE NOMINMAX)
    endif()

    # Enable CTest
    include(CTest)
    include(Catch)
    catch_discover_tests(orpheus_tests)
endif()

# -----------------------------------------------------------------------------
# Performance Benchmarks (Google Benchmark)
# -----------------------------------------------------------------------------
option(ORPHEUS_BUILD_BENCHMARKS "Build performance benchmarks" OFF)

if(ORPHEUS_BUILD_BENCHMARKS)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
        GIT_SHALLOW TRUE
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)

    # Benchmark sources
    file(GLOB ORPHEUS_BENCHMARK_SOURCES "benchmarks/*.cpp")

    add_executable(orpheus_benchmarks ${ORPHEUS_BENCHMARK_SOURCES})
    target_link_libraries(orpheus_benchmarks PRIVATE orpheus benchmark::benchmark)
    target_include_directories(orpheus_benchmarks PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${JSON_INCLUDE_DIR}
    )

    # Optimize benchmarks
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(orpheus_benchmarks PRIVATE -O3 -march=native)
    elseif(MSVC)
        target_compile_options(orpheus_benchmarks PRIVATE /O2)
    endif()
endif()

# -----------------------------------------------------------------------------
# Fuzzing (libFuzzer)
# -----------------------------------------------------------------------------
option(ORPHEUS_BUILD_FUZZER "Build fuzz tests (requires Clang)" OFF)

if(ORPHEUS_BUILD_FUZZER)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        message(FATAL_ERROR "Fuzzing requires Clang compiler. Use -DCMAKE_CXX_COMPILER=clang++")
    endif()

    # Fuzz sources
    file(GLOB ORPHEUS_FUZZ_SOURCES "fuzz/*.cpp")

    foreach(fuzz_source ${ORPHEUS_FUZZ_SOURCES})
        get_filename_component(fuzz_name ${fuzz_source} NAME_WE)
        add_executable(${fuzz_name} ${fuzz_source})
        target_link_libraries(${fuzz_name} PRIVATE orpheus)
        target_include_directories(${fuzz_name} PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${JSON_INCLUDE_DIR}
        )
        target_compile_options(${fuzz_name} PRIVATE 
            -fsanitize=fuzzer,address,undefined
            -fno-omit-frame-pointer
            -g
        )
        target_link_options(${fuzz_name} PRIVATE 
            -fsanitize=fuzzer,address,undefined
        )
    endforeach()

    message(STATUS "Fuzz targets created. Run with: ./fuzz_json_parser fuzz/corpus/")
endif()

# -----------------------------------------------------------------------------
# Code Coverage (gcov/llvm-cov)
# -----------------------------------------------------------------------------
option(ORPHEUS_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

if(ORPHEUS_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        # GCC: Use gcov
        set(COVERAGE_FLAGS "--coverage -fprofile-arcs -ftest-coverage")
        set(COVERAGE_LINK_FLAGS "--coverage")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        # Clang: Use llvm-cov
        set(COVERAGE_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
        set(COVERAGE_LINK_FLAGS "-fprofile-instr-generate")
    else()
        message(WARNING "Code coverage not supported for ${CMAKE_CXX_COMPILER_ID}")
        set(COVERAGE_FLAGS "")
        set(COVERAGE_LINK_FLAGS "")
    endif()

    if(COVERAGE_FLAGS)
        # Add coverage flags to library
        target_compile_options(orpheus PRIVATE ${COVERAGE_FLAGS})
        target_link_options(orpheus PRIVATE ${COVERAGE_LINK_FLAGS})

        # Add coverage to tests
        if(ORPHEUS_BUILD_TESTS)
            target_compile_options(orpheus_tests PRIVATE ${COVERAGE_FLAGS})
            target_link_options(orpheus_tests PRIVATE ${COVERAGE_LINK_FLAGS})
        endif()

        # Custom target for coverage report
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            add_custom_target(coverage
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
                COMMAND lcov --capture --directory . --output-file coverage/coverage.info --ignore-errors source
                COMMAND lcov --remove coverage/coverage.info '/usr/*' '*/build/*' --output-file coverage/coverage.info
                COMMAND genhtml coverage/coverage.info --output-directory coverage/html
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating HTML coverage report in build/coverage/html/"
            )
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
            add_custom_target(coverage
                COMMAND llvm-profdata merge -sparse default.profraw -o coverage.profdata
                COMMAND llvm-cov show ./orpheus_tests -instr-profile=coverage.profdata -format=html -output-dir=coverage
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating HTML coverage report in build/coverage/"
            )
        endif()

        message(STATUS "Code coverage enabled. Run tests then: cmake --build build --target coverage")
    endif()
endif()

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------
if(SOLOUD_FETCHED)
    set(SOLOUD_SOURCE "fetched from GitHub")
elseif(soloud_FOUND)
    set(SOLOUD_SOURCE "system (CMake package)")
else()
    set(SOLOUD_SOURCE "system")
endif()

message(STATUS "")
message(STATUS "Orpheus ${PROJECT_VERSION}")
message(STATUS "  SoLoud:                  ${SOLOUD_SOURCE}")
message(STATUS "  Build examples:          ${ORPHEUS_BUILD_EXAMPLES}")
message(STATUS "  Build tests:             ${ORPHEUS_BUILD_TESTS}")
message(STATUS "  Build benchmarks:        ${ORPHEUS_BUILD_BENCHMARKS}")
message(STATUS "  Build fuzzer:            ${ORPHEUS_BUILD_FUZZER}")
message(STATUS "  Code coverage:           ${ORPHEUS_ENABLE_COVERAGE}")
message(STATUS "  Use precompiled headers: ${ORPHEUS_USE_PCH}")
message(STATUS "")

