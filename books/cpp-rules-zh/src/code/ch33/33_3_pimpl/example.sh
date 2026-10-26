set -e

$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror document.cpp main.cpp -o "$BUILD/app"
"$BUILD/app"
