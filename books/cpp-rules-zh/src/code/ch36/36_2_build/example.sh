#!/usr/bin/env bash
# 演示：一次干净的 CI 构建；第二次增量构建不重写目标文件
set -euo pipefail
cd "$(dirname "$0")"

# 1. 干净的 CI 构建：配置 + 编译 + 测试，全部在一个目录里完成
bash ci.sh "$BUILD/ci" > "$BUILD/ci.log" 2>&1
echo "ci.sh on a fresh tree:"
grep -E 'Built target|tests passed' "$BUILD/ci.log" | sed 's/^/  /'

# 2. 第二次增量构建：记录 .o 的时间戳，再构建一次，确认没有被重写
cmake -S . -B "$BUILD/inc" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug > /dev/null
cmake --build "$BUILD/inc" --parallel 2 > /dev/null
before=$(find "$BUILD/inc" -name '*.o' -printf '%T@ %p\n' | sort)
cmake --build "$BUILD/inc" --parallel 2 > /dev/null
after=$(find "$BUILD/inc" -name '*.o' -printf '%T@ %p\n' | sort)
if [ "$before" = "$after" ]; then
    echo "second incremental build: no object file rewritten"
else
    echo "second incremental build: object files were rewritten"
    exit 1
fi

# 3. 改一个源文件，增量构建只重编受影响的翻译单元
touch src/freight.cpp
cmake --build "$BUILD/inc" --parallel 2 > /dev/null
if [ "$before" != "$(find "$BUILD/inc" -name '*.o' -printf '%T@ %p\n' | sort)" ]; then
    echo "after touching src/freight.cpp: one object file rebuilt"
else
    echo "after touching src/freight.cpp: nothing rebuilt"
    exit 1
fi

# 4. 测试可执行文件是构建产物，直接运行也给出确定的结果
"$BUILD/ci/freight_test"
