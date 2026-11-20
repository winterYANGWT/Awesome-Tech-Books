cmake_minimum_required(VERSION 3.25...4.2)
project(Emulator LANGUAGES CXX)

# 本机没有 aarch64 模拟器。这个工程的工具链只把 CMAKE_SYSTEM_NAME 设成
# Linux，让 CMake 进入交叉模式；编译器仍是主机的 g++，可执行文件能直接跑。
option(USE_EMULATOR "用模拟器运行目标程序" OFF)
if(USE_EMULATOR)
  # sh 只用来执行脚本，不需要脚本有可执行权限
  set(CMAKE_CROSSCOMPILING_EMULATOR "sh;${CMAKE_CURRENT_SOURCE_DIR}/tools/run.sh")
endif()
message(STATUS "cross=${CMAKE_CROSSCOMPILING} emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")

try_run(RUN_CODE COMPILE_OK
  SOURCE_FROM_CONTENT probe.cpp
    "#include <cstdio>\nint main() { std::printf(\"probe ran at configure time\\n\"); return 0; }"
  RUN_OUTPUT_VARIABLE RUN_OUT)
message(STATUS "compile=${COMPILE_OK} run=${RUN_CODE} out=${RUN_OUT}")

add_executable(probe main.cpp)
add_custom_command(TARGET probe POST_BUILD
  COMMAND probe
  COMMENT "用模拟器运行目标程序")
