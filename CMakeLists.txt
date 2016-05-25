cmake_minimum_required(VERSION 3.3)

project(GhulbusBase)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if($ENV{TRAVIS})
    if($ENV{TRAVIS} STREQUAL "true")
        message(STATUS "Building on Travis-CI.")
        set(IS_TRAVIS_BUILD TRUE)
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

add_subdirectory(external)

###############################################################################
## Library gbBase
###############################################################################

set(GB_BASE_VERSION_MAJOR 0)
set(GB_BASE_VERSION_MINOR 1)
set(GB_BASE_VERSION_PATCH 0)
set(GB_BASE_VERSION "${GB_BASE_VERSION_MAJOR}.${GB_BASE_VERSION_MINOR}.${GB_BASE_VERSION_PATCH}")

option(GB_BASE_BUILD_BARE "Set this if you want to build without external dependencies; Some features will be disabled." OFF)

if(NOT GB_BASE_BUILD_BARE)
    set(Boost_USE_STATIC_LIBS ON)
    find_package(Boost REQUIRED)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(GB_BASE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(GB_BASE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(GB_BASE_TEST_DIR ${PROJECT_SOURCE_DIR}/test)

set(GB_BASE_SOURCE_FILES
    ${GB_BASE_SOURCE_DIR}/Assert.cpp
)
if(NOT GB_BASE_BUILD_BARE)
    list(APPEND GB_BASE_SOURCE_FILES
        ${GB_BASE_SOURCE_DIR}/Log.cpp
        ${GB_BASE_SOURCE_DIR}/LogHandlers.cpp
    )
endif()

set(GB_BASE_HEADER_FILES
    ${GB_BASE_INCLUDE_DIR}/gbBase/config.hpp
    ${GB_BASE_INCLUDE_DIR}/gbBase/Assert.hpp
    ${GB_BASE_INCLUDE_DIR}/gbBase/RingPool.hpp
    ${GB_BASE_INCLUDE_DIR}/gbBase/UnusedVariable.hpp
)
if(NOT GB_BASE_BUILD_BARE)
    list(APPEND GB_BASE_HEADER_FILES
        ${GB_BASE_INCLUDE_DIR}/gbBase/Exception.hpp
        ${GB_BASE_INCLUDE_DIR}/gbBase/Log.hpp
        ${GB_BASE_INCLUDE_DIR}/gbBase/LogHandlers.hpp
    )
endif()

set(GB_BASE_TEST_SOURCES
    ${GB_BASE_TEST_DIR}/TestAssert.cpp
    ${GB_BASE_TEST_DIR}/TestBase.cpp
    ${GB_BASE_TEST_DIR}/TestRingPool.cpp
)
if(NOT GB_BASE_BUILD_BARE)
    list(APPEND GB_BASE_TEST_SOURCES
        ${GB_BASE_TEST_DIR}/TestException.cpp
        ${GB_BASE_TEST_DIR}/TestLog.cpp
        ${GB_BASE_TEST_DIR}/TestLogHandlers.cpp
    )
endif()

add_library(gbBase ${GB_BASE_SOURCE_FILES} ${GB_BASE_HEADER_FILES})
target_compile_features(gbBase PRIVATE cxx_lambdas cxx_override cxx_range_for)
target_compile_features(gbBase PUBLIC cxx_attributes cxx_func_identifier cxx_noexcept)
target_include_directories(gbBase PUBLIC $<BUILD_INTERFACE:${GB_BASE_INCLUDE_DIR}>)
if(GB_BASE_BUILD_BARE)
    target_compile_definitions(gbBase PUBLIC GHULBUS_CONFIG_BASE_BARE_BUILD)
else()
    target_include_directories(gbBase PUBLIC $<BUILD_INTERFACE:${Boost_INCLUDE_DIR}>)
    target_compile_definitions(gbBase PUBLIC BOOST_ALL_NO_LIB)
    # TODO: using the imported target will break the INSTALL for static builds
    #target_link_libraries(gbBase PRIVATE hinnant_date)
    target_include_directories(gbBase PRIVATE ${PROJECT_SOURCE_DIR}/external/hinnant_date)
    target_link_libraries(gbBase PUBLIC Threads::Threads)
endif()
target_compile_definitions(gbBase PUBLIC $<$<CONFIG:Debug>:GHULBUS_CONFIG_ASSERT_LEVEL_DEBUG>)
target_compile_options(gbBase PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4>)
target_compile_options(gbBase PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:-pedantic -Wall>)
target_compile_options(gbBase PRIVATE $<$<CXX_COMPILER_ID:GNU>:-pedantic -Wall>)

option(BUILD_SHARED_LIBS "If checked, generate dynamic instead of static libraries." OFF)
include(GenerateExportHeader)
generate_export_header(gbBase
    EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/generated/gbBase/gbBase_Export.hpp
    PREFIX_NAME GHULBUS_LIBRARY_
)
set(GB_BASE_GENERATED_HEADER_FILES ${PROJECT_BINARY_DIR}/generated/gbBase/gbBase_Export.hpp)
target_include_directories(gbBase PUBLIC $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generated>)
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

    if(NOT TARGET Catch)
        include(ExternalProject)
        if(IS_TRAVIS_BUILD)     # on travis, use git for fetching instead of wget
            set(GB_FETCH_EXTERNAL_CATCH
                GIT_REPOSITORY https://github.com/philsquared/Catch.git
                GIT_TAG 0c1c9fa9229c7358ce16c9910004a39290b20f5f)
        elseif(WIN32)
            set(GB_FETCH_EXTERNAL_CATCH
                URL https://github.com/philsquared/Catch/archive/v1.2.1-develop.12.zip
                URL_HASH MD5=cda228922a1c9248364c99a3ff9cd9fa)
        else()
            set(GB_FETCH_EXTERNAL_CATCH
                URL https://github.com/philsquared/Catch/archive/v1.2.1-develop.12.tar.gz
                URL_HASH MD5=a8dfb7be899a6e7fb30bd55d53426122)
        endif()
        ExternalProject_Add(Catch-External
            PREFIX ${CMAKE_BINARY_DIR}/external/Catch
            ${GB_FETCH_EXTERNAL_CATCH}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/external/Catch/src/Catch-External/single_include/catch.hpp
                                                     ${CMAKE_BINARY_DIR}/external/Catch/include/catch.hpp
        )
        add_library(Catch INTERFACE)
        add_dependencies(Catch Catch-External)
        target_include_directories(Catch INTERFACE ${CMAKE_BINARY_DIR}/external/Catch/include)
    endif()

    add_executable(gbBase_Test ${GB_BASE_TEST_SOURCES})
    target_link_libraries(gbBase_Test gbBase Catch)
    add_test(NAME TestBase COMMAND gbBase_Test)

    if(GB_GENERATE_COVERAGE_INFO AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
        target_compile_options(gbBase PRIVATE --coverage -fprofile-arcs -ftest-coverage)
        target_link_libraries(gbBase PRIVATE -fprofile-arcs -ftest-coverage)
        add_custom_target(gbBase_TestCoverage
            COMMAND ${GCOV_COMMAND} -o CMakeFiles/gbBase.dir/src ${GB_BASE_SOURCE_FILES}
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
)
target_include_directories(gbBase PUBLIC $<INSTALL_INTERFACE:include>)
install(FILES ${GB_BASE_HEADER_FILES} ${GB_BASE_GENERATED_HEADER_FILES} DESTINATION include/gbBase)
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
