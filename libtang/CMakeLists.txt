cmake_minimum_required(VERSION 3.5)
project(libtang)

option(BUILD_SHARED "Build shared Tang library" ON)
option(STATIC_BUILD "Create static build of Tang tools" OFF)

set(PROGRAM_PREFIX "" CACHE STRING "Name prefix for executables")

set(CMAKE_CXX_STANDARD 14)
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -bigobj -EHsc")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -O3")
endif()
set(CMAKE_DEFIN)
set(link_param "")
if (STATIC_BUILD)
    set(Boost_USE_STATIC_LIBS ON)
    if(MSVC)
        add_definitions(-DBOOST_PYTHON_STATIC_LIB)
        set(CMAKE_CXX_FLAGS_RELEASE "/MT")
        set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
    elseif (NOT APPLE)
        set(link_param "-static")
    endif()
else()
    if(MSVC)
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
endif()
if (WASI)
    set(USE_THREADS OFF)
    add_definitions(
        -DBOOST_EXCEPTION_DISABLE
        -DBOOST_NO_EXCEPTIONS
        -DBOOST_SP_NO_ATOMIC_ACCESS
        -DBOOST_AC_DISABLE_THREADS
        -DBOOST_NO_CXX11_HDR_MUTEX
        -DBOOST_NO_CXX11_HDR_ATOMIC
    )
else()
    set(USE_THREADS ON)
endif()
set(boost_libs filesystem program_options system)
if (USE_THREADS)
    list(APPEND boost_libs thread)
else()
    add_definitions(-DNO_THREADS)
endif()
set(Boost_NO_BOOST_CMAKE ON)

find_package(Boost REQUIRED COMPONENTS ${boost_libs})

find_package(Git)

include_directories(include/ ${Boost_INCLUDE_DIRS})

aux_source_directory(include/ INCLUDE_FILES)
aux_source_directory(src/ SOURCE_FILES)

if (BUILD_SHARED)
    add_library(tang SHARED ${INCLUDE_FILES} ${SOURCE_FILES})
else()
    add_library(tang STATIC ${INCLUDE_FILES} ${SOURCE_FILES})
endif()

target_link_libraries(tang LINK_PUBLIC ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

include(GNUInstallDirs)
file(RELATIVE_PATH TANG_RPATH_LIBDIR /${CMAKE_INSTALL_BINDIR} /${CMAKE_INSTALL_LIBDIR})
file(RELATIVE_PATH TANG_RPATH_DATADIR /${CMAKE_INSTALL_BINDIR} /${CMAKE_INSTALL_DATADIR})

function(setup_rpath name)
    if(APPLE)
        set_target_properties(${name} PROPERTIES
                              BUILD_WITH_INSTALL_RPATH ON
                              INSTALL_RPATH "@loader_path/${TANG_RPATH_LIBDIR}/${PROGRAM_PREFIX}tang"
                              INSTALL_NAME_DIR "@rpath")
    elseif(UNIX)
        set_target_properties(${name} PROPERTIES
                              BUILD_WITH_INSTALL_RPATH ON
                              INSTALL_RPATH "\$ORIGIN/${TANG_RPATH_LIBDIR}/${PROGRAM_PREFIX}tang")
    endif()
endfunction()


# Avoid perturbing build if git version hasn't changed
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
set(LAST_GIT_VERSION "")
if (NOT DEFINED CURRENT_GIT_VERSION)
    execute_process(COMMAND git describe --tags OUTPUT_VARIABLE CURRENT_GIT_VERSION WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()
string(STRIP "${CURRENT_GIT_VERSION}" CURRENT_GIT_VERSION)
if (EXISTS "${CMAKE_BINARY_DIR}/generated/last_git_version")
    file(READ "${CMAKE_BINARY_DIR}/generated/last_git_version" LAST_GIT_VERSION)
endif()
if (NOT ("${LAST_GIT_VERSION}" STREQUAL "${CURRENT_GIT_VERSION}") OR NOT GIT_EXECUTABLE)
    configure_file(
      ${CMAKE_SOURCE_DIR}/tools/version.cpp.in
      ${CMAKE_BINARY_DIR}/generated/version.cpp
    )
endif()
file(WRITE "${CMAKE_BINARY_DIR}/generated/last_git_version" CURRENT_GIT_VERSION)

add_executable(${PROGRAM_PREFIX}tangbit ${INCLUDE_FILES} tools/tangbit.cpp "${CMAKE_BINARY_DIR}/generated/version.cpp")
target_include_directories(${PROGRAM_PREFIX}tangbit PRIVATE tools)
target_compile_definitions(${PROGRAM_PREFIX}tangbit PRIVATE TANG_RPATH_DATADIR="${TANG_RPATH_DATADIR}" TANG_PREFIX="${CMAKE_INSTALL_PREFIX}" TANG_PROGRAM_PREFIX="${PROGRAM_PREFIX}")
target_link_libraries(${PROGRAM_PREFIX}tangbit tang ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${link_param})
setup_rpath(${PROGRAM_PREFIX}tangbit)

add_executable(${PROGRAM_PREFIX}tangunpack ${INCLUDE_FILES} tools/tangunpack.cpp "${CMAKE_BINARY_DIR}/generated/version.cpp")
target_include_directories(${PROGRAM_PREFIX}tangunpack PRIVATE tools)
target_compile_definitions(${PROGRAM_PREFIX}tangunpack PRIVATE TANG_RPATH_DATADIR="${TANG_RPATH_DATADIR}" TANG_PREFIX="${CMAKE_INSTALL_PREFIX}" TANG_PROGRAM_PREFIX="${PROGRAM_PREFIX}")
target_link_libraries(${PROGRAM_PREFIX}tangunpack tang ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${link_param})
setup_rpath(${PROGRAM_PREFIX}tangunpack)

add_executable(${PROGRAM_PREFIX}tangpack ${INCLUDE_FILES} tools/tangpack.cpp "${CMAKE_BINARY_DIR}/generated/version.cpp")
target_include_directories(${PROGRAM_PREFIX}tangpack PRIVATE tools)
target_compile_definitions(${PROGRAM_PREFIX}tangpack PRIVATE TANG_RPATH_DATADIR="${TANG_RPATH_DATADIR}" TANG_PREFIX="${CMAKE_INSTALL_PREFIX}" TANG_PROGRAM_PREFIX="${PROGRAM_PREFIX}")
target_link_libraries(${PROGRAM_PREFIX}tangpack tang ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${link_param})
setup_rpath(${PROGRAM_PREFIX}tangpack)

if (WASI)
    foreach (tool tangbit tangunpack tangpack)
        # set(CMAKE_EXECUTABLE_SUFFIX) breaks CMake tests for some reason
        set_property(TARGET ${PROGRAM_PREFIX}${tool} PROPERTY SUFFIX ".wasm")
    endforeach()
endif()

if (BUILD_SHARED)
    install(TARGETS tang ${PROGRAM_PREFIX}tangbit ${PROGRAM_PREFIX}tangunpack ${PROGRAM_PREFIX}tangpack
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROGRAM_PREFIX}tang
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
    install(TARGETS ${PROGRAM_PREFIX}tangbit ${PROGRAM_PREFIX}tangunpack ${PROGRAM_PREFIX}tangpack
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(DIRECTORY ../database DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROGRAM_PREFIX}tang PATTERN ".git" EXCLUDE)

file(GLOB_RECURSE CLANGFORMAT_FILES *.cpp *.hpp)

add_custom_target(
    clangformat
    COMMAND clang-format
    -style=file
    -i
    ${CLANGFORMAT_FILES}
)