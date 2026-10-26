#include "bad_session.h"

namespace net {
struct Session::Impl {
    int fd = 0;
};

Session::Session() : impl_(std::make_unique<Impl>()) {}
}
