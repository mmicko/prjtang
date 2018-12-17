cmake_minimum_required(VERSION 3.5)
project(libtang)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-Wall -pedantic -Wextra -O3")
set(CMAKE_DEFIN)
set(boost_libs filesystem thread program_options)

find_package(Boost REQUIRED COMPONENTS ${boost_libs})

include_directories(include/ ${Boost_INCLUDE_DIRS})

aux_source_directory(include/ INCLUDE_FILES)
aux_source_directory(src/ SOURCE_FILES)

add_library(tang STATIC ${INCLUDE_FILES} ${SOURCE_FILES})

target_link_libraries(tang LINK_PUBLIC ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(tangbit ${INCLUDE_FILES} tools/tangbit.cpp)
target_link_libraries(tangbit tang ${Boost_LIBRARIES})

