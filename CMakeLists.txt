cmake_minimum_required(VERSION 3.10)

project(PolyCubesThreadedTree)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

#Cpp 14 required by catch2
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED 14)

file(GLOB HEADER_FILES *.h *.hpp)

add_executable(PolyCubesThreadedTree cubes.cpp ${HEADER_FILES})

target_include_directories(PolyCubesThreadedTree PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/popl" 
	"${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/archive"
	"${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/emhash")
	
add_executable(PolyCubesThreadedTreeTest cubes_test.cpp "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/catch2/catch_amalgamated.cpp")

target_include_directories(PolyCubesThreadedTreeTest PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/catch2" 
	"${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/archive"
	"${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/emhash")