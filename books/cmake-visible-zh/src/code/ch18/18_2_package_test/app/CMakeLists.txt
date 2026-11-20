cmake_minimum_required(VERSION 3.25...4.2)
project(App LANGUAGES CXX)

find_package(Greet 1.0 CONFIG REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE Greet::greet)
add_custom_target(run COMMAND $<TARGET_FILE:app> DEPENDS app)
