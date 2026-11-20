function(proj_enable_sanitizers target)
  if(NOT PROJ_SANITIZE)
    return()
  endif()

  if("thread" IN_LIST PROJ_SANITIZE AND "address" IN_LIST PROJ_SANITIZE)
    message(FATAL_ERROR
      "PROJ_SANITIZE=${PROJ_SANITIZE}：thread 不能和 address 同时使用")
  endif()

  string(JOIN "," names ${PROJ_SANITIZE})
  target_compile_options(${target} PRIVATE -fsanitize=${names})
  target_link_options(${target} PRIVATE -fsanitize=${names})
endfunction()
