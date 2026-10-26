set -e

STD_SRC=/usr/lib/llvm-21/share/libc++/v1/std.cppm

$CXX -std=c++23 -stdlib=libc++ -Wno-reserved-module-identifier \
    --precompile "$STD_SRC" -o "$BUILD/std.pcm"
$CXX -std=c++23 -stdlib=libc++ -Wall -Wextra -Werror \
    -fmodule-file=std="$BUILD/std.pcm" app.cpp "$BUILD/std.pcm" -o "$BUILD/app"
"$BUILD/app"
