#include "acceptor.hpp"

#include <sys/epoll.h>

void Acceptor::handle_event(std::uint32_t events) {
    if (events & EPOLLIN) manager_.accept_all(fd_);
    if (events & (EPOLLERR | EPOLLHUP)) loop_.stop();
}
