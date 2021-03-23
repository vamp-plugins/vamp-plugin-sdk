cmake_minimum_required(VERSION 3.10)

project(vamp-plugin-sdk)

if(MSVC)
    add_compile_options("/W4")
else()
    add_compile_options("-Wall" "-Wextra")
endif()

# sdk
add_library(
    vamp-sdk
    src/vamp-sdk/FFT.cpp
    src/vamp-sdk/PluginAdapter.cpp
    src/vamp-sdk/RealTime.cpp
)
set_target_properties(vamp-sdk PROPERTIES POSITION_INDEPENDENT_CODE ON)  # to be linked to shared plugin libraries
target_include_directories(vamp-sdk PUBLIC .)
target_compile_features(vamp-sdk PUBLIC cxx_std_11)
add_library(vamp-plugin-sdk::vamp-sdk ALIAS vamp-sdk)

# host sdk
add_library(
    vamp-hostsdk
    src/vamp-hostsdk/Files.cpp
    src/vamp-hostsdk/PluginBufferingAdapter.cpp
    src/vamp-hostsdk/PluginChannelAdapter.cpp
    src/vamp-hostsdk/PluginHostAdapter.cpp
    src/vamp-hostsdk/PluginInputDomainAdapter.cpp
    src/vamp-hostsdk/PluginLoader.cpp
    src/vamp-hostsdk/PluginSummarisingAdapter.cpp
    src/vamp-hostsdk/PluginWrapper.cpp
    src/vamp-hostsdk/RealTime.cpp
)
target_link_libraries(vamp-hostsdk PUBLIC ${CMAKE_DL_LIBS})
target_include_directories(vamp-hostsdk PUBLIC .)
target_compile_definitions(vamp-hostsdk PUBLIC _USE_MATH_DEFINES)  # for e.g. M_PI constant
target_compile_features(vamp-hostsdk PUBLIC cxx_std_11)
add_library(vamp-plugin-sdk::vamp-hostsdk ALIAS vamp-hostsdk)

# rdf generator
option(VAMPSDK_BUILD_RDFGEN "Build RDF template generator" OFF)
if(VAMPSDK_BUILD_RDFGEN)
    add_executable(vamp-rdf-template-generator rdf/generator/vamp-rdf-template-generator.cpp)
    target_link_libraries(vamp-rdf-template-generator PRIVATE vamp-hostsdk)
endif()

# example plugins
option(VAMPSDK_BUILD_EXAMPLE_PLUGINS "Build example plugins" OFF)
if(VAMPSDK_BUILD_EXAMPLE_PLUGINS)
    file(GLOB sources_plugins "examples/*.cpp")
    add_library(vamp-example-plugins SHARED ${sources_plugins})
    set_target_properties(vamp-example-plugins PROPERTIES PREFIX "")  # remove lib* prefix
    target_link_libraries(vamp-example-plugins PRIVATE vamp-sdk)
endif()

# simple host
option(VAMPSDK_BUILD_SIMPLE_HOST "Build simple host" OFF)
if(VAMPSDK_BUILD_SIMPLE_HOST)
    find_path(LIBSNDFILE_INCLUDE_DIR NAMES sndfile.h)
    find_library(LIBSNDFILE_LIBRARY NAMES sndfile)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(
        SndFile
        REQUIRED_VARS
            LIBSNDFILE_LIBRARY
            LIBSNDFILE_INCLUDE_DIR
    )

    add_executable(vamp-simple-host host/vamp-simple-host.cpp)
    target_link_libraries(vamp-simple-host PRIVATE vamp-hostsdk ${LIBSNDFILE_LIBRARY})
    target_include_directories(vamp-simple-host PRIVATE ${LIBSNDFILE_INCLUDE_DIR})
endif()
