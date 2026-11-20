# 这个文件放在 cmake/ 里，函数被顶层 CMakeLists.txt 调用。

function(myhelpers_configure_banner out_header out_include)
  set(template_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
  set(gen_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
  file(MAKE_DIRECTORY "${gen_dir}")
  configure_file("${template_dir}/banner.h.in" "${gen_dir}/banner.h" @ONLY)
  set(${out_header} "${gen_dir}/banner.h" PARENT_SCOPE)
  set(${out_include} "${gen_dir}" PARENT_SCOPE)
  message(STATUS "模板目录=${template_dir}")
  message(STATUS "调用目录=${CMAKE_CURRENT_LIST_DIR}")
endfunction()

function(myhelpers_make_identifier out_var text)
  string(MAKE_C_IDENTIFIER "${text}" identifier)
  set(${out_var} "id_${identifier}")
  # 3.25 起：把局部变量送回调用者，而不用 PARENT_SCOPE
  return(PROPAGATE ${out_var})
endfunction()
