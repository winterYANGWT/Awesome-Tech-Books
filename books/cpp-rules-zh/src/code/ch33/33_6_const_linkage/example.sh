set -e

$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror a.cpp b.cpp main.cpp -o "$BUILD/app"
"$BUILD/app"
