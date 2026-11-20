message(STATUS "helpers: CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}")
message(STATUS "helpers: CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")

file(READ "${CMAKE_CURRENT_LIST_DIR}/greeting.txt" HELPER_GREETING)
string(STRIP "${HELPER_GREETING}" HELPER_GREETING)

function(add_greeting target)
  target_compile_definitions(${target} PRIVATE "GREETING=\"${HELPER_GREETING}\"")
endfunction()
