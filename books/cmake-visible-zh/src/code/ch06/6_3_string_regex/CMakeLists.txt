cmake_minimum_required(VERSION 3.25...4.2)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" VERSION_TEXT)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" VERSION_LINE REGEX "^v")

string(REGEX MATCH "^v([0-9]+\\.[0-9]+\\.[0-9]+)$" VERSION_MATCH "${VERSION_LINE}")
set(APP_VERSION "${CMAKE_MATCH_1}")

string(REGEX MATCH "codename: ([A-Za-z]+)" CODENAME_MATCH "${VERSION_TEXT}")
set(APP_CODENAME "${CMAKE_MATCH_1}")

project(StringRegex VERSION ${APP_VERSION} DESCRIPTION "codename ${APP_CODENAME}" LANGUAGES CXX)

string(TOUPPER "${APP_CODENAME}" CODENAME_UPPER)
string(MAKE_C_IDENTIFIER "build-${PROJECT_VERSION}" BUILD_ID)
message(STATUS "版本 ${PROJECT_VERSION}，代号 ${CODENAME_UPPER}，标识 ${BUILD_ID}")

configure_file(config.h.in config.h @ONLY)

add_executable(app main.cpp)
target_include_directories(app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(app PRIVATE BUILD_ID="${BUILD_ID}")
