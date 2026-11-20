cmake_minimum_required(VERSION 3.25...4.2)
project(Components LANGUAGES CXX)

find_package(Boost 1.80 CONFIG REQUIRED COMPONENTS filesystem)
message(STATUS "Boost_VERSION=${Boost_VERSION} Boost_DIR=${Boost_DIR}")

add_executable(app main.cpp)
target_link_libraries(app PRIVATE Boost::filesystem)
