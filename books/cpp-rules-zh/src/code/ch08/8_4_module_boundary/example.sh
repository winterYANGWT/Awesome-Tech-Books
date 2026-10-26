set -e
$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror -fPIC -shared \
    device.cpp -o "$BUILD/libdevice.so"
$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror \
    main.cpp -L"$BUILD" -ldevice -Wl,-rpath,"$BUILD" -o "$BUILD/app"
"$BUILD/app"
