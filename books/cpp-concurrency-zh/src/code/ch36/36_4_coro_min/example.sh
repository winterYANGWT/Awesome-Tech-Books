set -euo pipefail
# 手写 task<T> 加一个定时器 awaiter，不依赖 Asio。
$CXX $CXXFLAGS main.cpp timer_awaiter.cpp -o "$BUILD/coro_min"
"$BUILD/coro_min"
