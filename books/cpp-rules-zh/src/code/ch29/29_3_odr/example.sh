set -e
$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 \
    "$PWD/main.cpp" "$PWD/a.cpp" "$PWD/b.cpp" -o "$BUILD/app"
"$BUILD/app"
