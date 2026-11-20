# 从 INPUT 读两列 CSV，生成一个返回行数和总和的 C++ 文件，再写一份摘要。
file(STRINGS "${INPUT}" rows)
set(sum 0)
set(count 0)
foreach(row IN LISTS rows)
  if(row MATCHES "^[ \t]*[^,]+, *([0-9]+) *$")
    math(EXPR sum "${sum} + ${CMAKE_MATCH_1}")
    math(EXPR count "${count} + 1")
  endif()
endforeach()

file(WRITE "${OUTPUT}"
"// 由 gen_table.cmake 从 table.csv 生成，不要手改。
extern \"C\" int table_count() { return ${count}; }
extern \"C\" int table_sum() { return ${sum}; }
")
file(WRITE "${ROWS}" "rows=${count}\nsum=${sum}\n")
