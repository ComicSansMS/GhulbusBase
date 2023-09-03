cmake_minimum_required(VERSION 3.24)

project(GhulbusBase)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "If checked, generate dynamic instead of static libraries." OFF)

if(MSVC)
    option(GB_PARALLEL_BUILD "If set, uses multiple build processes in parallel." ON)
    if(GB_PARALLEL_BUILD)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    endif()
endif()

option(GB_GENERATE_COVERAGE_INFO "If set, line coverage info will be generated from debug test runs." OFF)
if(GB_GENERATE_COVERAGE_INFO)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # this is an undocumented option for changing the output extension from *.cpp.ext
        # to *.ext; As there is no reasonable way to obtain the list of object files
        # this seems to be the best option to generate the naming gcov expects.
        set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)
        set(GCOV_COMMAND gcov CACHE STRING "Command to invoke to run gcov.")
    endif()
endif()

###############################################################################
## Library gbBase
###############################################################################

set(GB_BASE_VERSION_MAJOR 0)
set(GB_BASE_VERSION_MINOR 9)
set(GB_BASE_VERSION_PATCH 0)
set(GB_BASE_VERSION "${GB_BASE_VERSION_MAJOR}.${GB_BASE_VERSION_MINOR}.${GB_BASE_VERSION_PATCH}")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(GB_BASE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(GB_BASE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(GB_BASE_TEST_DIR ${PROJECT_SOURCE_DIR}/test)

add_library(gbBase)

include(GenerateExportHeader)
generate_export_header(gbBase
    EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/generated/gbBase/gbBase_Export.hpp
    PREFIX_NAME GHULBUS_LIBRARY_
)
set(GB_BASE_GENERATED_HEADER_FILES ${PROJECT_BINARY_DIR}/generated/gbBase/gbBase_Export.hpp)

target_sources(gbBase
    PRIVATE
    ${GB_BASE_SOURCE_DIR}/Assert.cpp
    ${GB_BASE_SOURCE_DIR}/Log.cpp
    ${GB_BASE_SOURCE_DIR}/LogHandlers.cpp
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${GB_BASE_INCLUDE_DIR}
    FILES
    ${GB_BASE_INCLUDE_DIR}/gbBase/config.hpp
    ${GB_BASE_INCLUDE_DIR}/gbBase/AnyInvocable.hpp
    ${GB_BASE_INCLUDE_DIR}/gbBase/Assert.hpp
    ${GB_BASE_INCLUDE_DIR}/gbBase/Exception.hpp
    ${GB_BASE_INCLUDE_DIR}/gbBase/Finally.hpp
    ${GB_BASE_INCLUDE_DIR}/gbBase/FixedRing.hpp
    ${GB_BASE_INCLUDE_DIR}/gbBase/Log.hpp
    ${GB_BASE_INCLUDE_DIR}/gbBase/LogHandlers.hpp
    ${GB_BASE_INCLUDE_DIR}/gbBase/OverloadSet.hpp
    ${GB_BASE_INCLUDE_DIR}/gbBase/PerfLog.hpp
    ${GB_BASE_INCLUDE_DIR}/gbBase/UnusedVariable.hpp
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_BINARY_DIR}/generated
    FILES
    ${GB_BASE_GENERATED_HEADER_FILES}
)
target_link_libraries(gbBase PUBLIC Threads::Threads)
target_compile_definitions(gbBase PRIVATE $<$<CONFIG:Debug>:GHULBUS_CONFIG_ASSERT_LEVEL_DEBUG>)
target_compile_options(gbBase PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4>)
target_compile_options(gbBase PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/permissive->)
target_compile_options(gbBase PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:-pedantic -Wall>)
target_compile_options(gbBase PRIVATE $<$<CXX_COMPILER_ID:GNU>:-pedantic -Wall>)
target_sources(gbBase PRIVATE $<$<CXX_COMPILER_ID:MSVC>:${PROJECT_SOURCE_DIR}/debug_visualizers/gbBase.natvis>)
set_property(SOURCE ${PROJECT_SOURCE_DIR}/debug_visualizers/gbBase.natvis PROPERTY VS_TOOL_OVERRIDE Natvis)
if(BUILD_SHARED_LIBS)
    set_property(TARGET gbBase PROPERTY CXX_VISIBILITY_PRESET hidden)
    set_property(TARGET gbBase PROPERTY VISIBILITY_INLINES_HIDDEN ON)
endif()

set_property(TARGET gbBase PROPERTY VERSION ${GB_BASE_VERSION})
set_property(TARGET gbBase PROPERTY SOVERSION ${GB_BASE_VERSION_MAJOR})
set_property(TARGET gbBase PROPERTY DEBUG_POSTFIX d)

###############################################################################
## Testing gbBase
###############################################################################

option(GB_BUILD_TESTS "Determines whether to build tests." ON)
if(GB_BUILD_TESTS)
    enable_testing()

    if(NOT EXISTS ${PROJECT_BINARY_DIR}/external/catch2/catch_amalgamated.hpp)
        file(DOWNLOAD
             https://github.com/catchorg/Catch2/releases/download/v3.4.0/catch_amalgamated.hpp
             ${PROJECT_BINARY_DIR}/external/catch2/catch_amalgamated.hpp
             EXPECTED_HASH SHA1=be29d8580ffa74428f47e2480c327b4c1e25a183
             )
        file(DOWNLOAD
             https://github.com/catchorg/Catch2/releases/download/v3.4.0/catch_amalgamated.cpp
             ${PROJECT_BINARY_DIR}/external/catch2/catch_amalgamated.cpp
             EXPECTED_HASH SHA1=af0af6a63cee729a5672602b0c779c1d6426a546
             )
        file(WRITE ${PROJECT_BINARY_DIR}/external/catch2/catch.hpp "#include <catch_amalgamated.hpp>\n")
    endif()

    add_library(Catch2 STATIC)
    target_sources(Catch2
        PRIVATE
        ${PROJECT_BINARY_DIR}/external/catch2/catch_amalgamated.cpp
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_BINARY_DIR}/external/catch2
        FILES
        ${PROJECT_BINARY_DIR}/external/catch2/catch_amalgamated.hpp
        ${PROJECT_BINARY_DIR}/external/catch2/catch.hpp
    )

    add_executable(gbBase_Test)
    target_sources(gbBase_Test
        PRIVATE
        ${GB_BASE_TEST_DIR}/TestAnyInvocable.cpp
        ${GB_BASE_TEST_DIR}/TestAssert.cpp
        ${GB_BASE_TEST_DIR}/TestBase.cpp
        ${GB_BASE_TEST_DIR}/TestException.cpp
        ${GB_BASE_TEST_DIR}/TestFinally.cpp
        ${GB_BASE_TEST_DIR}/TestFixedRing.cpp
        ${GB_BASE_TEST_DIR}/TestLog.cpp
        ${GB_BASE_TEST_DIR}/TestLogHandlers.cpp
        ${GB_BASE_TEST_DIR}/TestOverloadSet.cpp
        ${GB_BASE_TEST_DIR}/TestPerfLog.cpp
    )
    target_link_libraries(gbBase_Test PUBLIC gbBase Catch2)
    target_compile_definitions(gbBase_Test PRIVATE $<$<CONFIG:Debug>:GHULBUS_CONFIG_ASSERT_LEVEL_DEBUG>)
    add_test(NAME TestBase COMMAND gbBase_Test)

    if(GB_GENERATE_COVERAGE_INFO AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
        target_compile_options(gbBase PRIVATE --coverage)
        target_link_libraries(gbBase PRIVATE --coverage)
        target_compile_options(gbBase_Test PRIVATE --coverage)
        target_link_libraries(gbBase_Test PRIVATE --coverage)
        add_custom_target(gbBase_TestCoverage
            COMMAND ${GCOV_COMMAND} -o CMakeFiles/gbBase.dir/src ${GB_BASE_SOURCE_FILES}
            COMMAND ${GCOV_COMMAND} -o CMakeFiles/gbBase_Test.dir/test ${GB_BASE_TEST_SOURCES}
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            COMMENT "Collecting gcov coverage information"
        )
    endif()
endif()

###############################################################################
## Doxygen gbBase
###############################################################################

option(GB_GENERATE_DOXYGEN_DOCUMENTATION "If checked, API documentation will be generated if Doxygen is available." ON)
if(GB_GENERATE_DOXYGEN_DOCUMENTATION)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(GB_LIB_DOC_DIR ${PROJECT_BINARY_DIR}/doc)
        configure_file(${PROJECT_SOURCE_DIR}/doc/Doxyfile.in ${GB_LIB_DOC_DIR}/Doxyfile @ONLY)
        add_custom_target(doxygen-doc
            COMMAND ${DOXYGEN_EXECUTABLE}
            WORKING_DIRECTORY ${GB_LIB_DOC_DIR}
            COMMENT "Building doxygen documentation"
            SOURCES ${PROJECT_SOURCE_DIR}/doc/Doxyfile.in
        )
        source_group("" FILES ${PROJECT_SOURCE_DIR}/doc/Doxyfile.in)
    endif()
endif()


###############################################################################
## Packaging gbBase
###############################################################################

set(GB_LIBRARY_NAMESPACE Ghulbus::)

install(TARGETS gbBase EXPORT gbBaseTargets
    RUNTIME DESTINATION bin/$<CONFIG>
    LIBRARY DESTINATION lib/$<CONFIG>
    ARCHIVE DESTINATION lib/$<CONFIG>
    INCLUDES DESTINATION include
    FILE_SET HEADERS
)
target_include_directories(gbBase PUBLIC $<INSTALL_INTERFACE:include>)
if(MSVC AND BUILD_SHARED_LIBS)
    install(FILES $<TARGET_PDB_FILE:gbBase> DESTINATION bin/Debug CONFIGURATIONS Debug)
    install(FILES $<TARGET_PDB_FILE:gbBase> DESTINATION bin/RelWithDebInfo CONFIGURATIONS RelWithDebInfo)
endif()
install(FILES LICENSE DESTINATION ".")

if(GB_GENERATE_DOXYGEN_DOCUMENTATION AND DOXYGEN_FOUND)
    install(DIRECTORY ${GB_LIB_DOC_DIR}/html DESTINATION doc MESSAGE_NEVER)
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/gbBaseConfig.cmake.in
    ${PROJECT_BINARY_DIR}/cmake/gbBaseConfig.cmake
    INSTALL_DESTINATION cmake
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/cmake/gbBaseVersion.cmake
    VERSION ${GB_BASE_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(${PROJECT_SOURCE_DIR}/cmake/gbBaseMacros.cmake.in
    ${PROJECT_BINARY_DIR}/cmake/gbBaseMacros.cmake
    COPYONLY
)

# installation - build tree specific package config files
export(EXPORT gbBaseTargets FILE ${PROJECT_BINARY_DIR}/cmake/gbBaseTargets.cmake NAMESPACE ${GB_LIBRARY_NAMESPACE})
export(PACKAGE gbBase)

# installation - relocatable package config files
install(EXPORT gbBaseTargets
    FILE gbBaseTargets.cmake
    NAMESPACE ${GB_LIBRARY_NAMESPACE}
    DESTINATION cmake
)
install(FILES
    ${PROJECT_BINARY_DIR}/cmake/gbBaseConfig.cmake
    ${PROJECT_BINARY_DIR}/cmake/gbBaseMacros.cmake
    ${PROJECT_BINARY_DIR}/cmake/gbBaseVersion.cmake
    DESTINATION cmake
)
