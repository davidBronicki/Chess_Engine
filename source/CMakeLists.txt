cmake_minimum_required(VERSION 3.0)
project(CHESS_ENGINE CXX)


set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)


find_package(Threads REQUIRED)


file(GLOB Main_Code_Pile
	"*.cpp")

add_executable(bronicki_engine ${Main_Code_Pile})

set_target_properties(bronicki_engine PROPERTIES CXX_STANDARD 20)

target_link_libraries(bronicki_engine Threads::Threads)