set -e
$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror -I. widget.cpp main.cpp -o "$BUILD/app"
"$BUILD/app"
